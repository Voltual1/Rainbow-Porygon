#include "global.h"
#include "dma3.h"

#define MAX_DMA_REQUESTS 128

#define DMA_REQUEST_COPY32 1
#define DMA_REQUEST_FILL32 2
#define DMA_REQUEST_COPY16 3
#define DMA_REQUEST_FILL16 4

struct Dma3Request
{
    const u8 *src;
    u8 *dest;
    u16 size;
    u16 mode;
    u32 value;
};

static struct Dma3Request sDma3Requests[MAX_DMA_REQUESTS];

static vbool8 sDma3ManagerLocked;
static u8 sDma3RequestCursor;

void ClearDma3Requests(void)
{
    int i;

    sDma3ManagerLocked = TRUE;
    sDma3RequestCursor = 0;

    for (i = 0; i < MAX_DMA_REQUESTS; i++)
    {
        sDma3Requests[i].size = 0;
        sDma3Requests[i].src = NULL;
        sDma3Requests[i].dest = NULL;
    }

    sDma3ManagerLocked = FALSE;
}

void ProcessDma3Requests(void)
{
    u16 bytesTransferred;

    if (sDma3ManagerLocked)
        return;

    bytesTransferred = 0;

    while (sDma3Requests[sDma3RequestCursor].size != 0)
    {
        bytesTransferred += sDma3Requests[sDma3RequestCursor].size;

        if (bytesTransferred > 40 * 1024)
            return;
        if (*(u8 *)REG_ADDR_VCOUNT > 224)
            return;

        switch (sDma3Requests[sDma3RequestCursor].mode)
        {
        case DMA_REQUEST_COPY32:
            Dma3CopyLarge32_(sDma3Requests[sDma3RequestCursor].src,
                             sDma3Requests[sDma3RequestCursor].dest,
                             sDma3Requests[sDma3RequestCursor].size);
            break;
        case DMA_REQUEST_FILL32:
            Dma3FillLarge32_(sDma3Requests[sDma3RequestCursor].value,
                             sDma3Requests[sDma3RequestCursor].dest,
                             sDma3Requests[sDma3RequestCursor].size);
            break;
        case DMA_REQUEST_COPY16:
            Dma3CopyLarge16_(sDma3Requests[sDma3RequestCursor].src,
                             sDma3Requests[sDma3RequestCursor].dest,
                             sDma3Requests[sDma3RequestCursor].size);
            break;
        case DMA_REQUEST_FILL16:
            Dma3FillLarge16_(sDma3Requests[sDma3RequestCursor].value,
                             sDma3Requests[sDma3RequestCursor].dest,
                             sDma3Requests[sDma3RequestCursor].size);
            break;
        }

        sDma3Requests[sDma3RequestCursor].src = NULL;
        sDma3Requests[sDma3RequestCursor].dest = NULL;
        sDma3Requests[sDma3RequestCursor].size = 0;
        sDma3Requests[sDma3RequestCursor].mode = 0;
        sDma3Requests[sDma3RequestCursor].value = 0;

        sDma3RequestCursor = INCREMENT_OR_WRAP(sDma3RequestCursor, MAX_DMA_REQUESTS);
    }
}

s16 RequestDma3Copy(const void *src, void *dest, u16 size, u32 mode)
{
    int cursor;
    int i = 0;

    sDma3ManagerLocked = TRUE;
    cursor = sDma3RequestCursor;

    while (i < MAX_DMA_REQUESTS)
    {
        if (sDma3Requests[cursor].size == 0)
        {
            sDma3Requests[cursor].src = src;
            sDma3Requests[cursor].dest = dest;
            sDma3Requests[cursor].size = size;

            if (mode == 1)
                sDma3Requests[cursor].mode = DMA_REQUEST_COPY32;
            else
                sDma3Requests[cursor].mode = DMA_REQUEST_COPY16;

            sDma3ManagerLocked = FALSE;
            return cursor;
        }

        cursor = INCREMENT_OR_WRAP(cursor, MAX_DMA_REQUESTS);
        i++;
    }
    sDma3ManagerLocked = FALSE;
    return -1;
}

s16 RequestDma3Fill(s32 value, void *dest, u16 size, u32 mode)
{
    int cursor;
    int i = 0;

    cursor = sDma3RequestCursor;
    sDma3ManagerLocked = TRUE;

    while (i < MAX_DMA_REQUESTS)
    {
        if (sDma3Requests[cursor].size == 0)
        {
            sDma3Requests[cursor].dest = dest;
            sDma3Requests[cursor].size = size;
            sDma3Requests[cursor].mode = mode;
            sDma3Requests[cursor].value = value;

            if (mode == 1)
                sDma3Requests[cursor].mode = DMA_REQUEST_FILL32;
            else
                sDma3Requests[cursor].mode = DMA_REQUEST_FILL16;

            sDma3ManagerLocked = FALSE;
            return cursor;
        }

        cursor = INCREMENT_OR_WRAP(cursor, MAX_DMA_REQUESTS);
        i++;
    }
    sDma3ManagerLocked = FALSE;
    return -1;
}

s16 CheckForSpaceForDma3Request(s16 index)
{
#ifdef PORTABLE
    ProcessDma3Requests();
#endif

    int i = 0;

    if (index == -1)
    {
        while (i < MAX_DMA_REQUESTS)
        {
            if (sDma3Requests[i].size != 0)
                return -1;
            i++;
        }
        return 0;
    }
    else
    {
        if (sDma3Requests[index].size != 0)
            return -1;
        return 0;
    }
}