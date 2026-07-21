#include "global.h"
#include "main.h"
#include "gba/m4a_internal.h"
#include "libgcnmultiboot.h"
#include <stdio.h>

#define STUB_FUNC(func) func { puts("function \"" #func "\" is a stub"); }
#define STUB_FUNC_BLOCK(func, block) func { puts("function \"" #func "\" is a stub"); block }
#define STUB_FUNC_QUIET(func) func {}
#define STUB_FUNC_QUIET_BLOCK(func, block) func { block }

// 1. 链接与连接管理
STUB_FUNC_QUIET_BLOCK(bool8 HandleLinkConnection(void), return 0;)
STUB_FUNC_QUIET(void Task_InitUnionRoom(void))
STUB_FUNC_BLOCK(int MultiBoot(struct MultiBootParam *mp), return 1;)
STUB_FUNC(void RegisterRamReset(u32 resetFlags))
STUB_FUNC(void IntrMain(void))

// 2. GameCube 联动存根 (已包含头文件，解决 GcmbStruct 报错)
STUB_FUNC(void GameCubeMultiBoot_Hash(void))
STUB_FUNC_QUIET(void GameCubeMultiBoot_Main(struct GcmbStruct *pStruct))
STUB_FUNC(void GameCubeMultiBoot_ExecuteProgram(struct GcmbStruct *pStruct))
STUB_FUNC(void GameCubeMultiBoot_Init(struct GcmbStruct *pStruct))
STUB_FUNC(void GameCubeMultiBoot_HandleSerialInterrupt(struct GcmbStruct *pStruct))
STUB_FUNC(void GameCubeMultiBoot_Quit(void))

// 3. RFU 与 Flash 存根
STUB_FUNC(void rfu_initializeAPI(void))
STUB_FUNC(void InitRFUAPI(void))
STUB_FUNC_BLOCK(u32 VerifyFlashSectorNBytes(u16 sectorNum, u8 *src, u32 n), return 0;)
STUB_FUNC_BLOCK(u32 VerifyFlashSector(u16 sectorNum, u8 *src), return 0;)

// 4. MultiBoot 流程
STUB_FUNC(void MultiBootInit(struct MultiBootParam *mp))
STUB_FUNC_BLOCK(int MultiBootMain(struct MultiBootParam *mp), return 1;)
STUB_FUNC(void MultiBootStartProbe(struct MultiBootParam *mp))
STUB_FUNC(void MultiBootStartMaster(struct MultiBootParam *mp, const u8 *srcp, int length, u8 palette_color, s8 palette_speed))
STUB_FUNC_BLOCK(int MultiBootCheckComplete(struct MultiBootParam *mp), return 1;)

// 5. m4a 音频函数 (修正签名与返回值)
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

// 6. 解决类型冲突 (char[] 匹配汇编标签)
char SoundMainRAM[1];
char gMaxLines[1];
char gNumMusicPlayers[1];

// 7. 数学与解压
STUB_FUNC_BLOCK(s32 Div(s32 num, s32 denom), return denom != 0 ? num / denom : 0;)
STUB_FUNC(void BitUnPack(const void *src, void *dst, const void *data))
STUB_FUNC(void FastUnsafeCopy32(const void *src, void *dst, u32 size))
STUB_FUNC(void LZ77UnCompWRAMOptimized(const u32 *src, void *dst))

// 8. 汇编填充符号
u8 LZ77UnCompWRAMOptimized_end[1];
u8 __iwram_end[1];
