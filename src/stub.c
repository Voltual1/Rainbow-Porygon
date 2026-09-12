// MIT License
// 
// Copyright (c) 2026 pokeemerald-multiplatform contributors
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of the original multiplatform-port modifications contributed through this
// fork (the "Port Modifications"), to deal in the Port Modifications without
// restriction, including without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell copies of the Port
// Modifications, and to permit persons to whom the Port Modifications are
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Port Modifications.
// 
// THE PORT MODIFICATIONS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
// EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE PORT MODIFICATIONS OR THE USE OR OTHER
// DEALINGS IN THE PORT MODIFICATIONS.
// 
// Scope
// -----
// 
// This license applies only to original multiplatform-port modifications made by
// contributors to this fork. It does not grant rights to, or relicense:
// 
// - The upstream pokeemerald decompilation or contributions from its authors.
// - Pokemon Emerald, Pokemon characters, names, graphics, audio, story, or other
//   copyrighted or trademarked material owned by Nintendo, Creatures Inc., GAME
//   FREAK inc., or other respective owners.
// - Third-party software included in this repository, which remains subject to
//   its own license terms.
// 
// Users are responsible for determining which portions of a distribution are
// covered by this license and for complying with all applicable third-party and
// upstream terms.
#include "global.h"
#include "main.h"
#include "gba/m4a_internal.h"
#include "libgcnmultiboot.h"
#include <stdio.h>
#include <string.h>

#undef RegisterRamReset
#undef IntrMain

void RegisterRamReset(u32 resetFlags) { puts("RegisterRamReset stub"); }
void IntrMain(void) { puts("IntrMain stub"); }

extern void CB2_InitCopyrightScreenAfterBootup(void);
void gInitialMainCB2(void)
{
    CB2_InitCopyrightScreenAfterBootup();
}

const u8 RomHeaderGameCode[4] = "BPEE";
const u8 RomHeaderSoftwareVersion = 0;

void ReInitializeEWRAM(void) {}
u8 ProgramFlashSector_DUMMY(u16 sectorNum, u8 *src) { return 0; }

u32 umul3232H32(u32 a, u32 b) { return (u32)(((u64)a * b) >> 32); }

extern const u8 gClockTable[];
extern const MPlayFunc gMPlayJumpTableTemplate[36];

void RealClearChain(void *x)
{
    // TODO: 64-bit Audio Stub
}

void TrackStop(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ChnVolSetAsm(struct MusicPlayerTrack *track, struct SoundChannel *chan)
{
    // TODO: 64-bit Audio Stub
}

void ply_fine(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_goto(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_patt(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_pend(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_rept(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_prio(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_tempo(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_keysh(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_voice(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_vol(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_pan(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_bend(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_bendr(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_lfos(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_lfodl(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_mod(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_modt(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_tune(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_port(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_endtie(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void ply_note(u32 note_cmd, struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit Audio Stub
}

void MPlayMain(struct MusicPlayerInfo *mplayInfo)
{
    // TODO: 64-bit Audio Stub - 完全禁用 64 位 MIDI 音乐解析循环
}

void SoundMain(void)
{
    // TODO: 64-bit Audio Stub
}

void SoundMainBTM(void) {}

void MPlayJumpTableCopy(MPlayFunc *mplayJumpTable) {
    if (mplayJumpTable == NULL) return;
    memcpy(mplayJumpTable, gMPlayJumpTableTemplate, 36 * sizeof(MPlayFunc));
}

static u8 SoundMainRAM_pad_before[0x4000] ALIGNED(8); 
char SoundMainRAM[0x40000] ALIGNED(8);                 
static u8 SoundMainRAM_pad_after[0x4000] ALIGNED(8);  

static u8 gMaxLines_pad_before[0x1000] ALIGNED(8);
char gMaxLines[0x100] ALIGNED(8);
static u8 gMaxLines_pad_after[0x1000] ALIGNED(8);

static u8 gNumMusicPlayers_pad_before[0x1000] ALIGNED(8);
char gNumMusicPlayers[0x100] ALIGNED(8);
static u8 gNumMusicPlayers_pad_after[0x1000] ALIGNED(8);

s32 Div(s32 num, s32 denom) { return denom != 0 ? num / denom : 0; }

void FastUnsafeCopy32(void *dst, const void *src, u32 size) {
    if (dst != NULL && src != NULL && size > 0) {
        memcpy(dst, src, size);
    }
}

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
