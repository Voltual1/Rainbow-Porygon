// MIT License
// 
// Copyright (c) 2026 pokeemerald-multiplatform contributors
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of the original multiplatform-port modifications contributed through this
// fork (the "Port Modifications"), to deal in the Port Modifications without
// restriction, including without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell copies of the Port
// Modifications, and to permit persons to whom the Port Modifications are
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Port Modifications.
// 
// THE PORT MODIFICATIONS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
// EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE PORT MODIFICATIONS OR THE USE OR OTHER
// DEALINGS IN THE PORT MODIFICATIONS.
// 
// Scope
// -----
// 
// This license applies only to original multiplatform-port modifications made by
// contributors to this fork. It does not grant rights to, or relicense:
// 
// - The upstream pokeemerald decompilation or contributions from its authors.
// - Pokemon Emerald, Pokemon characters, names, graphics, audio, story, or other
//   copyrighted or trademarked material owned by Nintendo, Creatures Inc., GAME
//   FREAK inc., or other respective owners.
// - Third-party software included in this repository, which remains subject to
//   its own license terms.
// 
// Users are responsible for determining which portions of a distribution are
// covered by this license and for complying with all applicable third-party and
// upstream terms.
#include "global.h"
#include "platform/dma.h"

#undef DmaSet

struct DMATransfer {
    union {
        const void *src;
        const u16 *src16;
        const u32 *src32;
    };
    union {
        void *dst;
        vu16 *dst16;
        vu32 *dst32;
    };
    u32 size;
    u16 control;
} DMAList[DMA_COUNT];

void RunDMAs(u32 type)
{
    for (int dmaNum = 0; dmaNum < DMA_COUNT; dmaNum++)
    {
        struct DMATransfer *dma = &DMAList[dmaNum];
        u32 dmaCntReg = (&REG_DMA0CNT)[dmaNum * 3];
        if (!((dmaCntReg >> 16) & DMA_ENABLE))
        {
            dma->control &= ~DMA_ENABLE;
        }
        
        if ( (dma->control & DMA_ENABLE) &&
           (((dma->control & DMA_START_MASK) >> 12) == type))
        {
            //printf("DMA%d src=%p, dest=%p, control=%d\n", dmaNum, dma->src, dma->dest, dma->control);
            for (int i = 0; i < (dma->size); i++)
            {
                if ((dma->control) & DMA_32BIT)
                     *dma->dst32 = *dma->src32;
                else *dma->dst16 = *dma->src16;

                // process destination pointer changes
                if (((dma->control) & DMA_DEST_MASK) == DMA_DEST_INC)
                {
                    if ((dma->control) & DMA_32BIT)
                            dma->dst32++;
                    else    dma->dst16++;
                }
                else if (((dma->control) & DMA_DEST_MASK) == DMA_DEST_DEC)
                {
                    if ((dma->control) & DMA_32BIT)
                            dma->dst32--;
                    else    dma->dst16--;
                }
                else if (((dma->control) & DMA_DEST_MASK) == DMA_DEST_RELOAD) // TODO
                {
                    if ((dma->control) & DMA_32BIT)
                            dma->dst32++;
                    else    dma->dst16++;
                }

                // process source pointer changes
                if (((dma->control) & DMA_SRC_MASK) == DMA_SRC_INC)
                {
                    if ((dma->control) & DMA_32BIT)
                            dma->src32++;
                    else    dma->src16++;
                }
                else if (((dma->control) & DMA_SRC_MASK) == DMA_SRC_DEC)
                {
                    if ((dma->control) & DMA_32BIT)
                            dma->src32--;
                    else    dma->src16--;
                }
            }

            if (dma->control & DMA_REPEAT)
            {
                dma->size = ((&REG_DMA0CNT)[dmaNum * 3] & 0x1FFFF);
                if (((dma->control) & DMA_DEST_MASK) == DMA_DEST_RELOAD)
                {
                    dma->dst = (void *)(uintptr_t)((&REG_DMA0DAD)[dmaNum * 3]);
                }
            }
            else
            {
                dma->control &= ~DMA_ENABLE;
            }
        }
    }
}

void DmaSet(int dmaNum, const void *src, void *dest, u32 control)
{
    if (dmaNum >= DMA_COUNT)
    {
        DBGPRINTF("DmaSet with invalid DMA number: dmaNum=%d, src=%p, dest=%p, control=%u\n", dmaNum, src, dest, (unsigned int)control);
        return;
    }

    (&REG_DMA0SAD)[dmaNum * 3] = (vu32)(uintptr_t)src;
    (&REG_DMA0DAD)[dmaNum * 3] = (vu32)(uintptr_t)dest;
    (&REG_DMA0CNT)[dmaNum * 3] = control;

    struct DMATransfer *dma = &DMAList[dmaNum];
    dma->src = src;
    dma->dst = dest;
    dma->size = control & 0x1ffff;
    dma->control = control >> 16;

    RunDMAs(DMA_NOW);
}