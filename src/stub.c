#include "global.h"
#include "main.h"
#include "gba/m4a_internal.h"
#include "libgcnmultiboot.h"
#include <stdio.h>
#include <string.h>

// 彻底清除宏定义的物理函数
#undef RegisterRamReset
#undef IntrMain
#undef ply_keysh

// --- 1. 底层系统 ---
void RegisterRamReset(u32 resetFlags) { puts("RegisterRamReset stub"); }
void IntrMain(void) { puts("IntrMain stub"); }

// 修复 ACCERR 崩溃：gInitialMainCB2 必须是真正的函数实体，以匹配主引擎的 extern void 声明
extern void CB2_InitCopyrightScreenAfterBootup(void);
void gInitialMainCB2(void)
{
    CB2_InitCopyrightScreenAfterBootup();
}

const u8 RomHeaderGameCode[4] = "BPEE";
const u8 RomHeaderSoftwareVersion = 0;

// --- 2. 内存与 Flash ---
void ReInitializeEWRAM(void) {}
u8 ProgramFlashSector_DUMMY(u16 sectorNum, u8 *src) { return 0; }

// --- 3. 音频 ---
u32 umul3232H32(u32 a, u32 b) { return (u32)(((u64)a * b) >> 32); }
void SoundMain(void) {}
void SoundMainBTM(void) {}
void RealClearChain(void *x) {}
void MPlayJumpTableCopy(MPlayFunc *mplayJumpTable) {}
void MPlayMain(struct MusicPlayerInfo *mplayInfo) {}
void TrackStop(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ChnVolSetAsm(void) {}
void ply_note(u32 note_cmd, struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_fine(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_goto(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_patt(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_pend(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_rept(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_prio(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_tempo(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_keysh(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) { puts("ply_keysh stub"); }
void ply_voice(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_vol(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_pan(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_bend(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_bendr(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_lfos(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_lfodl(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_mod(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_modt(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_tune(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_port(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
void ply_endtie(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}

// --- 4. 其它 ---
// 保持安全的缓冲区大小，防止越界写破坏 BSS 全局段
char SoundMainRAM[0x8000]; // 32KB
char gMaxLines[0x400];
char gNumMusicPlayers[0x400];
s32 Div(s32 num, s32 denom) { return denom != 0 ? num / denom : 0; }

// 确保物理快速拷贝执行
void FastUnsafeCopy32(void *dst, const void *src, u32 size) {
    if (dst != NULL && src != NULL && size > 0) {
        memcpy(dst, src, size);
    }
}

// 纯 C 语言实现的 LZ77UnCompWRAMOptimized
void LZ77UnCompWRAMOptimized(const u32 *src, void *dst) {
    if (src == NULL || dst == NULL) return;

    const u8 *src8 = (const u8 *)src;
    u8 *dst8 = (u8 *)dst;
    
    u32 header = src[0];
    u32 destSize = header >> 8; 
    
    src8 += 4; 
    
    u32 bytesWritten = 0;
    while (bytesWritten < destSize) {
        u8 flags = *src8++;
        for (int i = 0; i < 8; i++) {
            if (bytesWritten >= destSize) {
                break;
            }
            
            if (flags & (0x80 >> i)) { 
                u8 byte1 = *src8++;
                u8 byte2 = *src8++;
                
                u32 length = (byte1 >> 4) + 3;
                u32 disp = ((byte1 & 0x0F) << 8) | byte2;
                
                u8 *copySrc = dst8 - disp - 1;
                for (u32 j = 0; j < length; j++) {
                    *dst8++ = *copySrc++;
                    bytesWritten++;
                }
            } else {
                *dst8++ = *src8++;
                bytesWritten++;
            }
        }
    }
}

// 实现位合并转换 BitUnPack BIOS 函数
struct BitUnPackConfig {
    u16 srcLen;        
    u8 srcBitLen;      
    u8 dstBitLen;      
    u32 dataOffset;    
};

void BitUnPack(const void *src, void *dst, const void *data) {
    if (src == NULL || dst == NULL || data == NULL) return;

    const struct BitUnPackConfig *config = (const struct BitUnPackConfig *)data;
    const u8 *src8 = (const u8 *)src;
    u8 *dst8 = (u8 *)dst;
    
    u32 srcBitLen = config->srcBitLen;
    u32 dstBitLen = config->dstBitLen;
    u32 offset = config->dataOffset & 0x7FFFFFFF;
    bool8 zeroOffset = (config->dataOffset & 0x80000000) != 0;
    
    u32 totalDstBytes = (config->srcLen * 8 / srcBitLen) * dstBitLen / 8;
    memset(dst8, 0, totalDstBytes);
    
    u32 srcBitPos = 0;
    u32 dstBitPos = 0;
    u32 totalBits = config->srcLen * 8;
    
    while (srcBitPos < totalBits) {
        u32 rawVal = 0;
        for (u32 i = 0; i < srcBitLen; i++) {
            u32 bit = (src8[(srcBitPos + i) / 8] >> ((srcBitPos + i) % 8)) & 1;
            rawVal |= (bit << i);
        }
        srcBitPos += srcBitPos < totalBits ? srcBitLen : 0;
        
        if (rawVal != 0 || zeroOffset) {
            rawVal += offset;
        }
        
        for (u32 i = 0; i < dstBitLen; i++) {
            u32 bit = (rawVal >> i) & 1;
            u32 byteIdx = dstBitPos / 8;
            u32 bitIdx = dstBitPos % 8;
            if (bit) {
                dst8[byteIdx] |= (1 << bitIdx);
            }
            dstBitPos++;
        }
    }
}

u8 LZ77UnCompWRAMOptimized_end[0x100];
u8 __iwram_end[0x100];
void GameCubeMultiBoot_Hash(void) {}
void GameCubeMultiBoot_Main(struct GcmbStruct *pStruct) {}
void GameCubeMultiBoot_ExecuteProgram(struct GcmbStruct *pStruct) {}
void GameCubeMultiBoot_Init(struct GcmbStruct *pStruct) {}
void GameCubeMultiBoot_HandleSerialInterrupt(struct GcmbStruct *pStruct) {}
void GameCubeMultiBoot_Quit(void) {}