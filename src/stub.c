#include "global.h"
#include "main.h"
#include "gba/m4a_internal.h"
#include <stdio.h>

#define STUB_FUNC(func) func { puts("function \"" #func "\" is a stub"); }
#define STUB_FUNC_BLOCK(func, block) func { puts("function \"" #func "\" is a stub"); block }
#define STUB_FUNC_QUIET(func) func {}
#define STUB_FUNC_QUIET_BLOCK(func, block) func { block }

// 1. 多平台与链接管理 (补全返回值)
STUB_FUNC_QUIET_BLOCK(bool8 HandleLinkConnection(void), return 0;)
STUB_FUNC_QUIET(void Task_InitUnionRoom(void))
STUB_FUNC_BLOCK(int MultiBoot(struct MultiBootParam *mp), return 1;)
STUB_FUNC(void RegisterRamReset(u32 resetFlags))
STUB_FUNC(void IntrMain(void))
STUB_FUNC(void GameCubeMultiBoot_Hash(void))
STUB_FUNC_QUIET(void GameCubeMultiBoot_Main(struct GcmbStruct *pStruct))
STUB_FUNC(void GameCubeMultiBoot_ExecuteProgram(struct GcmbStruct *pStruct))
STUB_FUNC(void GameCubeMultiBoot_Init(struct GcmbStruct *pStruct))
STUB_FUNC(void GameCubeMultiBoot_HandleSerialInterrupt(struct GcmbStruct *pStruct))
STUB_FUNC(void GameCubeMultiBoot_Quit(void))
STUB_FUNC(void rfu_initializeAPI(void))
STUB_FUNC(void InitRFUAPI(void))

STUB_FUNC_BLOCK(u32 VerifyFlashSectorNBytes(u16 sectorNum, u8 *src, u32 n), return 0;)
STUB_FUNC_BLOCK(u32 VerifyFlashSector(u16 sectorNum, u8 *src), return 0;)

// 2. MultiBoot 存根 (补全返回值)
STUB_FUNC(void MultiBootInit(struct MultiBootParam *mp))
STUB_FUNC_BLOCK(int MultiBootMain(struct MultiBootParam *mp), return 1;)
STUB_FUNC(void MultiBootStartProbe(struct MultiBootParam *mp))
STUB_FUNC(void MultiBootStartMaster(struct MultiBootParam *mp, const u8 *srcp, int length, u8 palette_color, s8 palette_speed))
STUB_FUNC_BLOCK(int MultiBootCheckComplete(struct MultiBootParam *mp), return 1;)

// 3. m4a 音频函数 (修正函数签名)
STUB_FUNC_BLOCK(u32 umul3232H32(u32 a, u32 b), return (u32)(((u64)a * b) >> 32);)
STUB_FUNC(void SoundMain(void))
STUB_FUNC(void SoundMainBTM(void))
STUB_FUNC(void RealClearChain(void *x))
STUB_FUNC(void MPlayJumpTableCopy(MPlayFunc *mplayJumpTable))
STUB_FUNC(void MPlayMain(struct MusicPlayerInfo *mplayInfo))
STUB_FUNC(void TrackStop(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ChnVolSetAsm(void))
STUB_FUNC(void ply_note(u32 note_cmd, struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_fine(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_goto(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_patt(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_pend(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_rept(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_prio(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_tempo(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_keysh(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_voice(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_vol(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_pan(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_bend(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_bendr(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_lfos(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_lfodl(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_mod(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_modt(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_tune(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_port(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))
STUB_FUNC(void ply_endtie(struct MusicPlayerInfo *m, struct MusicPlayerTrack *t))

// 4. 解决类型冲突：头文件里是 char[]，我们不能定义成普通变量
char SoundMainRAM[1];
char gMaxLines[1];
char gNumMusicPlayers[1];

// 5. 其余 BIOS/数学存根
STUB_FUNC_BLOCK(s32 Div(s32 num, s32 denom), return denom != 0 ? num / denom : 0;)
STUB_FUNC(void BitUnPack(const void *src, void *dst, const void *data))
STUB_FUNC(void FastUnsafeCopy32(const void *src, void *dst, u32 size))
STUB_FUNC(void LZ77UnCompWRAMOptimized(const u32 *src, void *dst))

// 6. 特殊汇编标签符号
u8 LZ77UnCompWRAMOptimized_end[1];
u8 __iwram_end[1];
