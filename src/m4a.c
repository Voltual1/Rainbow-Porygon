#include "global.h"
#include <string.h>
#include "gba/m4a_internal.h"

#ifdef PORTABLE
    #include "cgb_audio.h"
    extern const MPlayFunc gMPlayJumpTableTemplate[36];
#endif

extern const u8 gCgb3Vol[];

#define BSS_CODE __attribute__((section(".bss.code")))

COMMON_DATA struct SoundInfo gSoundInfo = {0};
COMMON_DATA struct PokemonCrySong gPokemonCrySongs[MAX_POKEMON_CRIES] = {0};
COMMON_DATA struct MusicPlayerInfo gPokemonCryMusicPlayers[MAX_POKEMON_CRIES] = {0};
COMMON_DATA struct MusicPlayerInfo gMPlayInfo_BGM = {0};
COMMON_DATA MPlayFunc gMPlayJumpTable[36] = {0};
COMMON_DATA struct CgbChannel gCgbChans[4] = {0};
COMMON_DATA struct MusicPlayerInfo gMPlayInfo_SE1 = {0};
COMMON_DATA struct MusicPlayerInfo gMPlayInfo_SE2 = {0};
COMMON_DATA struct MusicPlayerTrack gPokemonCryTracks[MAX_POKEMON_CRIES * 2] = {0};
COMMON_DATA struct PokemonCrySong gPokemonCrySong = {0};
COMMON_DATA u8 gMPlayMemAccArea[0x10] = {0};
COMMON_DATA struct MusicPlayerInfo gMPlayInfo_SE3 = {0};

u32 MidiKeyToFreq(struct WaveData *wav, u8 key, u8 fineAdjust)
{
    // TODO: 64-bit compatible implementation
    return 0;
}

static void UNUSED UnusedDummyFunc(void)
{
    // TODO: 64-bit compatible implementation
}

void MPlayContinue(struct MusicPlayerInfo *mplayInfo)
{
    // TODO: 64-bit compatible implementation
}

void MPlayFadeOut(struct MusicPlayerInfo *mplayInfo, u16 speed)
{
    // TODO: 64-bit compatible implementation
}

void m4aSoundInit(void)
{
    // TODO: 64-bit compatible implementation
}

void m4aSoundMain(void)
{
    // TODO: 64-bit compatible implementation
}

void m4aSongNumStart(u16 n)
{
    // TODO: 64-bit compatible implementation
}

void m4aSongNumStartOrChange(u16 n)
{
    // TODO: 64-bit compatible implementation
}

static void UNUSED m4aSongNumStartOrContinue(u16 n)
{
    // TODO: 64-bit compatible implementation
}

void m4aSongNumStop(u16 n)
{
    // TODO: 64-bit compatible implementation
}

static void UNUSED m4aSongNumContinue(u16 n)
{
    // TODO: 64-bit compatible implementation
}

void m4aMPlayAllStop(void)
{
    // TODO: 64-bit compatible implementation
}

void m4aMPlayContinue(struct MusicPlayerInfo *mplayInfo)
{
    // TODO: 64-bit compatible implementation
}

void m4aMPlayAllContinue(void)
{
    // TODO: 64-bit compatible implementation
}

void m4aMPlayFadeOut(struct MusicPlayerInfo *mplayInfo, u16 speed)
{
    // TODO: 64-bit compatible implementation
}

void m4aMPlayFadeOutTemporarily(struct MusicPlayerInfo *mplayInfo, u16 speed)
{
    // TODO: 64-bit compatible implementation
}

void m4aMPlayFadeIn(struct MusicPlayerInfo *mplayInfo, u16 speed)
{
    // TODO: 64-bit compatible implementation
}

void m4aMPlayImmInit(struct MusicPlayerInfo *mplayInfo)
{
    // TODO: 64-bit compatible implementation
}

void MPlayExtender(struct CgbChannel *cgbChans)
{
    // TODO: 64-bit compatible implementation
}

static void UNUSED MusicPlayerJumpTableCopy(void)
{
    // TODO: 64-bit compatible implementation
}

void ClearChain(void *x)
{
    // TODO: 64-bit compatible implementation
}

void Clear64byte(void *x)
{
    // TODO: 64-bit compatible implementation
}

void SoundInit(struct SoundInfo *soundInfo)
{
    // TODO: 64-bit compatible implementation
}

void SampleFreqSet(u32 freq)
{
    // TODO: 64-bit compatible implementation
}

void m4aSoundMode(u32 mode)
{
    // TODO: 64-bit compatible implementation
}

void SoundClear(void)
{
    // TODO: 64-bit compatible implementation
}

void m4aSoundVSyncOff(void)
{
    // TODO: 64-bit compatible implementation
}

void m4aSoundVSyncOn(void)
{
    // TODO: 64-bit compatible implementation
}

void MPlayOpen(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *tracks, u8 trackCount)
{
    // TODO: 64-bit compatible implementation
}

void MPlayStart(struct MusicPlayerInfo *mplayInfo, struct SongHeader *songHeader)
{
    // TODO: 64-bit compatible implementation
}

void m4aMPlayStop(struct MusicPlayerInfo *mplayInfo)
{
    // TODO: 64-bit compatible implementation
}

void FadeOutBody(struct MusicPlayerInfo *mplayInfo)
{
    // TODO: 64-bit compatible implementation
}

void TrkVolPitSet(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

u32 MidiKeyToCgbFreq(u8 chanNum, u8 key, u8 fineAdjust)
{
    // TODO: 64-bit compatible implementation
    return 0;
}

void CgbOscOff(u8 chanNum)
{
    // TODO: 64-bit compatible implementation
}

static inline int ChnVolSetAsm(struct MusicPlayerTrack *track, struct SoundChannel *chan)
{
    // TODO: 64-bit compatible implementation
    return 0;
}

static inline int CgbPan(struct CgbChannel *chan)
{
    // TODO: 64-bit compatible implementation
    return 0;
}

void CgbModVol(struct CgbChannel *chan)
{
    // TODO: 64-bit compatible implementation
}

void CgbSound(void)
{
    // TODO: 64-bit compatible implementation
}

void m4aMPlayTempoControl(struct MusicPlayerInfo *mplayInfo, u16 tempo)
{
    // TODO: 64-bit compatible implementation
}

void m4aMPlayVolumeControl(struct MusicPlayerInfo *mplayInfo, u16 trackBits, u16 volume)
{
    // TODO: 64-bit compatible implementation
}

void m4aMPlayPitchControl(struct MusicPlayerInfo *mplayInfo, u16 trackBits, s16 pitch)
{
    // TODO: 64-bit compatible implementation
}

void m4aMPlayPanpotControl(struct MusicPlayerInfo *mplayInfo, u16 trackBits, s8 pan)
{
    // TODO: 64-bit compatible implementation
}

void ClearModM(struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void m4aMPlayModDepthSet(struct MusicPlayerInfo *mplayInfo, u16 trackBits, u8 modDepth)
{
    // TODO: 64-bit compatible implementation
}

void m4aMPlayLFOSpeedSet(struct MusicPlayerInfo *mplayInfo, u16 trackBits, u8 lfoSpeed)
{
    // TODO: 64-bit compatible implementation
}

void ply_memacc(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void ply_xcmd(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void ply_xxx(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void ply_xwave(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void ply_xtype(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void ply_xatta(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void ply_xdeca(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void ply_xsust(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void ply_xrele(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void ply_xiecv(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void ply_xiecl(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void ply_xleng(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void ply_xswee(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void ply_xwait(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void ply_xcmd_0D(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    // TODO: 64-bit compatible implementation
}

void DummyFunc(void)
{
    // TODO: 64-bit compatible implementation
}

struct MusicPlayerInfo *SetPokemonCryTone(struct ToneData *tone)
{
    // TODO: 64-bit compatible implementation
    return NULL;
}

void SetPokemonCryVolume(u8 val)
{
    // TODO: 64-bit compatible implementation
}

void SetPokemonCryPanpot(s8 val)
{
    // TODO: 64-bit compatible implementation
}

void SetPokemonCryPitch(s16 val)
{
    // TODO: 64-bit compatible implementation
}

void SetPokemonCryLength(u16 val)
{
    // TODO: 64-bit compatible implementation
}

void SetPokemonCryRelease(u8 val)
{
    // TODO: 64-bit compatible implementation
}

void SetPokemonCryProgress(u32 val)
{
    // TODO: 64-bit compatible implementation
}

bool32 IsPokemonCryPlaying(struct MusicPlayerInfo *mplayInfo)
{
    // TODO: 64-bit compatible implementation
    return FALSE;
}

void SetPokemonCryChorus(s8 val)
{
    // TODO: 64-bit compatible implementation
}

void SetPokemonCryStereo(u32 val)
{
    // TODO: 64-bit compatible implementation
}

void SetPokemonCryPriority(u8 val)
{
    // TODO: 64-bit compatible implementation
}

void m4aSoundVSync(void)
{
    // TODO: 64-bit compatible implementation
}
