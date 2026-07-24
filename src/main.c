#include "global.h"
#include "crt0.h"
#include "malloc.h"
#include "link.h"
#include "link_rfu.h"
#include "librfu.h"
#include "m4a.h"
#include "bg.h"
#include "rtc.h"
#include "scanline_effect.h"
#include "overworld.h"
#include "play_time.h"
#include "random.h"
#include "dma3.h"
#include "gba/flash_internal.h"
#include "load_save.h"
#include "gpu_regs.h"
#include "agb_flash.h"
#include "sound.h"
#include "battle.h"
#include "battle_controllers.h"
#include "text.h"
#include "intro.h"
#include "main.h"
#include "trainer_hill.h"
#include "test_runner.h"
#include "constants/rgb.h"

#ifdef PORTABLE
#include "platform.h"
#endif

extern void SDL_Log(const char *fmt, ...);

static void VBlankIntr(void);
static void HBlankIntr(void);
static void VCountIntr(void);
static void SerialIntr(void);
static void IntrDummy(void);

extern void gInitialMainCB2(void);
extern void CB2_FlashNotDetectedScreen(void);

const enum GameVersion gGameVersion = GAME_VERSION;
const enum Language gGameLanguage = GAME_LANGUAGE; 
const char BuildDateTime[] = "2005 02 21 11:10";

const IntrFunc gIntrTableTemplate[] =
{
    VCountIntr, SerialIntr, Timer3Intr, HBlankIntr, VBlankIntr, 
    IntrDummy, IntrDummy, IntrDummy, IntrDummy, IntrDummy, 
    IntrDummy, IntrDummy, IntrDummy, IntrDummy,
};

#define INTR_COUNT ((int)(sizeof(gIntrTableTemplate)/sizeof(IntrFunc)))

COMMON_DATA u16 gKeyRepeatStartDelay = 0;
COMMON_DATA bool8 gLinkTransferringData = 0;
COMMON_DATA struct Main gMain = {0};
COMMON_DATA u16 gKeyRepeatContinueDelay = 0;
COMMON_DATA bool8 gSoftResetDisabled = 0;
COMMON_DATA IntrFunc gIntrTable[INTR_COUNT] = {0};
COMMON_DATA u8 gLinkVSyncDisabled = 0;
COMMON_DATA s8 gPcmDmaCounter = 0;
COMMON_DATA void *gAgbMainLoop_sp = NULL;

static EWRAM_DATA u16 sTrainerId = 0;

static void UpdateLinkAndCallCallbacks(void);
static void InitMainCallbacks(void);
static void CallCallbacks(void);
#ifdef BUGFIX
static void SeedRngWithRtc(void);
#endif
static void ReadKeys(void);
void InitIntrHandlers(void);
static void WaitForVBlank(void);
void EnableVCountIntrAtLine150(void);

#define B_START_SELECT (B_BUTTON | START_BUTTON | SELECT_BUTTON)

void AgbMain(void)
{
    SDL_Log("CAN DEBUG: [Main] AgbMain Booting...");
    
    *(vu16 *)BG_PLTT = RGB_WHITE; 
    InitGpuRegManager();
    REG_WAITCNT = WAITCNT_PREFETCH_ENABLE
    | WAITCNT_WS0_S_1 | WAITCNT_WS0_N_3
    | WAITCNT_WS1_S_1 | WAITCNT_WS1_N_3;
    InitKeys();
    InitIntrHandlers();
    m4aSoundInit();
    EnableVCountIntrAtLine150();
#ifndef PORTABLE
    InitRFU();
#endif
    RtcInit();
#ifndef PORTABLE
    CheckForFlashMemory();
#endif
    InitMainCallbacks();
    InitMapMusic();
#ifdef BUGFIX
    SeedRngWithRtc(); 
#endif
    ClearDma3Requests();
    ResetBgs();
    SetDefaultFontsPointer();
    InitHeap(gHeap, HEAP_SIZE);

    gSoftResetDisabled = FALSE;
    gLinkTransferringData = FALSE;

    gAgbMainLoop_sp = __builtin_frame_address(0);
    SDL_Log("CAN DEBUG: [Main] Entering AgbMainLoop.");
    AgbMainLoop();
}

void AgbMainLoop(void)
{
    int loopCount = 0;
    for (;;)
    {
        SDL_Log("CAN DEBUG: [AgbMainLoop] LoopStart. Frame=%d, State=%d, CB2=%p", 
                loopCount, gMain.state, (void*)gMain.callback2);
        loopCount++;

        ReadKeys();

        if (Overworld_SendKeysToLinkIsRunning() == TRUE)
        {
            gLinkTransferringData = TRUE;
            UpdateLinkAndCallCallbacks();
            gLinkTransferringData = FALSE;
        }
        else
        {
            gLinkTransferringData = FALSE;
            UpdateLinkAndCallCallbacks();

            if (Overworld_RecvKeysFromLinkIsRunning() == TRUE)
            {
                gMain.newKeys = 0;
                ClearSpriteCopyRequests();
                gLinkTransferringData = TRUE;
                UpdateLinkAndCallCallbacks();
                gLinkTransferringData = FALSE;
            }
        }

        PlayTimeCounter_Update();
        MapMusicMain();
        
        SDL_Log("CAN DEBUG: [AgbMainLoop] Before WaitForVBlank. CB2=%p", (void*)gMain.callback2);
        WaitForVBlank();
        SDL_Log("CAN DEBUG: [AgbMainLoop] After WaitForVBlank. CB2=%p", (void*)gMain.callback2);
    }
}

static void UpdateLinkAndCallCallbacks(void)
{
    if (!HandleLinkConnection())
        CallCallbacks();
}

static void InitMainCallbacks(void)
{
    gMain.vblankCounter1 = 0;
    gTrainerHillVBlankCounter = NULL;
    gMain.vblankCounter2 = 0;
    gMain.callback1 = NULL;
    SetMainCallback2(gInitialMainCB2);
    gSaveBlock2Ptr = &gSaveblock2.block;
    gPokemonStoragePtr = &gPokemonStorage.block;
}

static void CallCallbacks(void)
{
    if (gMain.callback1) {
        SDL_Log("CAN DEBUG: [CallCallbacks] Calling CB1=%p", (void*)gMain.callback1);
        gMain.callback1();
        SDL_Log("CAN DEBUG: [CallCallbacks] Returned from CB1. CB2=%p", (void*)gMain.callback2);
    }

    if (gMain.callback2) {
        SDL_Log("CAN DEBUG: [CallCallbacks] Calling CB2=%p", (void*)gMain.callback2);
        gMain.callback2();
        SDL_Log("CAN DEBUG: [CallCallbacks] Returned from CB2. CB2=%p", (void*)gMain.callback2);
    } else {
        SDL_Log("CAN DEBUG: [CallCallbacks] WARNING! CB2 is NULL during CallCallbacks!");
    }
}

void SetMainCallback2(MainCallback callback)
{
    SDL_Log("CAN DEBUG: [SetMainCallback2] Changing CB2 from %p to %p", (void*)gMain.callback2, (void*)callback);
    gMain.callback2 = callback;
    gMain.state = 0;
}

void StartTimer1(void)
{
    REG_TM2CNT_L = 0;
    REG_TM2CNT_H = TIMER_ENABLE | TIMER_COUNTUP;
    REG_TM1CNT_H = TIMER_ENABLE;
}

void SeedRngAndSetTrainerId(void)
{
#ifndef PORTABLE
    u32 val;
    REG_TM1CNT_H = 0;
    REG_TM2CNT_H = 0;
    val = ((u32)REG_TM2CNT_L) << 16;
    val |= REG_TM1CNT_L;
    SeedRng(val);
    sTrainerId = Random();
#else
    SeedRng(0x1234);
    sTrainerId = 0x1234;
#endif
}

u16 GetGeneratedTrainerIdLower(void)
{
    return sTrainerId;
}

void EnableVCountIntrAtLine150(void)
{
    u16 gpuReg = (GetGpuReg(REG_OFFSET_DISPSTAT) & 0xFF) | (150 << 8);
    SetGpuReg(REG_OFFSET_DISPSTAT, gpuReg | DISPSTAT_VCOUNT_INTR);
    EnableInterrupts(INTR_FLAG_VCOUNT);
}

#ifdef BUGFIX
static void SeedRngWithRtc(void)
{
    #define BCD8(x) ((((x) >> 4) & 0xF) * 10 + ((x) & 0xF))
    u32 seconds;
    struct SiiRtcInfo rtc;
    RtcGetInfo(&rtc);
    seconds =
        ((HOURS_PER_DAY * RtcGetDayCount(&rtc) + BCD8(rtc.hour))
        * MINUTES_PER_HOUR + BCD8(rtc.minute))
        * SECONDS_PER_MINUTE + BCD8(rtc.second);
    SeedRng(seconds);
    #undef BCD8
}
#endif

void InitKeys(void)
{
    gKeyRepeatContinueDelay = 5;
    gKeyRepeatStartDelay = 40;

    gMain.heldKeys = 0;
    gMain.newKeys = 0;
    gMain.newAndRepeatedKeys = 0;
    gMain.heldKeysRaw = 0;
    gMain.newKeysRaw = 0;
}

static void ReadKeys(void)
{
#ifndef PORTABLE
    u16 keyInput = REG_KEYINPUT ^ KEYS_MASK;
#else
    u16 keyInput = Platform_GetKeyInput();
#endif
    gMain.newKeysRaw = keyInput & ~gMain.heldKeysRaw;
    gMain.newKeys = gMain.newKeysRaw;
    gMain.newAndRepeatedKeys = gMain.newKeysRaw;

    if (keyInput != 0 && gMain.heldKeys == keyInput)
    {
        gMain.keyRepeatCounter--;
        if (gMain.keyRepeatCounter == 0)
        {
            gMain.newAndRepeatedKeys = keyInput;
            gMain.keyRepeatCounter = gKeyRepeatContinueDelay;
        }
    }
    else
    {
        gMain.keyRepeatCounter = gKeyRepeatStartDelay;
    }

    gMain.heldKeysRaw = keyInput;
    gMain.heldKeys = gMain.heldKeysRaw;

    if (gSaveBlock2Ptr->optionsButtonMode == OPTIONS_BUTTON_MODE_L_EQUALS_A)
    {
        if (JOY_NEW(L_BUTTON))
            gMain.newKeys |= A_BUTTON;
        if (JOY_HELD(L_BUTTON))
            gMain.heldKeys |= A_BUTTON;
    }

    if (JOY_NEW(gMain.watchedKeysMask))
        gMain.watchedKeysPressed = TRUE;
}

void InitIntrHandlers(void)
{
    int i;
    for (i = 0; i < INTR_COUNT; i++)
        gIntrTable[i] = gIntrTableTemplate[i];

    INTR_VECTOR = IntrMain;

    SetVBlankCallback(NULL);
    SetHBlankCallback(NULL);
    SetSerialCallback(NULL);

    REG_IME = 1;
    EnableInterrupts(INTR_FLAG_VBLANK);
}

void SetVBlankCallback(IntrCallback callback)
{
    gMain.vblankCallback = callback;
}

void SetHBlankCallback(IntrCallback callback)
{
    gMain.hblankCallback = callback;
}

void SetVCountCallback(IntrCallback callback)
{
    gMain.vcountCallback = callback;
}

void RestoreSerialTimer3IntrHandlers(void)
{
    gIntrTable[1] = SerialIntr;
    gIntrTable[2] = Timer3Intr;
}

void SetSerialCallback(IntrCallback callback)
{
    gMain.serialCallback = callback;
}

static void VBlankIntr(void)
{
    SDL_Log("CAN DEBUG: [VBlankIntr] Start. CB2=%p", (void*)gMain.callback2);
    if (gWirelessCommType != 0)
        RfuVSync();
    else if (gLinkVSyncDisabled == FALSE)
        LinkVSync();

    gMain.vblankCounter1++;

    if (gTrainerHillVBlankCounter && *gTrainerHillVBlankCounter < 0xFFFFFFFF)
        (*gTrainerHillVBlankCounter)++;

    if (gMain.vblankCallback)
        gMain.vblankCallback();

    gMain.vblankCounter2++;

    SDL_Log("CAN DEBUG: [VBlankIntr] Before CopyBufferedValues. CB2=%p", (void*)gMain.callback2);
    CopyBufferedValuesToGpuRegs();
    SDL_Log("CAN DEBUG: [VBlankIntr] Before ProcessDma3. CB2=%p", (void*)gMain.callback2);
    ProcessDma3Requests();
    SDL_Log("CAN DEBUG: [VBlankIntr] After ProcessDma3. CB2=%p", (void*)gMain.callback2);

    gPcmDmaCounter = gSoundInfo.pcmDmaCounter;

    m4aSoundMain();
    SDL_Log("CAN DEBUG: [VBlankIntr] After m4aSoundMain. CB2=%p", (void*)gMain.callback2);
    TryReceiveLinkBattleData();

    if (!gTestRunnerEnabled && (!gMain.inBattle || !(gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_RECORDED))))
        AdvanceRandom();

    UpdateWirelessStatusIndicatorSprite();

    INTR_CHECK |= INTR_FLAG_VBLANK;
    gMain.intrCheck |= INTR_FLAG_VBLANK;
    SDL_Log("CAN DEBUG: [VBlankIntr] End. CB2=%p", (void*)gMain.callback2);
}

void InitFlashTimer(void)
{
    SetFlashTimerIntr(2, gIntrTable + 0x7);
}

static void HBlankIntr(void)
{
    if (gMain.hblankCallback)
        gMain.hblankCallback();

    INTR_CHECK |= INTR_FLAG_HBLANK;
    gMain.intrCheck |= INTR_FLAG_HBLANK;
}

static void VCountIntr(void)
{
    if (gMain.vcountCallback)
        gMain.vcountCallback();

    m4aSoundVSync();
    INTR_CHECK |= INTR_FLAG_VCOUNT;
    gMain.intrCheck |= INTR_FLAG_VCOUNT;
}

static void SerialIntr(void)
{
    if (gMain.serialCallback)
        gMain.serialCallback();

    INTR_CHECK |= INTR_FLAG_SERIAL;
    gMain.intrCheck |= INTR_FLAG_SERIAL;
}

static void IntrDummy(void)
{}

static void WaitForVBlank(void)
{
#ifdef PORTABLE
    VBlankIntrWait();
#else
    gMain.intrCheck &= ~INTR_FLAG_VBLANK;
    if (gWirelessCommType != 0)
    {
        while (!(gMain.intrCheck & INTR_FLAG_VBLANK))
            ;
    }
    else
    {
        VBlankIntrWait();
    }
#endif
}

void SetTrainerHillVBlankCounter(u32 *counter)
{
    gTrainerHillVBlankCounter = counter;
}

void ClearTrainerHillVBlankCounter(void)
{
    gTrainerHillVBlankCounter = NULL;
}

void DoSoftReset(void)
{
    REG_IME = 0;
    m4aSoundVSyncOff();
    ScanlineEffect_Stop();
    DmaStop(1);
    DmaStop(2);
    DmaStop(3);
    SiiRtcProtect();
    SoftReset(RESET_ALL);
}

void ClearPokemonCrySongs(void)
{
    CpuFill16(0, gPokemonCrySongs, MAX_POKEMON_CRIES * sizeof(struct PokemonCrySong));
}

#ifndef PORTABLE
void *_sbrk(int incr) { return (void *)-1; }
int _close(int file) { return -1; }
int _fstat(int file, void *st) { return -1; }
int _getpid(void) { return 1; }
int _isatty(int file) { return 1; }
int _kill(int pid, int sig) { return -1; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _read(int file, char *ptr, int len) { return 0; }
int _write(int file, char *ptr, int len) { return 0; }
#endif
