#include "global.h"
#include "bg.h"
#include "constants/rgb.h"
#include "gpu_regs.h"
#include "main.h"
#include "main_menu.h"
#include "menu.h"
#include "palette.h"
#include "scanline_effect.h"
#include "sprite.h"
#include "task.h"
#include "text.h"
#include "text_window.h"
#include "window.h"
#include "load_save.h" // 恢复存档头文件
#include "save.h"      // 恢复存档头文件
#include "new_game.h"  // 恢复新游戏头文件

#ifdef PORTABLE
extern void SDL_Log(const char *fmt, ...);
#endif

static const struct WindowTemplate sPlaceholderWindowTemplate[] =
{
    {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 2,
        .width = 26,
        .height = 15,
        .paletteNum = 15,
        .baseBlock = 1
    },
    DUMMY_WIN_TEMPLATE
};

static const struct BgTemplate sPlaceholderBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    }
};

static void Task_PlaceholderMenu(u8 taskId);
static void VBlankCB_Placeholder(void);
static void CB2_Placeholder(void);

static const u16 sPlaceholderPal[] = {
    RGB_BLACK, RGB_WHITE, RGB(12, 12, 12), RGB(26, 26, 25), 
    [15] = RGB_WHITE 
};

void CB2_InitMainMenu(void)
{
#ifdef PORTABLE
    SDL_Log("CAN DEBUG: [CB2_InitMainMenu] Step 2: Testing Save System Initialization.");
#endif

    // --- 开始恢复存档系统初始化逻辑 ---
    CheckForFlashMemory();
    SetSaveBlocksPointers(0);
    LoadGameSave(SAVE_NORMAL);
    
    if (gSaveFileStatus == SAVE_STATUS_EMPTY || gSaveFileStatus == SAVE_STATUS_CORRUPT)
    {
        Sav2_ClearSetDefault(); // 确保 gSaveBlock2Ptr 指向的不是全 0 区域
    }
    // --- 存档初始化结束 ---

    SetVBlankCallback(NULL);

    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG0CNT, 0);
    DmaFill16(3, 0, (void *)VRAM, VRAM_SIZE);
    DmaFill32(3, 0, (void *)OAM, OAM_SIZE);
    DmaFill16(3, 0, (void *)(PLTT), PLTT_SIZE);

    ResetPaletteFade();
    ScanlineEffect_Stop();
    ResetTasks();
    ResetSpriteData();
    FreeAllSpritePalettes();

    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sPlaceholderBgTemplates, ARRAY_COUNT(sPlaceholderBgTemplates));
    InitWindows(sPlaceholderWindowTemplate);
    DeactivateAllTextPrinters();

    LoadPalette(sPlaceholderPal, BG_PLTT_ID(0), 16 * 2);
    LoadPalette(sPlaceholderPal, BG_PLTT_ID(15), 16 * 2);

    EnableInterrupts(1);
    SetVBlankCallback(VBlankCB_Placeholder);
    SetMainCallback2(CB2_Placeholder);

    ShowBg(0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_BG0_ON | DISPCNT_OBJ_1D_MAP);
    
    CreateTask(Task_PlaceholderMenu, 0);

    FillWindowPixelBuffer(0, PIXEL_FILL(1)); 
    AddTextPrinterParameterized(0, FONT_NORMAL, (const u8[]) { 
        CHAR_S, CHAR_A, CHAR_V, CHAR_E, CHAR_SPACE, 
        CHAR_S, CHAR_Y, CHAR_S, CHAR_T, CHAR_E, CHAR_M, CHAR_SPACE, 
        CHAR_I, CHAR_N, CHAR_I, CHAR_T, CHAR_SPACE, CHAR_O, CHAR_K,
        CHAR_NEWLINE, CHAR_A, CHAR_SPACE, CHAR_B, CHAR_U, CHAR_T, CHAR_T, CHAR_O, CHAR_SPACE, CHAR_T, CHAR_O, CHAR_SPACE, CHAR_R, CHAR_E, CHAR_S, CHAR_E, CHAR_T,
        EOS 
    }, 0, 1, 0, 0);
    PutWindowTilemap(0);
    CopyWindowToVram(0, COPYWIN_FULL);
}

void CB2_ReinitMainMenu(void)
{
    CB2_InitMainMenu();
}

static void CB2_Placeholder(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_Placeholder(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void Task_PlaceholderMenu(u8 taskId)
{
    if (JOY_NEW(A_BUTTON))
    {
        DoSoftReset();
    }
}

void CreateYesNoMenuParameterized(u8 x, u8 y, u16 baseTileNum, u16 baseBlock, u8 yesNoPalNum, u8 winPalNum)
{
    struct WindowTemplate template = CreateWindowTemplate(0, x + 1, y + 1, 5, 4, winPalNum, baseBlock);
    CreateYesNoMenu(&template, baseTileNum, yesNoPalNum, 0);
}