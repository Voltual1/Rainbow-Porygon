#include "global.h"
#include "agb_flash.h"
#include "gba/flash_internal.h"
#include "fieldmap.h"
#include "save.h"
#include "task.h"
#include "decompress.h"
#include "load_save.h"
#include "overworld.h"
#include "hall_of_fame.h"
#include "pokemon_storage_system.h"
#include "trainer_hill.h"
#include "link.h"
#include "constants/game_stat.h"
#include "malloc.h"
#include <stdio.h>
#include <string.h>

#define MODERN_SAVE_MAGIC 0x4d454b50 // 'PKEM'
#define MODERN_SAVE_VERSION 1

#define BLOCK_ID_SAVEBLOCK2     1
#define BLOCK_ID_SAVEBLOCK1     2
#define BLOCK_ID_SAVEBLOCK3     3
#define BLOCK_ID_STORAGE        4
#define BLOCK_ID_HOF            5
#define BLOCK_ID_TRAINERIAL     6
#define BLOCK_ID_RECBATTLE      7

struct ModernSaveHeader {
    u32 magic;
    u32 version;
    u32 numBlocks;
    u32 reserved[8];
};

struct ModernSaveBlockHeader {
    u32 blockId;
    u32 size;
};

extern char gSavePath[];

static u16 CalculateChecksum(void *, u16);
static bool8 ReadFlashSector(u8, struct SaveSector *);
static u8 GetSaveValidStatus(const struct SaveSectorLocation *);
static u8 CopySaveSlotData(u16, struct SaveSectorLocation *);
static u8 TryWriteSector(u8, u8 *);
static u8 HandleWriteSector(u16, const struct SaveSectorLocation *);
static u8 HandleReplaceSector(u16, const struct SaveSectorLocation *);
static void CopyToSaveBlock3(u32, struct SaveSector *);
static void CopyFromSaveBlock3(u32, struct SaveSector *);

#define SAVEBLOCK_CHUNK(structure, chunkNum)                                   \
{                                                                              \
    chunkNum * SECTOR_DATA_SIZE,                                               \
    sizeof(structure) >= chunkNum * SECTOR_DATA_SIZE ?                         \
    min(sizeof(structure) - chunkNum * SECTOR_DATA_SIZE, SECTOR_DATA_SIZE) : 0 \
}

struct
{
    u16 offset;
    u16 size;
} static const sSaveSlotLayout[NUM_SECTORS_PER_SLOT] =
{
    SAVEBLOCK_CHUNK(struct SaveBlock2, 0), // SECTOR_ID_SAVEBLOCK2

    SAVEBLOCK_CHUNK(struct SaveBlock1, 0), // SECTOR_ID_SAVEBLOCK1_START
    SAVEBLOCK_CHUNK(struct SaveBlock1, 1),
    SAVEBLOCK_CHUNK(struct SaveBlock1, 2),
    SAVEBLOCK_CHUNK(struct SaveBlock1, 3), // SECTOR_ID_SAVEBLOCK1_END

    SAVEBLOCK_CHUNK(struct PokemonStorage, 0), // SECTOR_ID_PKMN_STORAGE_START
    SAVEBLOCK_CHUNK(struct PokemonStorage, 1),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 2),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 3),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 4),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 5),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 6),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 7),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 8), // SECTOR_ID_PKMN_STORAGE_END
};

STATIC_ASSERT(sizeof(struct SaveBlock3) <= SAVE_BLOCK_3_CHUNK_SIZE * NUM_SECTORS_PER_SLOT, SaveBlock3FreeSpace);
STATIC_ASSERT(sizeof(struct SaveBlock2) <= SECTOR_DATA_SIZE, SaveBlock2FreeSpace);
STATIC_ASSERT(sizeof(struct SaveBlock1) <= SECTOR_DATA_SIZE * (SECTOR_ID_SAVEBLOCK1_END - SECTOR_ID_SAVEBLOCK1_START + 1), SaveBlock1FreeSpace);
STATIC_ASSERT(sizeof(struct PokemonStorage) <= SECTOR_DATA_SIZE * (SECTOR_ID_PKMN_STORAGE_END - SECTOR_ID_PKMN_STORAGE_START + 1), PokemonStorageFreeSpace);

COMMON_DATA u16 gLastWrittenSector = 0;
COMMON_DATA u32 gLastSaveCounter = 0;
COMMON_DATA u16 gLastKnownGoodSector = 0;
COMMON_DATA u32 gDamagedSaveSectors = 0;
COMMON_DATA u32 gSaveCounter = 0;
COMMON_DATA struct SaveSector *gReadWriteSector = NULL;
COMMON_DATA u16 gIncrementalSectorId = 0;
COMMON_DATA u16 gSaveFileStatus = 0;
COMMON_DATA MainCallback gGameContinueCallback = NULL;
COMMON_DATA struct SaveSectorLocation gRamSaveSectorLocations[NUM_SECTORS_PER_SLOT] = {0};
COMMON_DATA u16 gSaveAttemptStatus = 0;

EWRAM_DATA struct SaveSector gSaveDataBuffer = {0};

static u8 SaveModernSave(u8 saveType)
{
    u8 *hofBuffer = NULL;
    u32 hofSize = 0;
    u8 *trHillBuffer = NULL;
    u32 trHillSize = 0;
    u8 *recBattleBuffer = NULL;
    u32 recBattleSize = 0;

    FILE *fInfo = fopen(gSavePath, "rb");
    if (fInfo != NULL)
    {
        struct ModernSaveHeader oldHeader;
        if (fread(&oldHeader, sizeof(oldHeader), 1, fInfo) == 1 && oldHeader.magic == MODERN_SAVE_MAGIC)
        {
            u32 i;
            for (i = 0; i < oldHeader.numBlocks; i++)
            {
                struct ModernSaveBlockHeader oldBlock;
                if (fread(&oldBlock, sizeof(oldBlock), 1, fInfo) != 1)
                    break;

                if (oldBlock.blockId == BLOCK_ID_HOF)
                {
                    hofSize = oldBlock.size;
                    hofBuffer = Alloc(hofSize);
                    if (hofBuffer) fread(hofBuffer, hofSize, 1, fInfo);
                    else fseek(fInfo, hofSize, SEEK_CUR);
                }
                else if (oldBlock.blockId == BLOCK_ID_TRAINERIAL)
                {
                    trHillSize = oldBlock.size;
                    trHillBuffer = Alloc(trHillSize);
                    if (trHillBuffer) fread(trHillBuffer, trHillSize, 1, fInfo);
                    else fseek(fInfo, trHillSize, SEEK_CUR);
                }
                else if (oldBlock.blockId == BLOCK_ID_RECBATTLE)
                {
                    recBattleSize = oldBlock.size;
                    recBattleBuffer = Alloc(recBattleSize);
                    if (recBattleBuffer) fread(recBattleBuffer, recBattleSize, 1, fInfo);
                    else fseek(fInfo, recBattleSize, SEEK_CUR);
                }
                else
                {
                    fseek(fInfo, oldBlock.size, SEEK_CUR);
                }
            }
        }
        fclose(fInfo);
    }

    if (saveType == SAVE_HALL_OF_FAME && gHoFSaveBuffer != NULL)
    {
        if (hofBuffer) Free(hofBuffer);
        hofSize = 7936;
        hofBuffer = Alloc(hofSize);
        if (hofBuffer)
            memcpy(hofBuffer, gHoFSaveBuffer, hofSize);
    }

    FILE *f = fopen(gSavePath, "wb");
    if (f == NULL)
    {
        if (hofBuffer) Free(hofBuffer);
        if (trHillBuffer) Free(trHillBuffer);
        if (recBattleBuffer) Free(recBattleBuffer);
        return SAVE_STATUS_ERROR;
    }

    struct ModernSaveHeader header;
    header.magic = MODERN_SAVE_MAGIC;
    header.version = MODERN_SAVE_VERSION;
    header.numBlocks = 4; // SB2, SB1, SB3, Storage
    if (hofBuffer) header.numBlocks++;
    if (trHillBuffer) header.numBlocks++;
    if (recBattleBuffer) header.numBlocks++;
    memset(header.reserved, 0, sizeof(header.reserved));

    fwrite(&header, sizeof(header), 1, f);

    struct ModernSaveBlockHeader blockHeader;

    // Block 1: SaveBlock2
    blockHeader.blockId = BLOCK_ID_SAVEBLOCK2;
    blockHeader.size = sizeof(struct SaveBlock2);
    fwrite(&blockHeader, sizeof(blockHeader), 1, f);
    fwrite(gSaveBlock2Ptr, sizeof(struct SaveBlock2), 1, f);

    // Block 2: SaveBlock1
    blockHeader.blockId = BLOCK_ID_SAVEBLOCK1;
    blockHeader.size = sizeof(struct SaveBlock1);
    fwrite(&blockHeader, sizeof(blockHeader), 1, f);
    fwrite(gSaveBlock1Ptr, sizeof(struct SaveBlock1), 1, f);

    // Block 3: SaveBlock3
    blockHeader.blockId = BLOCK_ID_SAVEBLOCK3;
    blockHeader.size = sizeof(struct SaveBlock3);
    fwrite(&blockHeader, sizeof(blockHeader), 1, f);
    fwrite(gSaveBlock3Ptr, sizeof(struct SaveBlock3), 1, f);

    // Block 4: PokemonStorage
    blockHeader.blockId = BLOCK_ID_STORAGE;
    blockHeader.size = sizeof(struct PokemonStorage);
    fwrite(&blockHeader, sizeof(blockHeader), 1, f);
    fwrite(gPokemonStoragePtr, sizeof(struct PokemonStorage), 1, f);

    if (hofBuffer)
    {
        blockHeader.blockId = BLOCK_ID_HOF;
        blockHeader.size = hofSize;
        fwrite(&blockHeader, sizeof(blockHeader), 1, f);
        fwrite(hofBuffer, hofSize, 1, f);
        Free(hofBuffer);
    }

    if (trHillBuffer)
    {
        blockHeader.blockId = BLOCK_ID_TRAINERIAL;
        blockHeader.size = trHillSize;
        fwrite(&blockHeader, sizeof(blockHeader), 1, f);
        fwrite(trHillBuffer, trHillSize, 1, f);
        Free(trHillBuffer);
    }

    if (recBattleBuffer)
    {
        blockHeader.blockId = BLOCK_ID_RECBATTLE;
        blockHeader.size = recBattleSize;
        fwrite(&blockHeader, sizeof(blockHeader), 1, f);
        fwrite(recBattleBuffer, recBattleSize, 1, f);
        Free(recBattleBuffer);
    }

    fclose(f);
    return SAVE_STATUS_OK;
}

static u8 LoadModernSave(u8 saveType)
{
    FILE *f = fopen(gSavePath, "rb");
    if (f == NULL)
        return SAVE_STATUS_EMPTY;

    struct ModernSaveHeader header;
    if (fread(&header, sizeof(header), 1, f) != 1 || header.magic != MODERN_SAVE_MAGIC)
    {
        fclose(f);
        return SAVE_STATUS_CORRUPT;
    }

    u8 status = SAVE_STATUS_OK;
    bool32 loadedRequired[4] = {FALSE};

    u32 i;
    for (i = 0; i < header.numBlocks; i++)
    {
        struct ModernSaveBlockHeader blockHeader;
        if (fread(&blockHeader, sizeof(blockHeader), 1, f) != 1)
        {
            status = SAVE_STATUS_CORRUPT;
            break;
        }

        u8 *dest = NULL;
        u32 expectedSize = 0;

        switch (blockHeader.blockId)
        {
        case BLOCK_ID_SAVEBLOCK2:
            dest = (u8 *)gSaveBlock2Ptr;
            expectedSize = sizeof(struct SaveBlock2);
            loadedRequired[0] = TRUE;
            break;
        case BLOCK_ID_SAVEBLOCK1:
            dest = (u8 *)gSaveBlock1Ptr;
            expectedSize = sizeof(struct SaveBlock1);
            loadedRequired[1] = TRUE;
            break;
        case BLOCK_ID_SAVEBLOCK3:
            dest = (u8 *)gSaveBlock3Ptr;
            expectedSize = sizeof(struct SaveBlock3);
            loadedRequired[2] = TRUE;
            break;
        case BLOCK_ID_STORAGE:
            dest = (u8 *)gPokemonStoragePtr;
            expectedSize = sizeof(struct PokemonStorage);
            loadedRequired[3] = TRUE;
            break;
        case BLOCK_ID_HOF:
            if (saveType == SAVE_HALL_OF_FAME && gHoFSaveBuffer != NULL)
            {
                dest = (u8 *)gHoFSaveBuffer;
                expectedSize = min(blockHeader.size, 7936);
            }
            break;
        }

        if (dest != NULL && expectedSize != 0)
        {
            u32 readSize = min(blockHeader.size, expectedSize);
            fread(dest, readSize, 1, f);
            if (blockHeader.size > expectedSize)
            {
                fseek(f, blockHeader.size - expectedSize, SEEK_CUR);
            }
            else if (blockHeader.size < expectedSize)
            {
                memset(dest + readSize, 0, expectedSize - readSize);
            }
        }
        else
        {
            fseek(f, blockHeader.size, SEEK_CUR);
        }
    }

    fclose(f);

    if (saveType == SAVE_NORMAL)
    {
        if (!loadedRequired[0] || !loadedRequired[1] || !loadedRequired[2] || !loadedRequired[3])
            return SAVE_STATUS_CORRUPT;
    }

    return status;
}

static u32 WriteSpecialBlock(u32 targetBlockId, u8 *src, u32 size)
{
    u8 *blocksData[10] = {NULL};
    u32 blocksSize[10] = {0};
    u32 blocksId[10] = {0};
    u32 numBlocks = 0;

    FILE *fInfo = fopen(gSavePath, "rb");
    if (fInfo != NULL)
    {
        struct ModernSaveHeader oldHeader;
        if (fread(&oldHeader, sizeof(oldHeader), 1, fInfo) == 1 && oldHeader.magic == MODERN_SAVE_MAGIC)
        {
            u32 i;
            for (i = 0; i < oldHeader.numBlocks; i++)
            {
                struct ModernSaveBlockHeader oldBlock;
                if (fread(&oldBlock, sizeof(oldBlock), 1, fInfo) != 1)
                    break;

                blocksId[numBlocks] = oldBlock.blockId;
                blocksSize[numBlocks] = oldBlock.size;
                blocksData[numBlocks] = Alloc(oldBlock.size);
                if (blocksData[numBlocks])
                {
                    fread(blocksData[numBlocks], oldBlock.size, 1, fInfo);
                }
                else
                {
                    fseek(fInfo, oldBlock.size, SEEK_CUR);
                }
                numBlocks++;
            }
        }
        fclose(fInfo);
    }

    bool32 found = FALSE;
    u32 i;
    for (i = 0; i < numBlocks; i++)
    {
        if (blocksId[i] == targetBlockId)
        {
            if (blocksData[i]) Free(blocksData[i]);
            blocksSize[i] = size;
            blocksData[i] = Alloc(size);
            if (blocksData[i])
                memcpy(blocksData[i], src, size);
            found = TRUE;
            break;
        }
    }

    if (!found && numBlocks < 10)
    {
        blocksId[numBlocks] = targetBlockId;
        blocksSize[numBlocks] = size;
        blocksData[numBlocks] = Alloc(size);
        if (blocksData[numBlocks])
            memcpy(blocksData[numBlocks], src, size);
        numBlocks++;
    }

    FILE *f = fopen(gSavePath, "wb");
    if (f == NULL)
    {
        for (i = 0; i < numBlocks; i++)
            if (blocksData[i]) Free(blocksData[i]);
        return SAVE_STATUS_ERROR;
    }

    struct ModernSaveHeader header;
    header.magic = MODERN_SAVE_MAGIC;
    header.version = MODERN_SAVE_VERSION;
    header.numBlocks = numBlocks;
    memset(header.reserved, 0, sizeof(header.reserved));

    fwrite(&header, sizeof(header), 1, f);

    for (i = 0; i < numBlocks; i++)
    {
        if (blocksData[i])
        {
            struct ModernSaveBlockHeader blockHeader;
            blockHeader.blockId = blocksId[i];
            blockHeader.size = blocksSize[i];
            fwrite(&blockHeader, sizeof(blockHeader), 1, f);
            fwrite(blocksData[i], blocksSize[i], 1, f);
            Free(blocksData[i]);
        }
    }

    fclose(f);
    return SAVE_STATUS_OK;
}

void ClearSaveData(void)
{
    remove(gSavePath);

    memset(gFlashBaseBuffer, 0xFF, sizeof(gFlashBaseBuffer));
    
    if (gFlashMemoryPresent == TRUE)
    {
        u16 i;
        for (i = 0; i < SECTORS_COUNT / 2; i++)
        {
            EraseFlashSector(i);
            EraseFlashSector(i + SECTORS_COUNT / 2);
        }
    }
}

void Save_ResetSaveCounters(void)
{
    gSaveCounter = 0;
    gLastWrittenSector = 0;
    gDamagedSaveSectors = 0;
}

static bool32 SetDamagedSectorBits(u8 op, u8 sectorId)
{
    bool32 retVal = FALSE;

    switch (op)
    {
    case ENABLE:
        gDamagedSaveSectors |= (1 << sectorId);
        break;
    case DISABLE:
        gDamagedSaveSectors &= ~(1 << sectorId);
        break;
    case CHECK:
        if (gDamagedSaveSectors & (1 << sectorId))
            retVal = TRUE;
        break;
    }

    return retVal;
}

static u8 WriteSaveSectorOrSlot(u16 sectorId, const struct SaveSectorLocation *locations)
{
    u32 status;
    u16 i;

    gReadWriteSector = &gSaveDataBuffer;

    if (sectorId != FULL_SAVE_SLOT)
    {
        status = HandleWriteSector(sectorId, locations);
    }
    else
    {
        gLastKnownGoodSector = gLastWrittenSector;
        gLastSaveCounter = gSaveCounter;
        gLastWrittenSector++;
        gLastWrittenSector = gLastWrittenSector % NUM_SECTORS_PER_SLOT;
        gSaveCounter++;
        status = SAVE_STATUS_OK;

        for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
            HandleWriteSector(i, locations);

        if (gDamagedSaveSectors)
        {
            status = SAVE_STATUS_ERROR;
            gLastWrittenSector = gLastKnownGoodSector;
            gSaveCounter = gLastSaveCounter;
        }
    }

    return status;
}

static u8 HandleWriteSector(u16 sectorId, const struct SaveSectorLocation *locations)
{
    u16 i;
    u16 sector;
    u8 *data;
    u16 size;

    sector = sectorId + gLastWrittenSector;
    sector %= NUM_SECTORS_PER_SLOT;
    sector += NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);

    data = locations[sectorId].data;
    size = locations[sectorId].size;

    for (i = 0; i < SECTOR_SIZE; i++)
        ((u8 *)gReadWriteSector)[i] = 0;

    gReadWriteSector->id = sectorId;
    gReadWriteSector->signature = SECTOR_SIGNATURE;
    gReadWriteSector->counter = gSaveCounter;

    for (i = 0; i < size; i++)
        gReadWriteSector->data[i] = data[i];

    CopyFromSaveBlock3(sectorId, gReadWriteSector);

    gReadWriteSector->checksum = CalculateChecksum(data, size);

    return TryWriteSector(sector, gReadWriteSector->data);
}

static u8 HandleWriteSectorNBytes(u8 sectorId, u8 *data, u16 size)
{
    u16 i;
    struct SaveSector *sector = &gSaveDataBuffer;

    for (i = 0; i < SECTOR_SIZE; i++)
        ((u8 *)sector)[i] = 0;

    sector->signature = SECTOR_SIGNATURE;

    for (i = 0; i < size; i++)
        sector->data[i] = data[i];

    sector->id = CalculateChecksum(data, size);
    return TryWriteSector(sectorId, sector->data);
}

static u8 TryWriteSector(u8 sector, u8 *data)
{
#ifdef PORTABLE
    if (ProgramFlashSector_DUMMY(sector, data))
#else
    if (ProgramFlashSectorAndVerify(sector, data))
#endif
    {
        SetDamagedSectorBits(ENABLE, sector);
        return SAVE_STATUS_ERROR;
    }
    else
    {
        SetDamagedSectorBits(DISABLE, sector);
        return SAVE_STATUS_OK;
    }
}

static u32 RestoreSaveBackupVarsAndIncrement(const struct SaveSectorLocation *locations)
{
    gReadWriteSector = &gSaveDataBuffer;
    gLastKnownGoodSector = gLastWrittenSector;
    gLastSaveCounter = gSaveCounter;
    gLastWrittenSector++;
    gLastWrittenSector %= NUM_SECTORS_PER_SLOT;
    gSaveCounter++;
    gIncrementalSectorId = 0;
    gDamagedSaveSectors = 0;
    return 0;
}

static u32 RestoreSaveBackupVars(const struct SaveSectorLocation *locations)
{
    gReadWriteSector = &gSaveDataBuffer;
    gLastKnownGoodSector = gLastWrittenSector;
    gLastSaveCounter = gSaveCounter;
    gIncrementalSectorId = 0;
    gDamagedSaveSectors = 0;
    return 0;
}

static u8 HandleWriteIncrementalSector(u16 numSectors, const struct SaveSectorLocation *locations)
{
    u8 status;

    if (gIncrementalSectorId < numSectors - 1)
    {
        status = SAVE_STATUS_OK;
        HandleWriteSector(gIncrementalSectorId, locations);
        gIncrementalSectorId++;
        if (gDamagedSaveSectors)
        {
            status = SAVE_STATUS_ERROR;
            gLastWrittenSector = gLastKnownGoodSector;
            gSaveCounter = gLastSaveCounter;
        }
    }
    else
    {
        status = SAVE_STATUS_ERROR;
    }

    return status;
}

static u8 HandleReplaceSectorAndVerify(u16 sectorId, const struct SaveSectorLocation *locations)
{
    u8 status = SAVE_STATUS_OK;

    HandleReplaceSector(sectorId - 1, locations);

    if (gDamagedSaveSectors)
    {
        status = SAVE_STATUS_ERROR;
        gLastWrittenSector = gLastKnownGoodSector;
        gSaveCounter = gLastSaveCounter;
    }
    return status;
}

static u8 HandleReplaceSector(u16 sectorId, const struct SaveSectorLocation *locations)
{
    u16 i;
    u16 sector;
    u8 *data;
    u16 size;
    u8 status;

    sector = sectorId + gLastWrittenSector;
    sector %= NUM_SECTORS_PER_SLOT;
    sector += NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);

    data = locations[sectorId].data;
    size = locations[sectorId].size;

    for (i = 0; i < SECTOR_SIZE; i++)
        ((u8 *)gReadWriteSector)[i] = 0;

    gReadWriteSector->id = sectorId;
    gReadWriteSector->signature = SECTOR_SIGNATURE;
    gReadWriteSector->counter = gSaveCounter;

    for (i = 0; i < size; i++)
        gReadWriteSector->data[i] = data[i];

    CopyFromSaveBlock3(sectorId, gReadWriteSector);

    gReadWriteSector->checksum = CalculateChecksum(data, size);

    EraseFlashSector(sector);

    status = SAVE_STATUS_OK;

    for (i = 0; i < SECTOR_SIGNATURE_OFFSET; i++)
    {
        if (ProgramFlashByte(sector, i, ((u8 *)gReadWriteSector)[i]))
        {
            status = SAVE_STATUS_ERROR;
            break;
        }
    }

    if (status == SAVE_STATUS_ERROR)
    {
        SetDamagedSectorBits(ENABLE, sector);
        return SAVE_STATUS_ERROR;
    }
    else
    {
        status = SAVE_STATUS_OK;

        for (i = 0; i < SECTOR_SIZE - (SECTOR_SIGNATURE_OFFSET + 1); i++)
        {
            if (ProgramFlashByte(sector, SECTOR_SIGNATURE_OFFSET + 1 + i, ((u8 *)gReadWriteSector)[SECTOR_SIGNATURE_OFFSET + 1 + i]))
            {
                status = SAVE_STATUS_ERROR;
                break;
            }
        }

        if (status == SAVE_STATUS_ERROR)
        {
            SetDamagedSectorBits(ENABLE, sector);
            return SAVE_STATUS_ERROR;
        }
        else
        {
            SetDamagedSectorBits(DISABLE, sector);
            return SAVE_STATUS_OK;
        }
    }
}

static u8 WriteSectorSignatureByte_NoOffset(u16 sectorId, const struct SaveSectorLocation *locations)
{
    u16 sector = sectorId + gLastWrittenSector;
    sector %= NUM_SECTORS_PER_SLOT;
    sector += NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);

    if (ProgramFlashByte(sector, SECTOR_SIGNATURE_OFFSET, SECTOR_SIGNATURE & 0xFF))
    {
        SetDamagedSectorBits(ENABLE, sector);
        gLastWrittenSector = gLastKnownGoodSector;
        gSaveCounter = gLastSaveCounter;
        return SAVE_STATUS_ERROR;
    }
    else
    {
        SetDamagedSectorBits(DISABLE, sector);
        return SAVE_STATUS_OK;
    }
}

static u8 CopySectorSignatureByte(u16 sectorId, const struct SaveSectorLocation *locations)
{
    u16 sector = sectorId + gLastWrittenSector - 1;
    sector %= NUM_SECTORS_PER_SLOT;
    sector += NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);

    if (ProgramFlashByte(sector, SECTOR_SIGNATURE_OFFSET, ((u8 *)gReadWriteSector)[SECTOR_SIGNATURE_OFFSET]))
    {
        SetDamagedSectorBits(ENABLE, sector);
        gLastWrittenSector = gLastKnownGoodSector;
        gSaveCounter = gLastSaveCounter;
        return SAVE_STATUS_ERROR;
    }
    else
    {
        SetDamagedSectorBits(DISABLE, sector);
        return SAVE_STATUS_OK;
    }
}

static u8 WriteSectorSignatureByte(u16 sectorId, const struct SaveSectorLocation *locations)
{
    u16 sector = sectorId + gLastWrittenSector - 1;
    sector %= NUM_SECTORS_PER_SLOT;
    sector += NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);

    if (ProgramFlashByte(sector, SECTOR_SIGNATURE_OFFSET, SECTOR_SIGNATURE & 0xFF))
    {
        SetDamagedSectorBits(ENABLE, sector);
        gLastWrittenSector = gLastKnownGoodSector;
        gSaveCounter = gLastSaveCounter;
        return SAVE_STATUS_ERROR;
    }
    else
    {
        SetDamagedSectorBits(DISABLE, sector);
        return SAVE_STATUS_OK;
    }
}

static u8 TryLoadSaveSlot(u16 sectorId, struct SaveSectorLocation *locations)
{
    u8 status;
    gReadWriteSector = &gSaveDataBuffer;
    if (sectorId != FULL_SAVE_SLOT)
    {
        status = SAVE_STATUS_ERROR;
    }
    else
    {
        status = GetSaveValidStatus(locations);
        CopySaveSlotData(FULL_SAVE_SLOT, locations);
    }

    return status;
}

static u8 CopySaveSlotData(u16 sectorId, struct SaveSectorLocation *locations)
{
    u16 i;
    u16 checksum;
    u16 slotOffset = NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);
    u16 id;

    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
    {
        ReadFlashSector(i + slotOffset, gReadWriteSector);

        id = gReadWriteSector->id;
        if (id == 0)
            gLastWrittenSector = i;

        checksum = CalculateChecksum(gReadWriteSector->data, locations[id].size);

        if (gReadWriteSector->signature == SECTOR_SIGNATURE && gReadWriteSector->checksum == checksum)
        {
            u16 j;
            for (j = 0; j < locations[id].size; j++)
                ((u8 *)locations[id].data)[j] = gReadWriteSector->data[j];
            CopyToSaveBlock3(id, gReadWriteSector);
        }
    }

    return SAVE_STATUS_OK;
}

static u8 GetSaveValidStatus(const struct SaveSectorLocation *locations)
{
    u16 i;
    u16 checksum;
    u32 saveSlot1Counter = 0;
    u32 saveSlot2Counter = 0;
    u32 validSectorFlags = 0;
    bool8 signatureValid = FALSE;
    u8 saveSlot1Status;
    u8 saveSlot2Status;

    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
    {
        ReadFlashSector(i, gReadWriteSector);
        if (gReadWriteSector->signature == SECTOR_SIGNATURE)
        {
            signatureValid = TRUE;
            checksum = CalculateChecksum(gReadWriteSector->data, locations[gReadWriteSector->id].size);
            if (gReadWriteSector->checksum == checksum)
            {
                saveSlot1Counter = gReadWriteSector->counter;
                validSectorFlags |= 1 << gReadWriteSector->id;
            }
        }
    }

    if (signatureValid)
    {
        if (validSectorFlags == (1 << NUM_SECTORS_PER_SLOT) - 1)
            saveSlot1Status = SAVE_STATUS_OK;
        else
            saveSlot1Status = SAVE_STATUS_ERROR;
    }
    else
    {
        saveSlot1Status = SAVE_STATUS_EMPTY;
    }

    validSectorFlags = 0;
    signatureValid = FALSE;

    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
    {
        ReadFlashSector(i + NUM_SECTORS_PER_SLOT, gReadWriteSector);
        if (gReadWriteSector->signature == SECTOR_SIGNATURE)
        {
            signatureValid = TRUE;
            checksum = CalculateChecksum(gReadWriteSector->data, locations[gReadWriteSector->id].size);
            if (gReadWriteSector->checksum == checksum)
            {
                saveSlot2Counter = gReadWriteSector->counter;
                validSectorFlags |= 1 << gReadWriteSector->id;
            }
        }
    }

    if (signatureValid)
    {
        if (validSectorFlags == (1 << NUM_SECTORS_PER_SLOT) - 1)
            saveSlot2Status = SAVE_STATUS_OK;
        else
            saveSlot2Status = SAVE_STATUS_ERROR;
    }
    else
    {
        saveSlot2Status = SAVE_STATUS_EMPTY;
    }

    if (saveSlot1Status == SAVE_STATUS_OK && saveSlot2Status == SAVE_STATUS_OK)
    {
        if ((saveSlot1Counter == -1 && saveSlot2Counter ==  0)
         || (saveSlot1Counter ==  0 && saveSlot2Counter == -1))
        {
            if ((unsigned)(saveSlot1Counter + 1) < (unsigned)(saveSlot2Counter + 1))
                gSaveCounter = saveSlot2Counter;
            else
                gSaveCounter = saveSlot1Counter;
        }
        else
        {
            if (saveSlot1Counter < saveSlot2Counter)
                gSaveCounter = saveSlot2Counter;
            else
                gSaveCounter = saveSlot1Counter;
        }
        return SAVE_STATUS_OK;
    }

    if (saveSlot1Status == SAVE_STATUS_OK)
    {
        gSaveCounter = saveSlot1Counter;
        if (saveSlot2Status == SAVE_STATUS_ERROR)
            return SAVE_STATUS_ERROR;
        return SAVE_STATUS_OK;
    }

    if (saveSlot2Status == SAVE_STATUS_OK)
    {
        gSaveCounter = saveSlot2Counter;
        if (saveSlot1Status == SAVE_STATUS_ERROR)
            return SAVE_STATUS_ERROR;
        return SAVE_STATUS_OK;
    }

    if (saveSlot1Status == SAVE_STATUS_EMPTY
     && saveSlot2Status == SAVE_STATUS_EMPTY)
    {
        gSaveCounter = 0;
        gLastWrittenSector = 0;
        return SAVE_STATUS_EMPTY;
    }

    gSaveCounter = 0;
    gLastWrittenSector = 0;
    return SAVE_STATUS_CORRUPT;
}

static u8 TryLoadSaveSector(u8 sectorId, u8 *data, u16 size)
{
    u16 i;
    struct SaveSector *sector = &gSaveDataBuffer;
    ReadFlashSector(sectorId, sector);
    if (sector->signature == SECTOR_SIGNATURE)
    {
        u16 checksum = CalculateChecksum(sector->data, size);
        if (sector->id == checksum)
        {
            for (i = 0; i < size; i++)
                data[i] = sector->data[i];
            return SAVE_STATUS_OK;
        }
        else
        {
            return SAVE_STATUS_CORRUPT;
        }
    }
    else
    {
        return SAVE_STATUS_EMPTY;
    }
}

static bool8 ReadFlashSector(u8 sectorId, struct SaveSector *sector)
{
    ReadFlash(sectorId, 0, sector->data, SECTOR_SIZE);
    return TRUE;
}

static u16 CalculateChecksum(void *data, u16 size)
{
    u16 i;
    u32 checksum = 0;

    for (i = 0; i < (size / 4); i++)
    {
        checksum += *((u32 *)data);
        data += sizeof(u32);
    }

    return ((checksum >> 16) + checksum);
}

static void UpdateSaveAddresses(void)
{
    int i = SECTOR_ID_SAVEBLOCK2;
    gRamSaveSectorLocations[i].data = (void *)(gSaveBlock2Ptr) + sSaveSlotLayout[i].offset;
    gRamSaveSectorLocations[i].size = sSaveSlotLayout[i].size;

    for (i = SECTOR_ID_SAVEBLOCK1_START; i <= SECTOR_ID_SAVEBLOCK1_END; i++)
    {
        gRamSaveSectorLocations[i].data = (void *)(gSaveBlock1Ptr) + sSaveSlotLayout[i].offset;
        gRamSaveSectorLocations[i].size = sSaveSlotLayout[i].size;
    }

    for (; i <= SECTOR_ID_PKMN_STORAGE_END; i++)
    {
        gRamSaveSectorLocations[i].data = (void *)(gPokemonStoragePtr) + sSaveSlotLayout[i].offset;
        gRamSaveSectorLocations[i].size = sSaveSlotLayout[i].size;
    }
}

u8 HandleSavingData(u8 saveType)
{
    u8 i;
    u32 *backupVar = gTrainerHillVBlankCounter;

    gTrainerHillVBlankCounter = NULL;
    UpdateSaveAddresses();
    switch (saveType)
    {
    case SAVE_HALL_OF_FAME_ERASE_BEFORE:
        for (i = SECTOR_ID_HOF_1; i < SECTORS_COUNT; i++)
            EraseFlashSector(i);
    case SAVE_HALL_OF_FAME:
        if (GetGameStat(GAME_STAT_ENTERED_HOF) < 999)
            IncrementGameStat(GAME_STAT_ENTERED_HOF);

        CopyPartyAndObjectsToSave();
        SaveModernSave(saveType);
        break;
    case SAVE_NORMAL:
    default:
        CopyPartyAndObjectsToSave();
        SaveModernSave(saveType);
        break;
    case SAVE_LINK:
    case SAVE_EREADER:
        CopyPartyAndObjectsToSave();
        SaveModernSave(saveType);
        break;
    case SAVE_OVERWRITE_DIFFERENT_FILE:
        for (i = SECTOR_ID_HOF_1; i < SECTORS_COUNT; i++)
            EraseFlashSector(i);

        CopyPartyAndObjectsToSave();
        SaveModernSave(saveType);
        break;
    }

    gTrainerHillVBlankCounter = backupVar;
    return 0;
}

u8 TrySavingData(u8 saveType)
{
    CopyPartyAndObjectsToSave();
    u8 status = SaveModernSave(saveType);
    if (status == SAVE_STATUS_OK)
    {
        gSaveAttemptStatus = SAVE_STATUS_OK;
        return SAVE_STATUS_OK;
    }
    else
    {
        DoSaveFailedScreen(saveType);
        gSaveAttemptStatus = SAVE_STATUS_ERROR;
        return SAVE_STATUS_ERROR;
    }
}

bool8 LinkFullSave_Init(void)
{
    UpdateSaveAddresses();
    CopyPartyAndObjectsToSave();
    RestoreSaveBackupVarsAndIncrement(gRamSaveSectorLocations);
    return FALSE;
}

bool8 LinkFullSave_WriteSector(void)
{
    SaveModernSave(SAVE_NORMAL);
    return TRUE;
}

bool8 LinkFullSave_ReplaceLastSector(void)
{
    SaveModernSave(SAVE_NORMAL);
    return FALSE;
}

bool8 LinkFullSave_SetLastSectorSignature(void)
{
    return FALSE;
}

bool8 WriteSaveBlock2(void)
{
    UpdateSaveAddresses();
    CopyPartyAndObjectsToSave();
    SaveModernSave(SAVE_NORMAL);
    return FALSE;
}

bool8 WriteSaveBlock1Sector(void)
{
    SaveModernSave(SAVE_NORMAL);
    return TRUE;
}

u8 LoadGameSave(u8 saveType)
{
    u8 status;
    UpdateSaveAddresses();

    switch (saveType)
    {
    case SAVE_NORMAL:
    default:
        status = LoadModernSave(saveType);
        if (status == SAVE_STATUS_CORRUPT || status == SAVE_STATUS_EMPTY)
        {
            if (gFlashMemoryPresent == TRUE)
                status = TryLoadSaveSlot(FULL_SAVE_SLOT, gRamSaveSectorLocations);
            else
                status = SAVE_STATUS_EMPTY;
        }
        CopyPartyAndObjectsFromSave();
        gSaveFileStatus = status;
        gGameContinueCallback = NULL;
        break;
    case SAVE_HALL_OF_FAME:
        status = LoadModernSave(saveType);
        if (status == SAVE_STATUS_CORRUPT || status == SAVE_STATUS_EMPTY)
        {
            if (gFlashMemoryPresent == TRUE && gHoFSaveBuffer != NULL)
            {
                u8 *hofData = (u8 *) gHoFSaveBuffer;
                status = TryLoadSaveSector(SECTOR_ID_HOF_1, hofData, SECTOR_DATA_SIZE);
                if (status == SAVE_STATUS_OK)
                    status = TryLoadSaveSector(SECTOR_ID_HOF_2, &hofData[SECTOR_DATA_SIZE], SECTOR_DATA_SIZE);
            }
            else
            {
                status = SAVE_STATUS_ERROR;
            }
        }
        break;
    }

    return status;
}

u16 GetSaveBlocksPointersBaseOffset(void)
{
    FILE *f = fopen(gSavePath, "rb");
    if (f == NULL)
        return 0;

    struct ModernSaveHeader header;
    if (fread(&header, sizeof(header), 1, f) != 1 || header.magic != MODERN_SAVE_MAGIC)
    {
        fclose(f);
        if (gFlashMemoryPresent != TRUE)
            return 0;
        u16 i, slotOffset;
        struct SaveSector *sector = gReadWriteSector = &gSaveDataBuffer;
        UpdateSaveAddresses();
        GetSaveValidStatus(gRamSaveSectorLocations);
        slotOffset = NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);
        for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
        {
            ReadFlashSector(i + slotOffset, gReadWriteSector);
            if (gReadWriteSector->id == SECTOR_ID_SAVEBLOCK2)
                return sector->data[offsetof(struct SaveBlock2, playerTrainerId[0])] +
                       sector->data[offsetof(struct SaveBlock2, playerTrainerId[1])] +
                       sector->data[offsetof(struct SaveBlock2, playerTrainerId[2])] +
                       sector->data[offsetof(struct SaveBlock2, playerTrainerId[3])];
        }
        return 0;
    }

    u16 offset = 0;
    u32 i;
    for (i = 0; i < header.numBlocks; i++)
    {
        struct ModernSaveBlockHeader blockHeader;
        if (fread(&blockHeader, sizeof(blockHeader), 1, f) != 1)
            break;

        if (blockHeader.blockId == BLOCK_ID_SAVEBLOCK2)
        {
            struct SaveBlock2 sb2;
            u32 readSize = min(blockHeader.size, (u32)sizeof(struct SaveBlock2));
            fread(&sb2, readSize, 1, f);
            offset = sb2.playerTrainerId[0] + sb2.playerTrainerId[1] + sb2.playerTrainerId[2] + sb2.playerTrainerId[3];
            break;
        }
        else
        {
            fseek(f, blockHeader.size, SEEK_CUR);
        }
    }

    fclose(f);
    return offset;
}

u32 TryReadSpecialSaveSector(u8 sector, u8 *dst)
{
    if (sector != SECTOR_ID_TRAINER_HILL && sector != SECTOR_ID_RECORDED_BATTLE)
        return SAVE_STATUS_ERROR;

    FILE *f = fopen(gSavePath, "rb");
    if (f == NULL)
        return SAVE_STATUS_ERROR;

    struct ModernSaveHeader header;
    if (fread(&header, sizeof(header), 1, f) != 1 || header.magic != MODERN_SAVE_MAGIC)
    {
        fclose(f);
        return SAVE_STATUS_ERROR;
    }

    u32 targetBlockId = (sector == SECTOR_ID_TRAINER_HILL) ? BLOCK_ID_TRAINERIAL : BLOCK_ID_RECBATTLE;
    u8 status = SAVE_STATUS_ERROR;

    u32 i;
    for (i = 0; i < header.numBlocks; i++)
    {
        struct ModernSaveBlockHeader blockHeader;
        if (fread(&blockHeader, sizeof(blockHeader), 1, f) != 1)
            break;

        if (blockHeader.blockId == targetBlockId)
        {
            u8 *tempBuf = Alloc(blockHeader.size);
            if (tempBuf)
            {
                fread(tempBuf, blockHeader.size, 1, f);
                if (*(u32 *)tempBuf == SPECIAL_SECTOR_SENTINEL)
                {
                    u32 copySize = min(blockHeader.size - 4, (u32)(SECTOR_COUNTER_OFFSET - 1));
                    memcpy(dst, tempBuf + 4, copySize);
                    status = SAVE_STATUS_OK;
                }
                Free(tempBuf);
            }
            break;
        }
        else
        {
            fseek(f, blockHeader.size, SEEK_CUR);
        }
    }

    fclose(f);
    return status;
}

u32 TryWriteSpecialSaveSector(u8 sector, u8 *src)
{
    if (sector != SECTOR_ID_TRAINER_HILL && sector != SECTOR_ID_RECORDED_BATTLE)
        return SAVE_STATUS_ERROR;

    u32 targetBlockId = (sector == SECTOR_ID_TRAINER_HILL) ? BLOCK_ID_TRAINERIAL : BLOCK_ID_RECBATTLE;

    u32 size = 4 + (SECTOR_COUNTER_OFFSET - 1);
    u8 *tempBuf = Alloc(size);
    if (tempBuf == NULL)
        return SAVE_STATUS_ERROR;

    *(u32 *)tempBuf = SPECIAL_SECTOR_SENTINEL;
    memcpy(tempBuf + 4, src, SECTOR_COUNTER_OFFSET - 1);

    u32 status = WriteSpecialBlock(targetBlockId, tempBuf, size);
    Free(tempBuf);

    return status;
}

#define tState         data[0]
#define tTimer         data[1]
#define tInBattleTower data[2]

void Task_LinkFullSave(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        gSoftResetDisabled = TRUE;
        tState = 1;
        break;
    case 1:
        SetLinkStandbyCallback();
        tState = 2;
        break;
    case 2:
        if (IsLinkTaskFinished())
        {
            if (!tInBattleTower)
                SaveMapView();
            tState = 3;
        }
        break;
    case 3:
        if (!tInBattleTower)
            SetContinueGameWarpStatusToDynamicWarp();
        LinkFullSave_Init();
        tState = 4;
        break;
    case 4:
        if (++tTimer == 5)
        {
            tTimer = 0;
            tState = 5;
        }
        break;
    case 5:
        if (LinkFullSave_WriteSector())
            tState = 6;
        else
            tState = 4;
        break;
    case 6:
        LinkFullSave_ReplaceLastSector();
        tState = 7;
        break;
    case 7:
        if (!tInBattleTower)
            ClearContinueGameWarpStatus2();
        SetLinkStandbyCallback();
        tState = 8;
        break;
    case 8:
        if (IsLinkTaskFinished())
        {
            LinkFullSave_SetLastSectorSignature();
            tState = 9;
        }
        break;
    case 9:
        SetLinkStandbyCallback();
        tState = 10;
        break;
    case 10:
        if (IsLinkTaskFinished())
            tState++;
        break;
    case 11:
        if (++tTimer > 5)
        {
            gSoftResetDisabled = FALSE;
            DestroyTask(taskId);
        }
        break;
    }
}

static u32 SaveBlock3Size(u32 sectorId)
{
    s32 begin = sectorId * SAVE_BLOCK_3_CHUNK_SIZE;
    s32 end = (sectorId + 1) * SAVE_BLOCK_3_CHUNK_SIZE;
    return max(0, min(end, (s32)sizeof(gSaveblock3)) - begin);
}

static void CopyToSaveBlock3(u32 sectorId, struct SaveSector *sector)
{
    u32 size = SaveBlock3Size(sectorId);
    memcpy((u8 *)&gSaveblock3 + (sectorId * SAVE_BLOCK_3_CHUNK_SIZE), sector->saveBlock3Chunk, size);
}

static void CopyFromSaveBlock3(u32 sectorId, struct SaveSector *sector)
{
    u32 size = SaveBlock3Size(sectorId);
    memcpy(sector->saveBlock3Chunk, (u8 *)&gSaveblock3 + (sectorId * SAVE_BLOCK_3_CHUNK_SIZE), size);
}