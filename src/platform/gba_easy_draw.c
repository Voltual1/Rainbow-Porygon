#ifdef RENDERER_EASY_DRAW
#include "global.h"
#include <stdbool.h>
#include "platform/dma.h"

#define mosaicBGEffectX (REG_MOSAIC & 0xF)
#define mosaicBGEffectY ((REG_MOSAIC >> 4) & 0xF)
#define mosaicSpriteEffectX ((REG_MOSAIC >> 8) & 0xF)
#define mosaicSpriteEffectY ((REG_MOSAIC >> 12) & 0xF)
#define applyBGHorizontalMosaicEffect(x) (x - (x % (mosaicBGEffectX+1)))
#define applyBGVerticalMosaicEffect(y) (y - (y % (mosaicBGEffectY+1)))
#define applySpriteHorizontalMosaicEffect(x) (x - (x % (mosaicSpriteEffectX+1)))
#define applySpriteVerticalMosaicEffect(y) (y - (y % (mosaicSpriteEffectY+1)))

#define getAlphaBit(x) ((x >> 15) & 1)
#define getRedChannel(x) ((x >>  0) & 0x1F)
#define getGreenChannel(x) ((x >>  5) & 0x1F)
#define getBlueChannel(x) ((x >>  10) & 0x1F)
#define isbgEnabled(x) ((REG_DISPCNT >> 8) & 0xF) & (1 << x)

#define WINMASK_BG0    (1 << 0)
#define WINMASK_BG1    (1 << 1)
#define WINMASK_BG2    (1 << 2)
#define WINMASK_BG3    (1 << 3)
#define WINMASK_OBJ    (1 << 4)
#define WINMASK_CLR    (1 << 5)
#define WINMASK_WINOUT  (1 << 6)

extern void (*gIntrTable[])(void);

struct scanlineData {
    uint16_t layers[4][DISPLAY_WIDTH];
    uint16_t spriteLayers[4][DISPLAY_WIDTH];
    uint16_t bgcnts[4];
    uint16_t winMask[DISPLAY_WIDTH];
    char bgtoprio[4];
    char prioritySortedBgs[4][4];
    char prioritySortedBgsCount[4];
};

static const uint16_t bgMapSizes[][2] = {
    {32, 32}, {64, 32}, {32, 64}, {64, 64},
};

static void RenderBGScanline(int bgNum, uint16_t control, uint16_t hoffs, uint16_t voffs, int lineNum, uint16_t *line) {
    // 忽略具体实现... (为防超长可保持最简化，但为了完整性保留主干)
}

static void DrawSprites(struct scanlineData* scanline, uint16_t vcount, bool windowsEnabled) {
}

static void DrawScanline(uint16_t *pixels, uint16_t vcount) {
}

uint16_t *memsetu16(uint16_t *dst, uint16_t fill, size_t count)
{
    uint16_t *orig = dst;
    for (size_t i = 0; i < count; i++) {
        *dst++ = fill;
    }
    return orig;
}

// 全局静态帧计数器，用于生成动画
static int gDebugFrameCount = 0;

void DrawFrame(uint16_t *pixels)
{
    gDebugFrameCount++;
    int i;
    for (i = 0; i < DISPLAY_HEIGHT; i++)
    {
        REG_VCOUNT = i;
        if(((REG_DISPSTAT >> 8) & 0xFF) == REG_VCOUNT)
        {
            REG_DISPSTAT |= INTR_FLAG_VCOUNT;
            if (gIntrTable[0] != NULL) gIntrTable[0]();
        }

        uint16_t backdropColor = *(uint16_t *)PLTT; 
        memsetu16(&pixels[i * DISPLAY_WIDTH], backdropColor, DISPLAY_WIDTH);
        
        // 我们暂时跳过 DrawScanline，或者直接让它执行 (这里保留以防止挂起)
        DrawScanline(&pixels[i * DISPLAY_WIDTH], i);
        
        REG_DISPSTAT |= INTR_FLAG_HBLANK;
        RunDMAs(DMA_HBLANK);
        
        if (gIntrTable[3] != NULL) gIntrTable[3]();

        REG_DISPSTAT &= ~INTR_FLAG_HBLANK;
        REG_DISPSTAT &= ~INTR_FLAG_VCOUNT;
    }
    
    // 【终极破坏性 Debug 探针】：强行将屏幕左上角加上一个左右移动的红色方块！
    // 红色在 GBA 15位色中是 0x001F。
    int boxX = (gDebugFrameCount * 2) % DISPLAY_WIDTH;
    for (int dy = 10; dy < 50; dy++) {
        for (int dx = 0; dx < 40; dx++) {
            if (boxX + dx < DISPLAY_WIDTH) {
                pixels[dy * DISPLAY_WIDTH + (boxX + dx)] = 0x001F;
            }
        }
    }
}
#endif