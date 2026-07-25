#include "global.h"
#include "music_player.h"
#include "sound_mixer.h"
#include "mp2k_common.h"

#ifdef PORTABLE
    #include "cgb_audio.h"
    // 引入 SDL_Log 声明，用于输出诊断日志
    extern void SDL_Log(const char *fmt, ...);
#endif

#define VCOUNT_VBLANK 160
#define TOTAL_SCANLINES 228

static inline void GenerateAudio(struct SoundMixerState *mixer, struct MixerSource *chan, struct WaveData2 *wav, float *outBuffer, u16 samplesPerFrame, float sampleRateReciprocal);
void SampleMixer(struct SoundMixerState *mixer, u32 scanlineLimit, u16 samplesPerFrame, float *outBuffer, u8 dmaCounter, u16 maxBufSize);
static inline bool32 TickEnvelope(struct MixerSource *chan, struct WaveData2 *wav);
void GeneratePokemonSampleAudio(struct SoundMixerState *mixer, struct MixerSource *chan, s8 *current, float *outBuffer, u16 samplesPerFrame, float sampleRateReciprocal, s32 samplesLeftInWav, signed envR, signed envL, s32 loopLen);
static s8 sub_82DF758(struct MixerSource *chan, u32 current);

void RunMixerFrame(void) {
    struct SoundMixerState *mixer = (struct SoundMixerState *)SOUND_INFO_PTR;
    
#ifdef PORTABLE
    // CAN DEBUG: 添加每 60 帧一次的锁状态诊断日志，确认是否因为状态锁未对齐而导致混音器提前退出
    static u32 mixerLogCount = 0;
    if (mixerLogCount++ % 60 == 0) {
        SDL_Log("CAN DEBUG: [RunMixerFrame] mixer=%p, lockStatus=0x%08X, MIXER_UNLOCKED=0x%08X, MIXER_LOCKED=0x%08X",
                (void*)mixer, 
                mixer ? (unsigned int)mixer->lockStatus : 0, 
                (unsigned int)MIXER_UNLOCKED, 
                (unsigned int)MIXER_LOCKED);
    }
#endif

    if (mixer->lockStatus != MIXER_UNLOCKED) {
        return;
    }
    mixer->lockStatus = MIXER_LOCKED;
    
    u32 maxScanlines = mixer->maxScanlines;
    if (mixer->maxScanlines != 0) {
        u32 vcount = REG_VCOUNT;
        maxScanlines += vcount;
        if (vcount < VCOUNT_VBLANK) {
            maxScanlines += TOTAL_SCANLINES;
        }
    }
    
    // CAN FIX: 致命双重 Tick 消除！
    // 永远不要在混音器（RunMixerFrame）中驱动游戏层序列器，
    // 因为这已经在 m4aSoundMain 里面精确驱动过了。
    /*
    if (mixer->firstPlayerFunc != NULL) {
        mixer->firstPlayerFunc(mixer->firstPlayer);
    }
    */
    
    mixer->cgbMixerFunc();
    
    s32 samplesPerFrame = mixer->samplesPerFrame;
    float *outBuffer = mixer->outBuffer;
    s32 dmaCounter = mixer->dmaCounter;
    
    if (dmaCounter > 1) {
        outBuffer += samplesPerFrame * (mixer->framesPerDmaCycle - (dmaCounter - 1)) * 2;
    }
    
    SampleMixer(mixer, maxScanlines, samplesPerFrame, outBuffer, dmaCounter, MIXED_AUDIO_BUFFER_SIZE);
    #ifdef PORTABLE
        cgb_audio_generate(samplesPerFrame);
        
        // 将 CGB PSG PSG方波与噪声混音通道的数据合并到 DirectSound 的 float 混合音频流
        float *cgbBuffer = cgb_get_buffer();
        for (int i = 0; i < samplesPerFrame * 2; i++) {
            outBuffer[i] += cgbBuffer[i];
        }
        
        // 将混音后渲染结果队列投递至跨平台底层 SDL 驱动
        extern void Platform_QueueAudio(float *audioBuffer, s32 samplesPerFrame);
        Platform_QueueAudio(outBuffer, samplesPerFrame * 2 * (s32)sizeof(float));
    #endif
}

void SampleMixer(struct SoundMixerState *mixer, u32 scanlineLimit, u16 samplesPerFrame, float *outBuffer, u8 dmaCounter, u16 maxBufSize) {
    u32 reverb = mixer->reverb;
    if (reverb) {
        float *tmp1 = outBuffer;
        float *tmp2;
        if (dmaCounter == 2) {
            tmp2 = mixer->outBuffer;
        } else {
            tmp2 = outBuffer + samplesPerFrame * 2;
        }
        uf16 i = 0;
        do {
            float s = tmp1[0] + tmp1[1] + tmp2[0] + tmp2[1];
            s *= ((float)reverb / 512.0f);
            tmp1[0] = tmp1[1] = s;
            tmp1+=2;
            tmp2+=2;
        }
        while(++i < samplesPerFrame);
    } else {
        for (int i = 0; i < samplesPerFrame; i++) {
            float *dst = &outBuffer[i*2];
            dst[1] = dst[0] = 0.0f;
        }
    }
    
    float sampleRateReciprocal = mixer->sampleRateReciprocal;
    sf8 numChans = mixer->numChans;
    struct MixerSource *chan = mixer->chans;
    
    for (int i = 0; i < numChans; i++, chan++) {
        struct WaveData2 *wav = (struct WaveData2 *)chan->wav;
        
        if (scanlineLimit != 0) {
            uf16 vcount = REG_VCOUNT;
            if (vcount < VCOUNT_VBLANK) {
                vcount += TOTAL_SCANLINES;
            }
            if (vcount >= scanlineLimit) {
                goto returnEarly;
            }
        }
        
        if (TickEnvelope(chan, wav)) 
        {
            GenerateAudio(mixer, chan, wav, outBuffer, samplesPerFrame, sampleRateReciprocal);
        }
    }
returnEarly:
    mixer->lockStatus = MIXER_UNLOCKED;
}

static inline bool32 TickEnvelope(struct MixerSource *chan, struct WaveData2 *wav) {
    u8 status = chan->status;
    if ((status & 0xC7) == 0) {
        return FALSE;
    }
    
    u8 env = 0;
    if ((status & 0x80) == 0) {
        env = chan->envelopeVol;
        
        if (status & 4) {
            --chan->echoVol;
            if (chan->echoVol <= 0) {
                chan->status = 0;
                return FALSE;
            } else {
                return TRUE;
            }
        } else if (status & 0x40) {
            chan->envelopeVol = env * chan->release / 256U;
            u8 echoVol = chan->echoVol;
            if (chan->envelopeVol > echoVol) {
                return TRUE;
            } else if (echoVol == 0) {
                chan->status = 0;
                return FALSE;
            } else {
                chan->status |= 4;
                return TRUE;
            }
        }
        
        switch (status & 3) {
        uf16 newEnv;
        case 2:
            chan->envelopeVol = env * chan->decay / 256U;
            
            u8 sustain = chan->sustain;
            if (chan->envelopeVol <= sustain && sustain == 0) {
                if (chan->echoVol == 0) {
                    chan->status = 0;
                    return FALSE;
                } else {
                    chan->status |= 4;
                    return TRUE;
                }
            } else if (chan->envelopeVol <= sustain) {
                chan->envelopeVol = sustain;
                --chan->status;
            }
            break;
        case 3:
        attack:
            newEnv = env + chan->attack;
            if (newEnv > 0xFF) {
                chan->envelopeVol = 0xFF;
                --chan->status;
            } else {
                chan->envelopeVol = newEnv;
            }
            break;
        case 1:
        default:
            break;
        }
        
        return TRUE;
    } else if (status & 0x40) {
        chan->status = 0;
        return FALSE;
    } else {
        chan->status = 3;
#ifdef POKEMON_EXTENSIONS
        chan->current = wav->data + chan->ct;
        chan->ct = wav->size - chan->ct;
#else
        chan->current = wav->data;
        chan->ct = wav->size;
#endif
        chan->fw = 0;
        chan->envelopeVol = 0;
        if (wav->loopFlags & 0xC0) {
            chan->status |= 0x10;
        }
        goto attack;
    }
}

static inline void GenerateAudio(struct SoundMixerState *mixer, struct MixerSource *chan, struct WaveData2 *wav, float *outBuffer, u16 samplesPerFrame, float sampleRateReciprocal) {
    uf8 v = chan->envelopeVol * (mixer->masterVol + 1) / 16U;
    chan->envelopeVolR = chan->rightVol * v / 256U;
    chan->envelopeVolL = chan->leftVol * v / 256U;
    
    s32 loopLen = 0;
    s8 *loopStart;
    if (chan->status & 0x10) {
        loopStart = wav->data + wav->loopStart;
        loopLen = wav->size - wav->loopStart;
    }
    s32 samplesLeftInWav = chan->ct;
    s8 *current = chan->current;
    signed envR = chan->envelopeVolR;
    signed envL = chan->envelopeVolL;
#ifdef POKEMON_EXTENSIONS
    if (chan->type & 0x30) {
        GeneratePokemonSampleAudio(mixer, chan, current, outBuffer, samplesPerFrame, sampleRateReciprocal, samplesLeftInWav, envR, envL, loopLen);
    }
    else
#endif
    if (chan->type & 8) {
        for (u16 i = 0; i < samplesPerFrame; i++, outBuffer+=2) {
            sf8 c = *(current++);
            
            outBuffer[1] += (c * envR) / 32768.0f;
            outBuffer[0] += (c * envL) / 32768.0f;
            if (--samplesLeftInWav == 0) {
                samplesLeftInWav = loopLen;
                if (loopLen != 0) {
                    current = loopStart;
                } else {
                    chan->status = 0;
                    return;
                }
            }
        }
        
        chan->ct = samplesLeftInWav;
        chan->current = current;
    } else {
        float finePos = chan->fw;
        float romSamplesPerOutputSample = chan->freq * sampleRateReciprocal;

        sf16 b = current[0];
        sf16 m = current[1] - b;
        current += 1;
        
        for (u16 i = 0; i < samplesPerFrame; i++, outBuffer+=2) {
            float sample = (finePos * m) + b;
            
            outBuffer[1] += (sample * envR) / 32768.0f;
            outBuffer[0] += (sample * envL) / 32768.0f;
            
            finePos += romSamplesPerOutputSample;
            u32 newCoarsePos = finePos;
            if (newCoarsePos != 0) {
                finePos -= (int)finePos;
                samplesLeftInWav -= newCoarsePos;
                if (samplesLeftInWav <= 0) {
                    if (loopLen != 0) {
                        current = loopStart;
                        newCoarsePos = -samplesLeftInWav;
                        samplesLeftInWav += loopLen;
                        while (samplesLeftInWav <= 0) {
                            newCoarsePos -= loopLen;
                            samplesLeftInWav += loopLen;
                        }
                        b = current[newCoarsePos];
                        m = current[newCoarsePos + 1] - b;
                        current += newCoarsePos + 1;
                    } else {
                        chan->status = 0;
                        return;
                    }
                } else {
                    b = current[newCoarsePos - 1];
                    m = current[newCoarsePos] - b;
                    current += newCoarsePos;
                }
            }
        }
        
        chan->fw = finePos;
        chan->ct = samplesLeftInWav;
        chan->current = current - 1;
    }
}

struct WaveData
{
    u16 type;
    u16 status;
    u32 freq;
    u32 loopStart;
    u32 size; 
    s8 data[1]; 
};

void GeneratePokemonSampleAudio(struct SoundMixerState *mixer, struct MixerSource *chan, s8 *current, float *outBuffer, u16 samplesPerFrame, float sampleRateReciprocal, s32 samplesLeftInWav, signed envR, signed envL, s32 loopLen) {
    struct WaveData *wav = chan->wav; 
    float finePos = chan->fw;
    if((chan->status & 0x20) == 0) {
        chan->status |= 0x20;
        if(chan->type & 0x10) {
            s8 *waveEnd = wav->data + wav->size;
            current = wav->data + (waveEnd - current);
            chan->current = current;
        }
        if(wav->type != 0) {
            current -= (uintptr_t)&wav->data;
            chan->current = current;
        }
    }
    float romSamplesPerOutputSample = chan->type & 8 ? 1.0f : chan->freq * sampleRateReciprocal;
    if(wav->type != 0) { 
        chan->blockCount = 0xFF000000;
        if(chan->type & 0x10) { 
            current -= 1;
            sf16 b = sub_82DF758(chan, (uintptr_t)current);
            sf16 m = sub_82DF758(chan, (uintptr_t)current - 1) - b;

            for (u16 i = 0; i < samplesPerFrame; i++, outBuffer+=2) {
                float sample = (finePos * m) + b;
                
                outBuffer[1] += (sample * envR) / 32768.0f;
                outBuffer[0] += (sample * envL) / 32768.0f;
                
                finePos += romSamplesPerOutputSample;
                int newCoarsePos = finePos;
                if (newCoarsePos != 0) {
                    finePos -= (int)finePos;
                    samplesLeftInWav -= newCoarsePos;
                    if (samplesLeftInWav <= 0) {
                        chan->status = 0;
                        break;
                    }
                    else {
                        current -= newCoarsePos;
                        b = sub_82DF758(chan, (uintptr_t)current);
                        m = sub_82DF758(chan, (uintptr_t)current - 1) - b;
                    }
                }
            }
            
            chan->fw = finePos;
            chan->ct = samplesLeftInWav;
            chan->current = current + 1;
        }
        else {
            sf16 b = sub_82DF758(chan, (uintptr_t)current);
            sf16 m = sub_82DF758(chan, (uintptr_t)current + 1) - b;
            current += 1;
        
            for (u16 i = 0; i < samplesPerFrame; i++, outBuffer+=2) {
                float sample = (finePos * m) + b;
                
                outBuffer[1] += (sample * envR) / 32768.0f;
                outBuffer[0] += (sample * envL) / 32768.0f;
                
                finePos += romSamplesPerOutputSample;
                u32 newCoarsePos = finePos; 
                if (newCoarsePos != 0) {
                    finePos -= (int)finePos;
                    samplesLeftInWav -= newCoarsePos;
                    if (samplesLeftInWav <= 0) {
                        if (loopLen != 0) {
                            current = wav->data + wav->loopStart;
                            newCoarsePos = -samplesLeftInWav;
                            samplesLeftInWav += loopLen;
                            while (samplesLeftInWav <= 0) {
                                newCoarsePos -= loopLen;
                                samplesLeftInWav += loopLen;
                            }
                            current += newCoarsePos;
                            b = sub_82DF758(chan, (uintptr_t)current);
                            m = sub_82DF758(chan, (uintptr_t)current + 1) - b;
                            current += 1;
                        } else {
                            chan->status = 0;
                            return;
                        }
                    } else {
                        current += newCoarsePos - 1;
                        b = sub_82DF758(chan, (uintptr_t)current);
                        m = sub_82DF758(chan, (uintptr_t)current + 1) - b;    
                        current += 1;
                    }
                }
            }
            
            chan->fw = finePos;
            chan->ct = samplesLeftInWav;
            chan->current = current - 1;
        }
    }
    else {
        if(chan->type & 0x10) { 
            current -= 1;
            sf16 b = current[0];
            sf16 m = current[-1] - b;
            
            for (u16 i = 0; i < samplesPerFrame; i++, outBuffer+=2) {
                float sample = (finePos * m) + b;
                
                outBuffer[1] += (sample * envR) / 32768.0f;
                outBuffer[0] += (sample * envL) / 32768.0f;
                
                finePos += romSamplesPerOutputSample;
                int newCoarsePos = finePos;
                if (newCoarsePos != 0) {
                    finePos -= (int)finePos;
                    samplesLeftInWav -= newCoarsePos;
                    if (samplesLeftInWav <= 0) {
                        chan->status = 0;
                        break;
                    }
                    else {
                        current -= newCoarsePos;
                        b = current[0];
                        m = current[-1] - b;
                    }
                }
            }
            
            chan->fw = finePos;
            chan->ct = samplesLeftInWav;
            chan->current = current + 1;
        }
    }
}

s8 gBDPCMBlockBuffer[64];
extern const s8 gDeltaEncodingTable[];

static s8 sub_82DF758(struct MixerSource *chan, u32 current) {
    u32 blockOffset = current >> 6; 
    u8 * blockPtr;
    int i;
    if (chan->wav->size < blockOffset * 0x21) {
            DBGPRINTF("Out of bounds decompress in %s wav->size = %u blockPtr = %u\n", __func__, (unsigned int)chan->wav->size, (unsigned int)(blockOffset * 0x21));
            return gBDPCMBlockBuffer[current & 63];
    }
    
    if(chan->blockCount != blockOffset) { 
        s32 s;
        chan->blockCount = blockOffset;
        blockPtr = (u8 *)(chan->wav->data + chan->blockCount * 0x21);
        gBDPCMBlockBuffer[0] = s = (s8)*blockPtr++;
        gBDPCMBlockBuffer[1] = s += gDeltaEncodingTable[*blockPtr++ & 0xF];
        for(i = 2; i < 64; i+=2) {
            u32 temp = *blockPtr++;
            gBDPCMBlockBuffer[i] = s += gDeltaEncodingTable[temp >> 4];
            gBDPCMBlockBuffer[i+1] = s += gDeltaEncodingTable[temp & 0xF];
        }
    }
    return gBDPCMBlockBuffer[current & 63]; 
}