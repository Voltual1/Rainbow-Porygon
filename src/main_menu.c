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

#ifdef PORTABLE
extern void SDL_Log(const char *fmt, ...);
#endif

// 占位符界面的最简窗口模板
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

// 模拟基础调色板
static const u16 sPlaceholderPal[] = {
    RGB_BLACK, RGB_WHITE, RGB(12, 12, 12), RGB(26, 26, 25), 
    [15] = RGB_WHITE 
};

void CB2_InitMainMenu(void)
{
#ifdef PORTABLE
    SDL_Log("CAN DEBUG: [CB2_InitMainMenu] Entering Minimal Placeholder Menu.");
#endif

    SetVBlankCallback(NULL);

    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetGpuReg(REG_OFFSET_BG0CNT, 0);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);

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
        CHAR_D, CHAR_E, CHAR_B, CHAR_U, CHAR_G, CHAR_SPACE, 
        CHAR_P, CHAR_L, CHAR_A, CHAR_C, CHAR_E, CHAR_H, CHAR_O, CHAR_L, CHAR_D, CHAR_E, CHAR_R, 
        CHAR_NEWLINE, CHAR_A, CHAR_SPACE, CHAR_B, CHAR_U, CHAR_T, CHAR_T, CHAR_O, CHAR_SPACE, CHAR_R, CHAR_E, CHAR_S, CHAR_E, CHAR_T,
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
#ifdef PORTABLE
        SDL_Log("CAN DEBUG: [Task_PlaceholderMenu] A Pressed. Attempting Soft Reset.");
#endif
        DoSoftReset();
    }
}

// --- CAN FIX: 必须保留的外部引用函数 ---
// 因为 slot_machine.c 等文件引用了这个函数，精简版必须保留它的定义以防止 ld.lld 链接失败
void CreateYesNoMenuParameterized(u8 x, u8 y, u16 baseTileNum, u16 baseBlock, u8 yesNoPalNum, u8 winPalNum)
{
    struct WindowTemplate template = CreateWindowTemplate(0, x + 1, y + 1, 5, 4, winPalNum, baseBlock);
    CreateYesNoMenu(&template, baseTileNum, yesNoPalNum, 0);
}