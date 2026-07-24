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
    //priority bookkeeping
    char bgtoprio[4]; //background to priority
    char prioritySortedBgs[4][4];
    char prioritySortedBgsCount[4];
};

struct bgPriority {
    char priority;
    char subPriority;
};

static const uint16_t bgMapSizes[][2] =
{
    {32, 32},
    {64, 32},
    {32, 64},
    {64, 64},
};

static void RenderBGScanline(int bgNum, uint16_t control, uint16_t hoffs, uint16_t voffs, int lineNum, uint16_t *line)
{
    unsigned int charBaseBlock = (control >> 2) & 3;
    unsigned int screenBaseBlock = (control >> 8) & 0x1F;
    unsigned int bitsPerPixel = ((control >> 7) & 1) ? 8 : 4;
    unsigned int mapWidth = bgMapSizes[control >> 14][0];
    unsigned int mapHeight = bgMapSizes[control >> 14][1];
    unsigned int mapWidthInPixels = mapWidth * 8;
    unsigned int mapHeightInPixels = mapHeight * 8;

    uint8_t *bgtiles = (uint8_t *)BG_CHAR_ADDR(charBaseBlock);
    uint16_t *pal = (uint16_t *)PLTT;
     
    if (control & BGCNT_MOSAIC)
        lineNum = applyBGVerticalMosaicEffect(lineNum);

    hoffs &= 0x1FF;
    voffs &= 0x1FF;

    for (unsigned int x = 0; x < DISPLAY_WIDTH; x++)
    {
        uint16_t *bgmap = (uint16_t *)BG_SCREEN_ADDR(screenBaseBlock);
        // adjust for scroll
        unsigned int xx;
        if (control & BGCNT_MOSAIC)
            xx = (applyBGHorizontalMosaicEffect(x) + hoffs) & 0x1FF;
        else
            xx = (x + hoffs) & 0x1FF;
        
        unsigned int yy = (lineNum + voffs) & 0x1FF;
        
        //if x or y go above 255 pixels it goes to the next screen base which are 0x400 WORDs long
        if (xx > 255 && mapWidthInPixels > 256) {
            bgmap += 0x400;
        }
        
        if (yy > 255 && mapHeightInPixels > 256) {
            //the width check is for 512x512 mode support, it jumps by two screen bases instead
            bgmap += (mapWidthInPixels > 256) ? 0x800 : 0x400;
        }
        
        //maximum width for bgtile block is 256
        xx &= 0xFF;
        yy &= 0xFF;

        unsigned int mapX = xx / 8;
        unsigned int mapY = yy / 8;
        uint16_t entry = bgmap[mapY * 32 + mapX];

        unsigned int tileNum = entry & 0x3FF;
        unsigned int paletteNum = (entry >> 12) & 0xF;
        
        unsigned int tileX = xx % 8;
        unsigned int tileY = yy % 8;

        // Flip if necessary
        if (entry & (1 << 10))
            tileX = 7 - tileX;
        if (entry & (1 << 11))
            tileY = 7 - tileY;

        uint16_t tileLoc = tileNum * (bitsPerPixel * 8);
        uint16_t tileLocY = tileY * bitsPerPixel;
        uint16_t tileLocX = tileX;
        if (bitsPerPixel == 4)
            tileLocX /= 2;

        uint8_t pixel = bgtiles[tileLoc + tileLocY + tileLocX];

        if (bitsPerPixel == 4) {
            if (tileX & 1)
                pixel >>= 4;
            else
                pixel &= 0xF;

            if (pixel != 0)
                line[x] = pal[16 * paletteNum + pixel] | 0x8000;
        }
        else {
            line[x] = pal[pixel] | 0x8000;
        }
    }
}

static inline uint32_t getBgX(int bgNumber)
{
    if (bgNumber == 2) return REG_BG2X;
    else if (bgNumber == 3) return REG_BG3X;
    return 0;
}

static inline uint32_t getBgY(int bgNumber)
{
    if (bgNumber == 2) return REG_BG2Y;
    else if (bgNumber == 3) return REG_BG3Y;
    return 0;
}

static inline uint16_t getBgPA(int bgNumber)
{
    if (bgNumber == 2) return REG_BG2PA;
    else if (bgNumber == 3) return REG_BG3PA;
    return 0;
}

static inline uint16_t getBgPB(int bgNumber)
{
    if (bgNumber == 2) return REG_BG2PB;
    else if (bgNumber == 3) return REG_BG3PB;
    return 0;
}

static inline uint16_t getBgPC(int bgNumber)
{
    if (bgNumber == 2) return REG_BG2PC;
    else if (bgNumber == 3) return REG_BG3PC;
    return 0;
}

static inline uint16_t getBgPD(int bgNumber)
{
    if (bgNumber == 2) return REG_BG2PD;
    else if (bgNumber == 3) return REG_BG3PD;
    return 0;
}

static void RenderRotScaleBGScanline(int bgNum, uint16_t control, uint16_t x, uint16_t y, int lineNum, uint16_t *line)
{
    vBgCnt *bgcnt = (vBgCnt *)&control;
    unsigned int charBaseBlock = bgcnt->charBaseBlock;
    unsigned int screenBaseBlock = bgcnt->screenBaseBlock;
    unsigned int mapWidth = 1 << (4 + (bgcnt->screenSize)); // number of tiles

    uint8_t *bgtiles = (uint8_t *)(VRAM_ + charBaseBlock * 0x4000);
    uint8_t *bgmap = (uint8_t *)(VRAM_ + screenBaseBlock * 0x800);
    uint16_t *pal = (uint16_t *)PLTT;

    if (control & BGCNT_MOSAIC)
        lineNum = applyBGVerticalMosaicEffect(lineNum);
    

    s16 pa = getBgPA(bgNum);
    s16 pb = getBgPB(bgNum);
    s16 pc = getBgPC(bgNum);
    s16 pd = getBgPD(bgNum);

    int sizeX = 128;
    int sizeY = 128;

    switch (bgcnt->screenSize)
    {
    case 0:
        break;
    case 1:
        sizeX = sizeY = 256;
        break;
    case 2:
        sizeX = sizeY = 512;
        break;
    case 3:
        sizeX = sizeY = 1024;
        break;
    }

    int maskX = sizeX - 1;
    int maskY = sizeY - 1;

    int yshift = ((control >> 14) & 3) + 4;

    s32 currentX = getBgX(bgNum);
    s32 currentY = getBgY(bgNum);
    //sign extend 28 bit number
    currentX = ((currentX & (1 << 27)) ? currentX | 0xF0000000 : currentX);
    currentY = ((currentY & (1 << 27)) ? currentY | 0xF0000000 : currentY);

    currentX += lineNum * pb;
    currentY += lineNum * pd;

    int realX = currentX;
    int realY = currentY;

    if (bgcnt->areaOverflowMode)
    {
        for (int px = 0; px < DISPLAY_WIDTH; px++)
        {
            int xxx = (realX >> 8) & maskX;
            int yyy = (realY >> 8) & maskY;

            int tile = bgmap[(xxx >> 3) + ((yyy >> 3) << yshift)];

            int tileX = xxx & 7;
            int tileY = yyy & 7;

            uint8_t pixel = bgtiles[(tile << 6) + (tileY << 3) + tileX];

            if (pixel != 0) {
                line[px] = pal[pixel] | 0x8000;
            }

            realX += pa;
            realY += pc;
        }
    }
    else
    {
        for (int px = 0; px < DISPLAY_WIDTH; px++)
        {
            int xxx = (realX >> 8);
            int yyy = (realY >> 8);

            if (xxx < 0 || yyy < 0 || xxx >= sizeX || yyy >= sizeY)
            {
                //line[x] = 0x80000000;
            }
            else
            {
                int tile = bgmap[(xxx >> 3) + ((yyy >> 3) << yshift)];

                int tileX = xxx & 7;
                int tileY = yyy & 7;

                uint8_t pixel = bgtiles[(tile << 6) + (tileY << 3) + tileX];

                if (pixel != 0) {
                    line[px] = pal[pixel] | 0x8000;
                }
            }
            realX += pa;
            realY += pc;
        }
    }
    //the only way i could figure out how to get accurate mosaic on affine bgs 
    if (control & BGCNT_MOSAIC && mosaicBGEffectX > 0)
    {
        for (int px = 0; px < DISPLAY_WIDTH; px++)
        {
            uint16_t color = line[applyBGHorizontalMosaicEffect(px)];
            line[px] = color;
            
        }
    }
}

const u8 spriteSizes[][2] =
{
    {8, 16},
    {8, 32},
    {16, 32},
    {32, 64},
};

static uint16_t alphaBlendColor(uint16_t targetA, uint16_t targetB)
{
    unsigned int eva = REG_BLDALPHA & 0x1F;
    unsigned int evb = (REG_BLDALPHA >> 8) & 0x1F;
    // shift right by 4 = division by 16
    unsigned int r = ((getRedChannel(targetA) * eva) + (getRedChannel(targetB) * evb)) >> 4;
    unsigned int g = ((getGreenChannel(targetA) * eva) + (getGreenChannel(targetB) * evb)) >> 4;
    unsigned int b = ((getBlueChannel(targetA) * eva) + (getBlueChannel(targetB) * evb)) >> 4;
    
    if (r > 31) r = 31;
    if (g > 31) g = 31;
    if (b > 31) b = 31;

     return r | (g << 5) | (b << 10) | (1 << 15);
}

static uint16_t alphaBrightnessIncrease(uint16_t targetA)
{
    unsigned int evy = (REG_BLDY & 0x1F);
    unsigned int r = getRedChannel(targetA) + (31 - getRedChannel(targetA)) * evy / 16;
    unsigned int g = getGreenChannel(targetA) + (31 - getGreenChannel(targetA)) * evy / 16;
    unsigned int b = getBlueChannel(targetA) + (31 - getBlueChannel(targetA)) * evy / 16;
    
    if (r > 31) r = 31;
    if (g > 31) g = 31;
    if (b > 31) b = 31;
    
     return r | (g << 5) | (b << 10) | (1 << 15);
}

static uint16_t alphaBrightnessDecrease(uint16_t targetA)
{
    unsigned int evy = (REG_BLDY & 0x1F);
    unsigned int r = getRedChannel(targetA) - getRedChannel(targetA) * evy / 16;
    unsigned int g = getGreenChannel(targetA) - getGreenChannel(targetA) * evy / 16;
    unsigned int b = getBlueChannel(targetA) - getBlueChannel(targetA) * evy / 16;
    
    if (r > 31) r = 31;
    if (g > 31) g = 31;
    if (b > 31) b = 31;
    
     return r | (g << 5) | (b << 10) | (1 << 15);
}

static bool alphaBlendSelectTargetB(struct scanlineData* scanline, uint16_t* colorOutput, char prnum, char prsub, int pixelpos, bool spriteBlendEnabled)
{   
    for (unsigned int blndprnum = prnum; blndprnum <= 3; blndprnum++)
    {
        if (spriteBlendEnabled == true && getAlphaBit(scanline->spriteLayers[blndprnum][pixelpos]) == 1)
        {
            *colorOutput = scanline->spriteLayers[blndprnum][pixelpos];
            return true;
        }
            
        for (unsigned int blndprsub = prsub; blndprsub < scanline->prioritySortedBgsCount[blndprnum]; blndprsub++)
        {
            char currLayer = scanline->prioritySortedBgs[blndprnum][blndprsub];
            if (getAlphaBit( scanline->layers[currLayer][pixelpos] ) == 1 && REG_BLDCNT & ( 1 << (8 + currLayer)) && isbgEnabled(currLayer))
            {
                *colorOutput = scanline->layers[currLayer][pixelpos];
                return true;
            }
            if ( getAlphaBit( scanline->layers[currLayer][pixelpos] ) == 1 && isbgEnabled(currLayer) && prnum != blndprnum )
            {
                return false;
            }
        }
        prsub = 0; 
    }
    if (REG_BLDCNT & BLDCNT_TGT2_BD)
    {
        *colorOutput = *(uint16_t*)PLTT;
        return true;
    }
    return false;
}

static bool winCheckHorizontalBounds(u16 left, u16 right, u16 xpos)
{
    if (left > right)
        return (xpos >= left || xpos < right);
    else
        return (xpos >= left && xpos < right);
}

static void DrawSprites(struct scanlineData* scanline, uint16_t vcount, bool windowsEnabled)
{
    int i;
    void *objtiles = VRAM_ + 0x10000;
    unsigned int blendMode = (REG_BLDCNT >> 6) & 3;
    bool winShouldBlendPixel = true;

    int16_t matrix[2][2] = {};

    if (!(REG_DISPCNT & (1 << 6)))
    {
        // 2-D OBJ Character mapping not supported.
    }

    for (i = 127; i >= 0; i--)
    {
        struct OamData *oam = &((struct OamData *)OAM)[i];
        unsigned int width;
        unsigned int height;
        uint16_t *pixels;

        bool isAffine  = oam->affineMode & 1;
        bool doubleSizeOrDisabled = (oam->affineMode >> 1) & 1;
        bool isSemiTransparent = (oam->objMode == 1);
        bool isObjWin = (oam->objMode == 2);

        if (!(isAffine) && doubleSizeOrDisabled)
            continue;

        if (oam->shape == 0)
        {
            width = (1 << oam->size) * 8;
            height = (1 << oam->size) * 8;
        }
        else if (oam->shape == 1) // wide
        {
            width = spriteSizes[oam->size][1];
            height = spriteSizes[oam->size][0];
        }
        else if (oam->shape == 2) // tall
        {
            width = spriteSizes[oam->size][0];
            height = spriteSizes[oam->size][1];
        }
        else
        {
            continue; // prohibited
        }

        int rect_width = width;
        int rect_height = height;

        int half_width = width / 2;
        int half_height = height / 2;

        pixels = scanline->spriteLayers[oam->priority];

        int32_t x = oam->x;
        int32_t y = oam->y;

        if (x >= DISPLAY_WIDTH) x -= 512;
        if (y >= DISPLAY_HEIGHT) y -= 256;

        if (isAffine)
        {
            u8 matrixNum = oam->matrixNum * 4;

            struct OamData *oam1 = &((struct OamData *)OAM)[matrixNum];
            struct OamData *oam2 = &((struct OamData *)OAM)[matrixNum + 1];
            struct OamData *oam3 = &((struct OamData *)OAM)[matrixNum + 2];
            struct OamData *oam4 = &((struct OamData *)OAM)[matrixNum + 3];

            matrix[0][0] = oam1->affineParam;
            matrix[0][1] = oam2->affineParam;
            matrix[1][0] = oam3->affineParam;
            matrix[1][1] = oam4->affineParam;

            if (doubleSizeOrDisabled)
            {
                rect_width *= 2;
                rect_height *= 2;
                half_width *= 2;
                half_height *= 2;
            }
        }
        else
        {
            matrix[0][0] = 0x100;
            matrix[0][1] = 0;
            matrix[1][0] = 0;
            matrix[1][1] = 0x100;
        }

        x += half_width;
        y += half_height;

        if (vcount >= (y - half_height) && vcount < (y + half_height))
        {
            int local_y = (oam->mosaic == 1) ? applySpriteVerticalMosaicEffect(vcount) - y : vcount - y;
            bool flipX  = !isAffine && ((oam->matrixNum >> 3) & 1);
            bool flipY  = !isAffine && ((oam->matrixNum >> 4) & 1);
            bool is8BPP  = oam->bpp & 1;

            for (int local_x = -half_width; local_x <= half_width; local_x++)
            {
                uint8_t *tiledata = (uint8_t *)objtiles;
                uint16_t *palette = (uint16_t *)(PLTT + 0x200);
                int local_mosaicX;
                int tex_x;
                int tex_y;

                unsigned int global_x = local_x + x;

                if (global_x < 0 || global_x >= DISPLAY_WIDTH)
                    continue;

                if (oam->mosaic == 1)
                {
                    local_mosaicX = applySpriteHorizontalMosaicEffect(global_x) - x;
                    tex_x = ((matrix[0][0] * local_mosaicX + matrix[0][1] * local_y) >> 8) + (width / 2);
                    tex_y = ((matrix[1][0] * local_mosaicX + matrix[1][1] * local_y) >> 8) + (height / 2);
                }else{
                    tex_x = ((matrix[0][0] * local_x + matrix[0][1] * local_y) >> 8) + (width / 2);
                    tex_y = ((matrix[1][0] * local_x + matrix[1][1] * local_y) >> 8) + (height / 2);
                }

                if (tex_x >= width || tex_y >= height || tex_x < 0 || tex_y < 0)
                    continue;

                if (flipX) tex_x = width  - tex_x - 1;
                if (flipY) tex_y = height - tex_y - 1;

                int tile_x = tex_x % 8;
                int tile_y = tex_y % 8;
                int block_x = tex_x / 8;
                int block_y = tex_y / 8;
                int block_offset = ((block_y * (REG_DISPCNT & 0x40 ? (width / 8) : 16)) + block_x);
                uint16_t pixel = 0;

                if (!is8BPP)
                {
                    pixel = tiledata[(block_offset + oam->tileNum) * 32 + (tile_y * 4) + (tile_x / 2)];
                    if (tile_x & 1)
                        pixel >>= 4;
                    else
                        pixel &= 0xF;
                    palette += oam->paletteNum * 16;
                }
                else
                {
                    pixel = tiledata[(block_offset * 2 + oam->tileNum) * 32 + (tile_y * 8) + tile_x];
                }

                if (pixel != 0)
                {
                    uint16_t color = palette[pixel];
                    
                    if (isObjWin)
                    {
                        if (scanline->winMask[global_x] & WINMASK_WINOUT)
                        scanline->winMask[global_x] = (REG_WINOUT >> 8) & 0x3F;
                        continue;
                    }
                    if (global_x < DISPLAY_WIDTH && global_x >= 0)
                    {
                        winShouldBlendPixel = (windowsEnabled == false || scanline->winMask[global_x] & WINMASK_CLR);
                        
                        if ((blendMode == 1 && REG_BLDCNT & BLDCNT_TGT1_OBJ && winShouldBlendPixel) || isSemiTransparent)
                        {
                            uint16_t targetA = color;
                            uint16_t targetB = 0;
                            if (alphaBlendSelectTargetB(scanline, &targetB, oam->priority, 0, global_x, false))
                            {
                                color = alphaBlendColor(targetA, targetB);
                            }
                        }
                        else if (REG_BLDCNT & BLDCNT_TGT1_OBJ && winShouldBlendPixel)
                        {
                            switch (blendMode)
                            {
                            case 2: color = alphaBrightnessIncrease(color); break;
                            case 3: color = alphaBrightnessDecrease(color); break;
                            }
                        }
                        pixels[global_x] = color | (1 << 15);
                    }
                }
            }
        }
    }
}

static void DrawScanline(uint16_t *pixels, uint16_t vcount)
{
    unsigned int mode = REG_DISPCNT & 3;
    unsigned char numOfBgs = (mode == 0 ? 4 : 3);
    int bgnum, prnum;
    struct scanlineData scanline;
    unsigned int blendMode = (REG_BLDCNT >> 6) & 3;
    unsigned int xpos;

    memset(scanline.layers, 0, sizeof(scanline.layers));
    memset(scanline.winMask, 0, sizeof(scanline.winMask));
    memset(scanline.spriteLayers, 0, sizeof(scanline.spriteLayers));
    memset(scanline.prioritySortedBgsCount, 0, sizeof(scanline.prioritySortedBgsCount));

    for (bgnum = 0; bgnum < numOfBgs; bgnum++)
    {
        uint16_t bgcnt = *(uint16_t*)(REG_ADDR_BG0CNT + bgnum * 2);
        uint16_t priority;
        scanline.bgcnts[bgnum] = bgcnt;
        scanline.bgtoprio[bgnum] = priority = (bgcnt & 3);
        
        char priorityCount = scanline.prioritySortedBgsCount[priority];
        scanline.prioritySortedBgs[priority][priorityCount] = bgnum;
        scanline.prioritySortedBgsCount[priority]++;
    }
    
    switch (mode)
    {
    case 0:
        for (bgnum = 3; bgnum >= 0; bgnum--)
        {
            if (isbgEnabled(bgnum))
            {
                uint16_t bghoffs = *(uint16_t *)(REG_ADDR_BG0HOFS + bgnum * 4);
                uint16_t bgvoffs = *(uint16_t *)(REG_ADDR_BG0VOFS + bgnum * 4);
                RenderBGScanline(bgnum, scanline.bgcnts[bgnum], bghoffs, bgvoffs, vcount, scanline.layers[bgnum]);
            }
        }
        break;
    case 1:
        bgnum = 2;
        if (isbgEnabled(bgnum))
        {
            RenderRotScaleBGScanline(bgnum, scanline.bgcnts[bgnum], REG_BG2X, REG_BG2Y, vcount, scanline.layers[bgnum]);
        }
        for (bgnum = 1; bgnum >= 0; bgnum--)
        {
            if (isbgEnabled(bgnum))
            {
                uint16_t bghoffs = *(uint16_t *)(REG_ADDR_BG0HOFS + bgnum * 4);
                uint16_t bgvoffs = *(uint16_t *)(REG_ADDR_BG0VOFS + bgnum * 4);
                RenderBGScanline(bgnum, scanline.bgcnts[bgnum], bghoffs, bgvoffs, vcount, scanline.layers[bgnum]);
            }
        }
        break;
    default:
        break;
    }
    
    bool windowsEnabled = false;
    uint16_t WIN0bottom, WIN0top, WIN0right, WIN0left;
    uint16_t WIN1bottom, WIN1top, WIN1right, WIN1left;
    bool WIN0enable = false;
    bool WIN1enable = false;

    if (REG_DISPCNT & DISPCNT_WIN0_ON)
    {
        WIN0bottom = (REG_WIN0V & 0xFF);
        WIN0top = (REG_WIN0V & 0xFF00) >> 8;
        WIN0right = (REG_WIN0H & 0xFF);
        WIN0left = (REG_WIN0H & 0xFF00) >> 8;
        
        if (WIN0top > WIN0bottom) {
            if (vcount >= WIN0top || vcount < WIN0bottom) WIN0enable = true;
        } else {
            if (vcount >= WIN0top && vcount < WIN0bottom) WIN0enable = true;
        }
        windowsEnabled = true;
    }
    if (REG_DISPCNT & DISPCNT_WIN1_ON)
    {
        WIN1bottom = (REG_WIN0V & 0xFF);
        WIN1top = (REG_WIN0V & 0xFF00) >> 8;
        WIN1right = (REG_WIN0H & 0xFF);
        WIN1left = (REG_WIN0H & 0xFF00) >> 8;
        
        if (WIN1top > WIN1bottom) {
            if (vcount >= WIN1top || vcount < WIN1bottom) WIN1enable = true;
        } else {
            if (vcount >= WIN1top && vcount < WIN1bottom) WIN1enable = true;
        }
        windowsEnabled = true;
    }
    if (REG_DISPCNT & DISPCNT_OBJWIN_ON && REG_DISPCNT & DISPCNT_OBJ_ON)
    {
        windowsEnabled = true;
    }
    
    if (windowsEnabled)
    {
        for (xpos = 0; xpos < DISPLAY_WIDTH; xpos++)
        {
            if (WIN0enable && winCheckHorizontalBounds(WIN0left, WIN0right, xpos))
                scanline.winMask[xpos] = REG_WININ & 0x3F;
            else if (WIN1enable && winCheckHorizontalBounds(WIN1left, WIN1right, xpos))
                scanline.winMask[xpos] = (REG_WININ >> 8) & 0x3F;
            else
                scanline.winMask[xpos] = (REG_WINOUT & 0x3F) | WINMASK_WINOUT;
        }
    }

    if (REG_DISPCNT & DISPCNT_OBJ_ON)
        DrawSprites(&scanline, vcount, windowsEnabled);

    for (prnum = 3; prnum >= 0; prnum--)
    {
        for (int prsub = scanline.prioritySortedBgsCount[prnum] - 1; prsub >= 0; prsub--)
        {
            char bgnum = scanline.prioritySortedBgs[prnum][prsub];
            if (isbgEnabled(bgnum))
            {
                uint16_t *src = scanline.layers[bgnum];
                for (xpos = 0; xpos < DISPLAY_WIDTH; xpos++)
                {
                    uint16_t color = src[xpos];
                    bool winEffectEnable = true;
                    
                    if (!getAlphaBit(color)) continue;
                    
                    if (windowsEnabled)
                    {
                        winEffectEnable = ((scanline.winMask[xpos] & WINMASK_CLR) >> 5);
                        if ( !(scanline.winMask[xpos] & 1 << bgnum) ) continue;
                    }
                    
                    if (blendMode != 0 && REG_BLDCNT & (1 << bgnum) && winEffectEnable)
                    {
                        uint16_t targetA = color;
                        uint16_t targetB = 0;
                        char isSpriteBlendingEnabled;
                        
                        switch (blendMode)
                        {
                        case 1:
                            isSpriteBlendingEnabled = REG_BLDCNT & BLDCNT_TGT2_OBJ ? 1 : 0;
                            if (alphaBlendSelectTargetB(&scanline, &targetB, prnum, prsub+1, xpos, isSpriteBlendingEnabled))
                            {
                                color = alphaBlendColor(targetA, targetB);
                            }
                            break;
                        case 2: color = alphaBrightnessIncrease(targetA); break;
                        case 3: color = alphaBrightnessDecrease(targetA); break;
                        }
                    }
                    pixels[xpos] = color;
                }
            }
        }
        uint16_t *src = scanline.spriteLayers[prnum];
        for (xpos = 0; xpos < DISPLAY_WIDTH; xpos++)
        {
            if (getAlphaBit(src[xpos]))
            {
                if (windowsEnabled && !(scanline.winMask[xpos] & WINMASK_OBJ))
                        continue;
                pixels[xpos] = src[xpos];
            }
        }
    }
}

uint16_t *memsetu16(uint16_t *dst, uint16_t fill, size_t count)
{
    uint16_t *orig = dst;
    for (size_t i = 0; i < count; i++)
    {
        *dst++ = fill;
    }
    return orig;
}

// 保留你的调试动画计数器
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
#ifdef __ANDROID__
            if (REG_IE & INTR_FLAG_VCOUNT)
#else
            if (REG_DISPSTAT & DISPSTAT_VCOUNT_INTR)
#endif
                if (gIntrTable[0] != NULL) gIntrTable[0]();
        }

        unsigned int blendMode = (REG_BLDCNT >> 6) & 3;
        uint16_t backdropColor = *(uint16_t *)PLTT;
        if (REG_BLDCNT & BLDCNT_TGT1_BD)
        {
            switch (blendMode)
            {
            case 2: backdropColor = alphaBrightnessIncrease(backdropColor); break;
            case 3: backdropColor = alphaBrightnessDecrease(backdropColor); break;
            }
        }

        memsetu16(&pixels[i * DISPLAY_WIDTH], backdropColor, DISPLAY_WIDTH);
        
        // 核心修复：重新启用光栅化逻辑，真正渲染 VRAM 的图块！
        DrawScanline(&pixels[i * DISPLAY_WIDTH], i);
        
        REG_DISPSTAT |= INTR_FLAG_HBLANK;
        RunDMAs(DMA_HBLANK);
        
        if (REG_DISPSTAT & DISPSTAT_HBLANK_INTR)
            if (gIntrTable[3] != NULL) gIntrTable[3]();

        REG_DISPSTAT &= ~INTR_FLAG_HBLANK;
        REG_DISPSTAT &= ~INTR_FLAG_VCOUNT;
    }

    // 在 160 行画完后（正式进入 VBlank），我们应当让 GBA 的 VBlank DMA 被触发！
    // 很多图像数据是在 VBlank 时期 DMA 到 OAM 或 PLTT 上的。
    RunDMAs(DMA_VBLANK);

    // 依然保留红色移动方块（覆盖在图层之上），方便你确认渲染引擎没死
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