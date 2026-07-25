#include "global.h"
#include "gba/m4a_internal.h"

// 显式声明所有 MIDI 处理器 C 函数
extern void ply_fine(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_goto(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_patt(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_pend(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_rept(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_memacc(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_prio(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_tempo(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_keysh(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_voice(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_vol(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_pan(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_bend(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_bendr(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_lfos(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_lfodl(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_mod(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_modt(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_tune(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_port(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_xcmd(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_endtie(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_note(u32, struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_xxx(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_xwave(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_xtype(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_xatta(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_xdeca(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_xsust(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_xrele(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_xiecv(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_xiecl(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_xleng(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_xswee(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_xwait(struct MusicPlayerInfo *, struct MusicPlayerTrack *);
extern void ply_xcmd_0D(struct MusicPlayerInfo *, struct MusicPlayerTrack *);

// CAN FIX: 合并并定义唯一且类型正确的跨平台音频跳转表模板，保留全部占位符以确保指令映射绝对精确
const MPlayFunc gMPlayJumpTableTemplate[36] =
{
    (MPlayFunc)ply_fine,        // 0 (0xB1: FINE)
    (MPlayFunc)ply_goto,        // 1 (0xB2: GOTO)
    (MPlayFunc)ply_patt,        // 2 (0xB3: PATT)
    (MPlayFunc)ply_pend,        // 3 (0xB4: PEND)
    (MPlayFunc)ply_rept,        // 4 (0xB5: REPT)
    (MPlayFunc)ply_fine,        // 5 (0xB6: Placeholder)
    (MPlayFunc)ply_fine,        // 6 (0xB7: Placeholder)
    (MPlayFunc)ply_fine,        // 7 (0xB8: Placeholder)
    (MPlayFunc)ply_fine,        // 8 (0xB9: MEMACC, overwritten by Extender)
    (MPlayFunc)ply_prio,        // 9 (0xBA: PRIO)
    (MPlayFunc)ply_tempo,       // 10 (0xBB: TEMPO)
    (MPlayFunc)ply_keysh,       // 11 (0xBC: KEYSH)
    (MPlayFunc)ply_voice,       // 12 (0xBD: VOICE)
    (MPlayFunc)ply_vol,         // 13 (0xBE: VOL)
    (MPlayFunc)ply_pan,         // 14 (0xBF: PAN)
    (MPlayFunc)ply_bend,        // 15 (0xC0: BEND)
    (MPlayFunc)ply_bendr,       // 16 (0xC1: BENDR)
    (MPlayFunc)ply_lfos,        // 17 (0xC2: LFOS, overwritten by Extender)
    (MPlayFunc)ply_lfodl,       // 18 (0xC3: LFODL)
    (MPlayFunc)ply_mod,         // 19 (0xC4: MOD, overwritten by Extender)
    (MPlayFunc)ply_modt,        // 20 (0xC5: MODT)
    (MPlayFunc)ply_fine,        // 21 (0xC6: Placeholder)
    (MPlayFunc)ply_fine,        // 22 (0xC7: Placeholder)
    (MPlayFunc)ply_tune,        // 23 (0xC8: TUNE)
    (MPlayFunc)ply_fine,        // 24 (0xC9: Placeholder)
    (MPlayFunc)ply_fine,        // 25 (0xCA: Placeholder)
    (MPlayFunc)ply_fine,        // 26 (0xCB: Placeholder)
    (MPlayFunc)ply_port,        // 27 (0xCC: PORT)
    (MPlayFunc)ply_fine,        // 28 (0xCD: XCMD, overwritten by Extender)
    (MPlayFunc)ply_endtie,      // 29 (0xCE: EOT)
    (MPlayFunc)SampleFreqSet,   // 30
    (MPlayFunc)TrackStop,       // 31
    (MPlayFunc)FadeOutBody,     // 32
    (MPlayFunc)TrkVolPitSet,    // 33
    (MPlayFunc)RealClearChain,  // 34
    (MPlayFunc)SoundMainBTM     // 35
};

// This is a table of deltas between sample values in compressed PCM data.
const s8 gDeltaEncodingTable[] =
{
      0,
      1,
      4,
      9,
     16,
     25,
     36,
     49,
    -64,
    -49,
    -36,
    -25,
    -16,
     -9,
     -4,
     -1,
};

const u8 gScaleTable[] =
{
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
};

const u32 gFreqTable[] =
{
    2147483648u,
    2275179671u,
    2410468894u,
    2553802834u,
    2705659852u,
    2866546760u,
    3037000500u,
    3217589947u,
    3408917802u,
    3611622603u,
    3826380858u,
    4053909305u,
};

const u16 gPcmSamplesPerVBlankTable[] =
{
    96,
    132,
    176,
    224,
    264,
    304,
    352,
    448,
    528,
    608,
    672,
    704,
};

const u8 gCgbScaleTable[] =
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB,
};

const s16 gCgbFreqTable[] =
{
    -2004,
    -1891,
    -1785,
    -1685,
    -1591,
    -1501,
    -1417,
    -1337,
    -1262,
    -1192,
    -1125,
    -1062,
};

const u8 gNoiseTable[] =
{
    0xD7, 0xD6, 0xD5, 0xD4,
    0xC7, 0xC6, 0xC5, 0xC4,
    0xB7, 0xB6, 0xB5, 0xB4,
    0xA7, 0xA6, 0xA5, 0xA4,
    0x97, 0x96, 0x95, 0x94,
    0x87, 0x86, 0x85, 0x84,
    0x77, 0x76, 0x75, 0x74,
    0x67, 0x66, 0x65, 0x64,
    0x57, 0x56, 0x55, 0x54,
    0x47, 0x46, 0x45, 0x44,
    0x37, 0x36, 0x35, 0x34,
    0x27, 0x26, 0x25, 0x24,
    0x17, 0x16, 0x15, 0x14,
    0x07, 0x06, 0x05, 0x04,
    0x03, 0x02, 0x01, 0x00,
};

const u8 gCgb3Vol[] =
{
    0x00, 0x00,
    0x60, 0x60, 0x60, 0x60,
    0x40, 0x40, 0x40, 0x40,
    0x80, 0x80, 0x80, 0x80,
    0x20, 0x20,
};

const u8 gClockTable[] =
{
    0x00,
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07,
    0x08,
    0x09,
    0x0A,
    0x0B,
    0x0C,
    0x0D,
    0x0E,
    0x0F,
    0x10,
    0x11,
    0x12,
    0x13,
    0x14,
    0x15,
    0x16,
    0x17,
    0x18,
    0x1C,
    0x1E,
    0x20,
    0x24,
    0x28,
    0x2A,
    0x2C,
    0x30,
    0x34,
    0x36,
    0x38,
    0x3C,
    0x40,
    0x42,
    0x44,
    0x48,
    0x4C,
    0x4E,
    0x50,
    0x54,
    0x58,
    0x5A,
    0x5C,
    0x60,
};

#define FINE   0xb1
#define GOTO   0xb2
#define PATT   0xb3
#define PEND   0xb4
#define REPT   0xb5
#define MEMACC 0xb9
#define PRIO   0xba
#define TEMPO  0xbb
#define KEYSH  0xbc
#define VOICE  0xbd
#define VOL    0xbe
#define PAN    0xbf
#define BEND   0xc0
#define BENDR  0xc1
#define LFOS   0xc2
#define LFODL  0xc3
#define MOD    0xc4
#define MODT   0xc5
#define TUNE   0xc8

#define XCMD   0xcd
#define xRELE  0x07
#define xIECV  0x08
#define xIECL  0x09
#define xWAIT  0x0c

#define EOT    0xce
#define TIE    0xcf

const struct PokemonCrySong gPokemonCrySongTemplate =
{
    .trackCount = 1,
    .blockCount = 0,
    .priority = 255,
    .reverb = 0,
    .tone = (struct ToneData *)&voicegroup_dummy,
    .part = {NULL, NULL},
    .gap = 0,
    .part0 = TUNE,
    .tuneValue = C_V,
    .gotoCmd = GOTO,
    .gotoTarget = 0,
    .part1 = TUNE,
    .tuneValue2 = C_V + 16,
    .cont = {VOICE, 0}, // part0 jumps here with gotoCmd
    .volCmd = VOL,
    .volumeValue = 127,
    .unkCmd0D = {XCMD, 0x0D},
    .unkCmd0DParam = 0,
    .xreleCmd = {XCMD, xRELE},
    .releaseValue = 0,
    .panCmd = PAN,
    .panValue = C_V,
    .tieCmd = TIE,
    .tieKeyValue = 60, // default is Cn3
    .tieVelocityValue = 127,
    .xwaitCmd = {XCMD, xWAIT},
    .length = 60, // frames to wait
    .end = {EOT, FINE}
};

const XcmdFunc gXcmdTable[] =
{
    ply_xxx,
    ply_xwave,
    ply_xtype,
    ply_xxx,
    ply_xatta,
    ply_xdeca,
    ply_xsust,
    ply_xrele,
    ply_xiecv,
    ply_xiecl,
    ply_xleng,
    ply_xswee,
    ply_xwait,
    ply_xcmd_0D,
};