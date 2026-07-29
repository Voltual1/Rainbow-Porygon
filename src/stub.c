#include "global.h"
#include "main.h"
#include "gba/m4a_internal.h"
#include "libgcnmultiboot.h"
#include <stdio.h>
#include <string.h>

// 清除宏定义的物理函数
#undef RegisterRamReset
#undef IntrMain

// 底层系统 
void RegisterRamReset(u32 resetFlags) { puts("RegisterRamReset stub"); }
void IntrMain(void) { puts("IntrMain stub"); }

extern void CB2_InitCopyrightScreenAfterBootup(void);
void gInitialMainCB2(void)
{
    CB2_InitCopyrightScreenAfterBootup();
}

const u8 RomHeaderGameCode[4] = "BPEE";
const u8 RomHeaderSoftwareVersion = 0;

// 内存与 Flash 
void ReInitializeEWRAM(void) {}
u8 ProgramFlashSector_DUMMY(u16 sectorNum, u8 *src) { return 0; }

// 音频 (C 语言还原 m4a_1.s) 
u32 umul3232H32(u32 a, u32 b) { return (u32)(((u64)a * b) >> 32); }

extern const u8 gClockTable[];
extern const MPlayFunc gMPlayJumpTableTemplate[36];

void RealClearChain(void *x)
{
    struct SoundChannel *chan = (struct SoundChannel *)x;
    struct MusicPlayerTrack *track = chan->track;
    if (track == NULL) return;
    
    struct SoundChannel *next = chan->nextChannelPointer;
    struct SoundChannel *prev = chan->prevChannelPointer;
    
    if (prev != NULL)
        prev->nextChannelPointer = next;
    else
        track->chan = next;
        
    if (next != NULL)
        next->prevChannelPointer = prev;
        
    chan->track = NULL;
}

void TrackStop(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    if (track->flags & MPT_FLG_EXIST)
    {
        struct SoundChannel *chan = track->chan;
        while (chan != NULL)
        {
            if (chan->statusFlags != 0)
            {
                if (chan->type & TONEDATA_TYPE_CGB)
                {
                    if (SOUND_INFO_PTR->CgbOscOff)
                        SOUND_INFO_PTR->CgbOscOff(chan->type & TONEDATA_TYPE_CGB);
                }
                chan->statusFlags = 0;
            }
            chan->track = NULL;
            chan = chan->nextChannelPointer;
        }
        track->chan = NULL;
    }
}

void ChnVolSetAsm(struct MusicPlayerTrack *track, struct SoundChannel *chan)
{
    s32 velocity = chan->velocity;
    s32 rhythmPan = (s8)chan->rhythmPan;
    s32 right = 0x80 + rhythmPan;
    s32 left = 0x7F - rhythmPan;
    
    s32 rightVol = (right * velocity * track->volMR) >> 14;
    s32 leftVol = (left * velocity * track->volML) >> 14;
    
    chan->rightVolume = rightVol > 0xFF ? 0xFF : rightVol;
    chan->leftVolume = leftVol > 0xFF ? 0xFF : leftVol;
}

void ply_fine(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    struct SoundChannel *chan = track->chan;
    while (chan != NULL)
    {
        if (chan->statusFlags & SOUND_CHANNEL_SF_ON)
            chan->statusFlags |= SOUND_CHANNEL_SF_STOP;
        struct SoundChannel *next = chan->nextChannelPointer;
        RealClearChain(chan);
        chan = next;
    }
    track->flags = 0;
}

void ply_goto(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    u32 ptr = (track->cmdPtr[0] << 0) | (track->cmdPtr[1] << 8) | (track->cmdPtr[2] << 16) | (track->cmdPtr[3] << 24);
    track->cmdPtr = (u8 *)(uintptr_t)ptr;
}

void ply_patt(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    if (track->patternLevel < 3)
    {
        track->patternStack[track->patternLevel] = track->cmdPtr + 4;
        track->patternLevel++;
        ply_goto(mplayInfo, track);
    }
    else
    {
        ply_fine(mplayInfo, track);
    }
}

void ply_pend(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    if (track->patternLevel > 0)
    {
        track->patternLevel--;
        track->cmdPtr = track->patternStack[track->patternLevel];
    }
}

void ply_rept(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    if (*track->cmdPtr == 0)
    {
        track->cmdPtr++;
        ply_goto(mplayInfo, track);
    }
    else
    {
        track->repN++;
        if (track->repN >= *track->cmdPtr)
        {
            track->repN = 0;
            track->cmdPtr += 5;
        }
        else
        {
            track->cmdPtr++;
            ply_goto(mplayInfo, track);
        }
    }
}

void ply_prio(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    track->priority = *track->cmdPtr++;
}

void ply_tempo(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    u16 tempo = *track->cmdPtr++;
    mplayInfo->tempoD = tempo * 2;
    mplayInfo->tempoI = (mplayInfo->tempoD * mplayInfo->tempoU) >> 8;
}

void ply_keysh(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    track->keyShift = *track->cmdPtr++;
    track->flags |= MPT_FLG_PITCHG;
}

void ply_voice(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    u8 voice = *track->cmdPtr++;
    struct ToneData *tone = &mplayInfo->tone[voice];
    track->tone.type = tone->type;
    track->tone.wav = tone->wav;
    track->tone.attack = tone->attack;
    track->tone.decay = tone->decay;
    track->tone.sustain = tone->sustain;
    track->tone.release = tone->release;
    track->tone.length = tone->length;
    track->tone.pan_sweep = tone->pan_sweep;
}

void ply_vol(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    track->vol = *track->cmdPtr++;
    track->flags |= MPT_FLG_VOLCHG;
}

void ply_pan(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    track->pan = *track->cmdPtr++ - C_V;
    track->flags |= MPT_FLG_VOLCHG;
}

void ply_bend(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    track->bend = *track->cmdPtr++ - C_V;
    track->flags |= MPT_FLG_PITCHG;
}

void ply_bendr(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    track->bendRange = *track->cmdPtr++;
    track->flags |= MPT_FLG_PITCHG;
}

void ply_lfos(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    track->lfoSpeed = *track->cmdPtr++;
    if (track->lfoSpeed == 0)
    {
        ClearModM(track);
    }
}

void ply_lfodl(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    track->lfoDelay = *track->cmdPtr++;
}

void ply_mod(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    track->mod = *track->cmdPtr++;
    if (track->mod == 0)
    {
        ClearModM(track);
    }
}

void ply_modt(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    u8 type = *track->cmdPtr++;
    if (track->modT != type)
    {
        track->modT = type;
        track->flags |= (MPT_FLG_VOLCHG | MPT_FLG_PITCHG);
    }
}

void ply_tune(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    track->tune = *track->cmdPtr++ - C_V;
    track->flags |= MPT_FLG_PITCHG;
}

void ply_port(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    u8 offset = *track->cmdPtr++;
    u8 data = *track->cmdPtr++;
    vu8 *reg = (vu8 *)(REG_ADDR_SOUND1CNT_L + offset);
    *reg = data;
}

void ply_endtie(struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    u8 key;
    if (*track->cmdPtr < 0x80)
    {
        key = *track->cmdPtr++;
        track->key = key;
    }
    else
    {
        key = track->key;
    }

    struct SoundChannel *chan = track->chan;
    while (chan != NULL)
    {
        if ((chan->statusFlags & (SOUND_CHANNEL_SF_START | SOUND_CHANNEL_SF_ENV)) && !(chan->statusFlags & SOUND_CHANNEL_SF_STOP))
        {
            if (chan->midiKey == key)
            {
                chan->statusFlags |= SOUND_CHANNEL_SF_STOP;
                return;
            }
        }
        chan = chan->nextChannelPointer;
    }
}

void ply_note(u32 note_cmd, struct MusicPlayerInfo *mplayInfo, struct MusicPlayerTrack *track)
{
    track->gateTime = gClockTable[note_cmd];
    
    if (*track->cmdPtr < 0x80)
    {
        track->key = *track->cmdPtr++;
        if (*track->cmdPtr < 0x80)
        {
            track->velocity = *track->cmdPtr++;
            if (*track->cmdPtr < 0x80)
            {
                track->gateTime += *track->cmdPtr++;
            }
        }
    }
    
    struct SoundChannel *chan = NULL;
    struct ToneData *tone = &track->tone;
    u8 key = track->key;
    u32 rhythmPan = 0;
    
    if (tone->type & (TONEDATA_TYPE_RHY | TONEDATA_TYPE_SPL))
    {
        u8 index = key;
        if (tone->type & TONEDATA_TYPE_SPL)
        {
            u32 ptr;
            memcpy(&ptr, &tone->attack, 4);
            u8 *keySplitTable = (u8 *)(uintptr_t)ptr;
            index = keySplitTable[key];
        }
        
        struct ToneData *subTone = (struct ToneData *)tone->wav + index;
        if (subTone->type & (TONEDATA_TYPE_RHY | TONEDATA_TYPE_SPL))
            return;
            
        if (tone->type & TONEDATA_TYPE_RHY)
        {
            if (subTone->pan_sweep & 0x80)
            {
                rhythmPan = (s8)(subTone->pan_sweep - TONEDATA_P_S_PAN) * 2;
            }
            key = subTone->key;
        }
        tone = subTone;
    }
    
    u32 priority = mplayInfo->priority + track->priority;
    if (priority > 0xFF) priority = 0xFF;
    
    u8 cgbType = tone->type & TONEDATA_TYPE_CGB;
    
    if (cgbType != 0)
    {
        if (SOUND_INFO_PTR->cgbChans == NULL)
            return;
            
        struct CgbChannel *cgbChan = &SOUND_INFO_PTR->cgbChans[cgbType - 1];
        if (cgbChan->statusFlags & SOUND_CHANNEL_SF_ON)
        {
            if (!(cgbChan->statusFlags & SOUND_CHANNEL_SF_STOP))
            {
                if (cgbChan->priority >= priority)
                {
                    if (cgbChan->priority > priority || (uintptr_t)cgbChan->track >= (uintptr_t)track)
                        return;
                }
            }
        }
        chan = (struct SoundChannel *)cgbChan;
    }
    else
    {
        struct SoundChannel *bestChan = NULL;
        
        for (int i = 0; i < SOUND_INFO_PTR->maxChans; i++)
        {
            struct SoundChannel *c = &SOUND_INFO_PTR->chans[i];
            if (!(c->statusFlags & SOUND_CHANNEL_SF_ON))
            {
                bestChan = c;
                break;
            }
            
            if (c->statusFlags & SOUND_CHANNEL_SF_STOP)
            {
                if (bestChan == NULL || !(bestChan->statusFlags & SOUND_CHANNEL_SF_STOP) ||
                    c->priority < bestChan->priority || 
                    (c->priority == bestChan->priority && (uintptr_t)c->track < (uintptr_t)bestChan->track))
                {
                    bestChan = c;
                }
            }
            else if (bestChan == NULL || !(bestChan->statusFlags & SOUND_CHANNEL_SF_STOP))
            {
                if (c->priority < priority || (c->priority == priority && (uintptr_t)c->track < (uintptr_t)track))
                {
                    if (bestChan == NULL || c->priority < bestChan->priority ||
                       (c->priority == bestChan->priority && (uintptr_t)c->track < (uintptr_t)bestChan->track))
                    {
                        bestChan = c;
                    }
                }
            }
        }
        
        chan = bestChan;
        if (chan == NULL) return;
    }
    
    RealClearChain(chan);
    
    chan->prevChannelPointer = NULL;
    chan->nextChannelPointer = track->chan;
    if (track->chan != NULL)
        track->chan->prevChannelPointer = chan;
    track->chan = chan;
    chan->track = track;
    
    track->lfoDelayC = track->lfoDelay;
    if (track->lfoDelay == 0)
        ClearModM(track);
        
    TrkVolPitSet(mplayInfo, track);
    
    chan->gateTime = track->gateTime;
    chan->priority = priority;
    chan->key = key;
    chan->rhythmPan = rhythmPan;
    chan->type = tone->type;
    chan->wav = tone->wav;
    chan->attack = tone->attack;
    chan->decay = tone->decay;
    chan->sustain = tone->sustain;
    chan->release = tone->release;
    chan->pseudoEchoVolume = track->pseudoEchoVolume;
    chan->pseudoEchoLength = track->pseudoEchoLength;
    chan->midiKey = track->key;
    chan->velocity = track->velocity;
    
    ChnVolSetAsm(track, chan);
    
    s32 finalKey = chan->key + track->keyM;
    if (finalKey < 0) finalKey = 0;
    
    if (cgbType != 0)
    {
        struct CgbChannel *cgbChan = (struct CgbChannel *)chan;
        cgbChan->length = tone->length;
        cgbChan->sweep = (tone->pan_sweep & 0x80) ? 0x08 : tone->pan_sweep;
        cgbChan->frequency = SOUND_INFO_PTR->MidiKeyToCgbFreq(cgbType, finalKey, track->pitM);
    }
    else
    {
        chan->count = track->unk_3C;
        chan->frequency = MidiKeyToFreq(chan->wav, finalKey, track->pitM);
    }
    
    chan->statusFlags = SOUND_CHANNEL_SF_START;
    track->flags &= 0xF0;
}

void MPlayMain(struct MusicPlayerInfo *mplayInfo)
{
    if (mplayInfo->ident != ID_NUMBER)
        return;

    mplayInfo->ident++;

    if (mplayInfo->MPlayMainNext != NULL)
        mplayInfo->MPlayMainNext(mplayInfo->musicPlayerNext);

    if (mplayInfo->status & MUSICPLAYER_STATUS_PAUSE)
    {
        mplayInfo->ident = ID_NUMBER;
        return;
    }

    FadeOutBody(mplayInfo);

    if (mplayInfo->status & MUSICPLAYER_STATUS_PAUSE)
    {
        mplayInfo->ident = ID_NUMBER;
        return;
    }

    mplayInfo->tempoC += mplayInfo->tempoI;
    while (mplayInfo->tempoC >= 150)
    {
        mplayInfo->tempoC -= 150;

        for (int i = 0; i < mplayInfo->trackCount; i++)
        {
            struct MusicPlayerTrack *track = &mplayInfo->tracks[i];
            if (!(track->flags & MPT_FLG_EXIST))
                continue;

            struct SoundChannel *chan = track->chan;
            while (chan != NULL)
            {
                struct SoundChannel *next = chan->nextChannelPointer;
                if (chan->statusFlags & SOUND_CHANNEL_SF_ON)
                {
                    if (chan->gateTime != 0)
                    {
                        chan->gateTime--;
                        if (chan->gateTime == 0)
                            chan->statusFlags |= SOUND_CHANNEL_SF_STOP;
                    }
                }
                else
                {
                    RealClearChain(chan);
                }
                chan = next;
            }

            if (track->flags & MPT_FLG_START)
            {
                Clear64byte(track);
                track->flags = MPT_FLG_EXIST;
                track->bendRange = 2;
                track->volX = 64;
                track->lfoSpeed = 22;
                track->tone.type = 1;
            }

            while (track->wait == 0)
            {
                u8 cmd = *track->cmdPtr;
                if (cmd < 0x80)
                {
                    cmd = track->runningStatus;
                }
                else
                {
                    track->cmdPtr++;
                    if (cmd >= 0xBD)
                        track->runningStatus = cmd;
                }

                if (cmd >= 0xCF)
                {
                    SOUND_INFO_PTR->plynote(cmd - 0xCF, mplayInfo, track);
                }
                else if (cmd >= 0xB1)
                {
                    mplayInfo->cmd = cmd - 0xB1;
                    SOUND_INFO_PTR->MPlayJumpTable[mplayInfo->cmd](mplayInfo, track);
                    if (!(track->flags & MPT_FLG_EXIST))
                        break;
                }
                else
                {
                    track->wait = gClockTable[cmd - 0x80];
                }
            }

            if (track->flags & MPT_FLG_EXIST)
            {
                track->wait--;

                if (track->lfoSpeed != 0 && track->mod != 0)
                {
                    if (track->lfoDelayC != 0)
                    {
                        track->lfoDelayC--;
                    }
                    else
                    {
                        track->lfoSpeedC += track->lfoSpeed;
                        s8 phase = (s8)track->lfoSpeedC;
                        u32 val = (phase >= 0) ? phase : (0x80 - phase);
                        u32 modM = (track->mod * val) >> 6;
                        if (track->modM != modM)
                        {
                            track->modM = modM;
                            if (track->modT == 0)
                                track->flags |= MPT_FLG_PITCHG;
                            else
                                track->flags |= MPT_FLG_VOLCHG;
                        }
                    }
                }
            }
        }

        mplayInfo->clock++;
        if (mplayInfo->status == 0)
            break;
    }

    for (int i = 0; i < mplayInfo->trackCount; i++)
    {
        struct MusicPlayerTrack *track = &mplayInfo->tracks[i];
        if (!(track->flags & MPT_FLG_EXIST))
            continue;

        if (track->flags & (MPT_FLG_VOLCHG | MPT_FLG_PITCHG))
        {
            TrkVolPitSet(mplayInfo, track);
            struct SoundChannel *chan = track->chan;
            while (chan != NULL)
            {
                struct SoundChannel *next = chan->nextChannelPointer;
                if (!(chan->statusFlags & SOUND_CHANNEL_SF_ON))
                {
                    RealClearChain(chan);
                    chan = next;
                    continue;
                }

                u8 type = chan->type & TONEDATA_TYPE_CGB;

                if (track->flags & MPT_FLG_VOLCHG)
                {
                    ChnVolSetAsm(track, chan);

                    if (type != 0)
                    {
                        ((struct CgbChannel *)chan)->modify |= CGB_CHANNEL_MO_VOL;
                    }
                }

                if (track->flags & MPT_FLG_PITCHG)
                {
                    s32 key = chan->key + track->keyM;
                    if (key < 0) key = 0;

                    if (type != 0)
                    {
                        ((struct CgbChannel *)chan)->frequency = SOUND_INFO_PTR->MidiKeyToCgbFreq(type, key, track->pitM);
                        ((struct CgbChannel *)chan)->modify |= CGB_CHANNEL_MO_PIT;
                    }
                    else
                    {
                        chan->frequency = MidiKeyToFreq(chan->wav, key, track->pitM);
                    }
                }

                chan = next;
            }
            track->flags &= ~(MPT_FLG_VOLCHG | MPT_FLG_PITCHG);
        }
    }

    mplayInfo->ident = ID_NUMBER;
}

void SoundMain(void)
{
    if (SOUND_INFO_PTR->ident != ID_NUMBER) return;
    SOUND_INFO_PTR->ident++;
    
    if (SOUND_INFO_PTR->MPlayMainHead != NULL)
        SOUND_INFO_PTR->MPlayMainHead(SOUND_INFO_PTR->musicPlayerHead);
        
    if (SOUND_INFO_PTR->CgbSound != NULL)
        SOUND_INFO_PTR->CgbSound();
        
    SOUND_INFO_PTR->ident = ID_NUMBER;
}

void SoundMainBTM(void) {}

void MPlayJumpTableCopy(MPlayFunc *mplayJumpTable) {
    memcpy(mplayJumpTable, gMPlayJumpTableTemplate, 36 * sizeof(MPlayFunc));
}

// --- 4. 防灾防御隔离区 (Memory Padding Guard) ---
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

// --- 5. 工具函数 ---
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