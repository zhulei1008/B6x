/**
 ****************************************************************************************
 *
 * @file xapi.c
 *
 * @brief Boot Drivers of memory api
 *
 ****************************************************************************************
 */

#include "string.h"
#include <stdbool.h>
#include "b6x.h"
//#include "drvs.h"

/* Nonzero if either X or Y is not aligned on a "uint32_t" boundary.  */
/* Same as (((uint32_t)X | (uint32_t)Y) << 30) */
#define UNALIGNED(X, Y) \
    (((uint32_t)X & (sizeof(uint32_t) - 1)) | ((uint32_t)Y & (sizeof(uint32_t) - 1)))

#define UNALIGN4(X)     ((uint32_t)X & (sizeof(uint32_t) - 1))
    
/* How many bytes are copied each iteration of the 4X unrolled loop.  */
#define BIGBLOCKSIZE    (sizeof(uint32_t) << 2)

/* How many bytes are copied each iteration of the word copy loop.  */
#define LITBLOCKSIZE    (sizeof(uint32_t))

/* Threshhold for punting to the byte copier.  */
#define TOO_SMALL(LEN)  ((LEN) < BIGBLOCKSIZE)

void xmemcpy(void * dst, const void * src, uint32_t size)
{
    uint8_t *byte_dst = (uint8_t *)dst;
    const uint8_t *byte_src = (const uint8_t *)src;
    
    if (!UNALIGNED(dst, src))
    {
        uint32_t *align_dst = (uint32_t *)dst;
        const uint32_t *align_src = (const uint32_t *)src;
        
        #if (1)
        /* Copy 4X uint32_t words at a time if possible.  */
        while (size >= BIGBLOCKSIZE)
        {
            *align_dst++ = *align_src++;
            *align_dst++ = *align_src++;
            *align_dst++ = *align_src++;
            *align_dst++ = *align_src++;
            size -= BIGBLOCKSIZE;
        }
        #endif
        
        /* Copy one uint32_t word at a time if possible.  */
        while (size >= LITBLOCKSIZE)
        {
            *align_dst++ = *align_src++;
            size -= LITBLOCKSIZE;
        }
        
        /* Pick up any residual with a byte copier.  */
        byte_dst = (uint8_t *)align_dst;
        byte_src = (const uint8_t *)align_src;
    }

    while (size--)
    {
        *byte_dst++ = *byte_src++;
    }
}

// same as mc_p.l(memmove.o)
void xmemmove(void * dst, const void * src, uint32_t size)
{
    uint8_t *byte_dst = (uint8_t *)dst;
    const uint8_t *byte_src = (const uint8_t *)src;
    
    if ((int32_t)(byte_dst-byte_src) < size)
    {
        byte_dst += size;
        byte_src += size;
        
        while (size--)
        {
            *--byte_dst = *--byte_src;
        }
    }
    else
    {
        xmemcpy(dst, src, size);
    }
}

void xmemset(void *m, uint8_t c, uint32_t size)
{
    uint8_t *s = (uint8_t *)m;
    
    if (size >= LITBLOCKSIZE*2)
    {
        uint32_t *align;
        uint32_t val32 = (uint8_t)c;
        val32 |= (val32 << 8);
        val32 |= (val32 << 16);
        
        // unalign bytes, max LITBLOCKSIZE-1
        while (UNALIGN4(s))
        {
            *s++ = (uint8_t)c;
            --size;
        }
        
        // aligned words
        align = (uint32_t *)s;

        do {
            *align++ = val32;
            size -= LITBLOCKSIZE;
        } while (size >= LITBLOCKSIZE);
        
        s = (uint8_t *)align;
    }
    
    while (size--)
    {
        *s++ = (uint8_t)c;
    }
}

int xmemcmp(const void *m1, const void *m2, uint32_t size)
{
    const uint8_t *s1 = (const uint8_t *)m1;
    const uint8_t *s2 = (const uint8_t *)m2;

    int ret = 0;
    
#if 1
    // aligned compare
    if ((size >= LITBLOCKSIZE*2) && !UNALIGNED(m1, m2))
    {
        const uint32_t *a1 = (const uint32_t *)m1;
        const uint32_t *a2 = (const uint32_t *)m2;
        
        do {
            //if ((ret = *a1++ - *a2++) != 0)
            //    return ret;
            if (*a1 != *a2) break;
            a1++; a2++;
            
            size -= LITBLOCKSIZE;
        } while (size >= LITBLOCKSIZE);
        
        s1 = (const uint8_t *)a1;
        s2 = (const uint8_t *)a2;
    }
    
    //while (size-- && ((ret = *s1++ - *s2++) == 0));
    for (uint32_t i = 0; i < size; i++)
    {
        ret = s1[i] - s2[i];
        
        if (ret != 0) break;
    }

    return ret;
#else
    // same as mc_p.l(memcmp.o)
    //int ret = 0;
    int i = 0;
    goto cmp;
iadd:
    i++;
cmp:
    if (i < size)
    {
        ret = s1[i] - s2[i];

        if (ret == 0)
        {
            goto iadd;
        }
    }
    return ret;
#endif
}
