#include "global.h"
#include "main.h"
#include "gba/m4a_internal.h"
#include "libgcnmultiboot.h"
#include <stdio.h>

#define STUB_FUNC(func) func { puts("function \"" #func "\" is a stub"); }
#define STUB_FUNC_BLOCK(func, block) func { puts("function \"" #func "\" is a stub"); block }
#define STUB_FUNC_QUIET(func) func {}
#define STUB_FUNC_QUIET_BLOCK(func, block) func { block }

// 解决 GBA 硬件宏和内联函数的干扰
#undef RegisterRamReset
#undef IntrMain

// --- 1. 底层系统存根 ---
void RegisterRamReset(u32 resetFlags) { puts("RegisterRamReset stub"); }
void IntrMain(void) { puts("IntrMain stub"); }
void *gInitialMainCB2 = NULL;
const u8 RomHeaderGameCode[4] = "BPEE";
const u8 RomHeaderSoftwareVersion = 0;

// --- 2. 内存与 Flash 管理 ---
void ReInitializeEWRAM(void) {}
u8 ProgramFlashSector_DUMMY(u16 sectorNum, u8 *src) { return 0; }

// --- 3. GameCube 联动存根 ---
void GameCubeMultiBoot_Hash(void) {}
void GameCubeMultiBoot_Main(struct GcmbStruct *pStruct) {}
void GameCubeMultiBoot_ExecuteProgram(struct GcmbStruct *pStruct) {}
void GameCubeMultiBoot_Init(struct GcmbStruct *pStruct) {}
void GameCubeMultiBoot_HandleSerialInterrupt(struct GcmbStruct *pStruct) {}
void GameCubeMultiBoot_Quit(void) {}

// --- 4. m4a 音频引擎存根 ---
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
void ply_keysh(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t) {}
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

// --- 5. 符号标签 (匹配 extern char[]) ---
char SoundMainRAM[1];
char gMaxLines[1];
char gNumMusicPlayers[1];

// --- 6. GBA BIOS 数学与解压 ---
s32 Div(s32 num, s32 denom) { return denom != 0 ? num / denom : 0; }
void BitUnPack(const void *src, void *dst, const void *data) {}
void FastUnsafeCopy32(const void *src, void *dst, u32 size) {}
void LZ77UnCompWRAMOptimized(const u32 *src, void *dst) {}

// --- 7. 符号边界 ---
u8 LZ77UnCompWRAMOptimized_end[1];
u8 __iwram_end[1];
