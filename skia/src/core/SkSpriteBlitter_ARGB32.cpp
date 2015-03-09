
/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "SkSpriteBlitter.h"
#include "SkBlitRow.h"
#include "SkColorFilter.h"
#include "SkColorPriv.h"
#include "SkTemplates.h"
#include "SkUtils.h"
#include "SkXfermode.h"

///////////////////////////////////////////////////////////////////////////////

class Sprite_D32_S32 : public SkSpriteBlitter {
public:
    Sprite_D32_S32(const SkBitmap& src, U8CPU alpha)  : INHERITED(src) {
        SkASSERT(src.config() == SkBitmap::kARGB_8888_Config);

        unsigned flags32 = 0;
        if (255 != alpha) {
            flags32 |= SkBlitRow::kGlobalAlpha_Flag32;
        }
        if (!src.isOpaque()) {
            flags32 |= SkBlitRow::kSrcPixelAlpha_Flag32;
        }

        fProc32 = SkBlitRow::Factory32(flags32);
        fAlpha = alpha;
    }

    virtual void blitRect(int x, int y, int width, int height) {
        SkASSERT(width > 0 && height > 0);
        uint32_t* SK_RESTRICT dst = fDevice->getAddr32(x, y);
        const uint32_t* SK_RESTRICT src = fSource->getAddr32(x - fLeft,
                                                             y - fTop);
        size_t dstRB = fDevice->rowBytes();
        size_t srcRB = fSource->rowBytes();
        SkBlitRow::Proc32 proc = fProc32;
        U8CPU             alpha = fAlpha;

        do {
            proc(dst, src, width, alpha);
            dst = (uint32_t* SK_RESTRICT)((char*)dst + dstRB);
            src = (const uint32_t* SK_RESTRICT)((const char*)src + srcRB);
        } while (--height != 0);
    }

private:
    SkBlitRow::Proc32   fProc32;
    U8CPU               fAlpha;

    typedef SkSpriteBlitter INHERITED;
};

///////////////////////////////////////////////////////////////////////////////

static void S16_Opaque_D32(SkPMColor* SK_RESTRICT dst,
                                 const uint16_t* SK_RESTRICT src,
                                 int count, U8CPU alpha) {
    SkASSERT(255 == alpha);
    if (count > 0) {
#if defined(__ARM_HAVE_NEON) && defined(SK_CPU_LENDIAN)
        asm volatile (
                      "cmp        %[count], #8                    \n\t"
                      "vmov.u16   d6, #0xFF00                     \n\t"   // Load alpha value into q8 for later use.
                      "blt        2f                              \n\t"   // if count < 0, branch to label 2
                      "vmov.u8    d5, #0xFF                       \n\t"                
                      "sub        %[count], %[count], #8          \n\t"   // count -= 8

                      "1:                                         \n\t"   // 8 loop
                      // Handle 8 pixels in one loop.
                      "vld1.u16   {q0}, [%[src]]!                 \n\t"   // load eight src 565 pixels
                      "pld        [%[src], #64]                   \n\t"
                      "subs       %[count], %[count], #8          \n\t"   // count -= 8, set flag
                      "vshrn.u16  d2, q0, #8                      \n\t" //R
                      "vshrn.u16  d3, q0, #3                      \n\t" //G
                      "vsli.u16   q0, q0, #5                      \n\t" //B
                      "vsri.u8    d2, d2, #5                      \n\t"
                      "vsri.u8    d3, d3, #6                      \n\t"
                      "vshrn.u16  d4, q0, #2                      \n\t"
                      "vst4.u8    {d2, d3, d4, d5}, [%[dst]]!     \n\t"

                      "bge        1b                              \n\t"   // loop if count >= 0
                      "add        %[count], %[count], #8          \n\t"

                      "2:                                         \n\t"   // exit of 8 loop
                      "cmp        %[count], #4                    \n\t"   //
                      "blt        3f                              \n\t"   // if count < 0, branch to label 3
                      "sub        %[count], %[count], #4          \n\t"
                      // Handle 4 pixels at once
                      "vld1.u16   {d0}, [%[src]]!                 \n\t"   // load eight src 565 pixels

                      "vshl.u16   d2, d0, #5                      \n\t"   // put green in the 6 high bits of d2
                      "vshl.u16   d1, d0, #11                     \n\t"   // put blue in the 5 high bits of d1
                      "vmov.u16   d3, d6                          \n\t"   // copy alpha from d6
                      "vsri.u16   d3, d1, #8                      \n\t"   // put blue below alpha in d3
                      "vsri.u16   d3, d1, #13                     \n\t"   // put 3 MSB blue below blue in d3
                      "vsri.u16   d2, d2, #6                      \n\t"   // put 2 MSB green below green in d2
                      "vsri.u16   d2, d0, #8                      \n\t"   // put red below green in d2
                      "vsri.u16   d2, d0, #13                     \n\t"   // put 3 MSB red below red in d2
                      "vzip.16    d2, d3                          \n\t"   // interleave d2 and d3
                      "vst1.16    {d2, d3}, [%[dst]]!             \n\t"   // store d2 and d3 to dst

                      "3:                                         \n\t"   // end
                      : [src] "+r" (src), [dst] "+r" (dst), [count] "+r" (count)
                      :
                      : "cc", "memory","d0","d1","d2","d3","d4","d5","d6"
                     );

        for (int i = (count & 3); i > 0; --i) {
            *dst = SkPixel16ToPixel32(*src);
            src += 1;
            dst += 1;
        }

#else
        do {
            *dst = SkPixel16ToPixel32(*src);
            src += 1;
            dst += 1;
        } while (--count > 0);
#endif
    }
}

static void S16_Blend_D32(SkPMColor* SK_RESTRICT dst,
                                 const uint16_t* SK_RESTRICT src,
                                 int count, U8CPU alpha) {
    SkASSERT(alpha <= 255);
    if (count > 0) {
        do {
            *dst = SkAlphaMulQ(SkPixel16ToPixel32(*src), alpha);
            src += 1;
            dst += 1;
        } while (--count > 0);
    }
}

class Sprite_D32_S16 : public SkSpriteBlitter {
public:
    Sprite_D32_S16(const SkBitmap& src, U8CPU alpha)  : INHERITED(src) {
        SkASSERT(src.config() == SkBitmap::kRGB_565_Config);

        if (255 != alpha) {
            fProc32 = S16_Blend_D32;
        }
        else {
            fProc32 = S16_Opaque_D32;
        }
        fAlpha = alpha;
    }

    virtual void blitRect(int x, int y, int width, int height) {
        SkASSERT(width > 0 && height > 0);
        uint32_t* SK_RESTRICT dst = fDevice->getAddr32(x, y);
        const uint16_t* SK_RESTRICT src = fSource->getAddr16(x - fLeft,
                                                             y - fTop);
        size_t dstRB = fDevice->rowBytes();
        size_t srcRB = fSource->rowBytes();
        Proc_S16_D32 proc = fProc32;
        U8CPU  alpha = fAlpha;

        do {
            proc(dst, src, width, alpha);
            dst = (uint32_t* SK_RESTRICT)((char*)dst + dstRB);
            src = (const uint16_t* SK_RESTRICT)((const char*)src + srcRB);
        } while (--height != 0);
    }

private:
    typedef void (*Proc_S16_D32)(uint32_t* dst,
                         const uint16_t* src,
                         int count, U8CPU alpha);


    Proc_S16_D32   fProc32;
    U8CPU               fAlpha;

    typedef SkSpriteBlitter INHERITED;
};

///////////////////////////////////////////////////////////////////////////////

class Sprite_D32_XferFilter : public SkSpriteBlitter {
public:
    Sprite_D32_XferFilter(const SkBitmap& source, const SkPaint& paint)
        : SkSpriteBlitter(source) {
        fColorFilter = paint.getColorFilter();
        SkSafeRef(fColorFilter);

        fXfermode = paint.getXfermode();
        SkSafeRef(fXfermode);

        fBufferSize = 0;
        fBuffer = NULL;

        unsigned flags32 = 0;
        if (255 != paint.getAlpha()) {
            flags32 |= SkBlitRow::kGlobalAlpha_Flag32;
        }
        if (!source.isOpaque()) {
            flags32 |= SkBlitRow::kSrcPixelAlpha_Flag32;
        }

        fProc32 = SkBlitRow::Factory32(flags32);
        fAlpha = paint.getAlpha();
    }

    virtual ~Sprite_D32_XferFilter() {
        delete[] fBuffer;
        SkSafeUnref(fXfermode);
        SkSafeUnref(fColorFilter);
    }

    virtual void setup(const SkBitmap& device, int left, int top,
                       const SkPaint& paint) {
        this->INHERITED::setup(device, left, top, paint);

        int width = device.width();
        if (width > fBufferSize) {
            fBufferSize = width;
            delete[] fBuffer;
            fBuffer = new SkPMColor[width];
        }
    }

protected:
    SkColorFilter*      fColorFilter;
    SkXfermode*         fXfermode;
    int                 fBufferSize;
    SkPMColor*          fBuffer;
    SkBlitRow::Proc32   fProc32;
    U8CPU               fAlpha;

private:
    typedef SkSpriteBlitter INHERITED;
};

///////////////////////////////////////////////////////////////////////////////

class Sprite_D32_S32A_XferFilter : public Sprite_D32_XferFilter {
public:
    Sprite_D32_S32A_XferFilter(const SkBitmap& source, const SkPaint& paint)
        : Sprite_D32_XferFilter(source, paint) {}

    virtual void blitRect(int x, int y, int width, int height) {
        SkASSERT(width > 0 && height > 0);
        uint32_t* SK_RESTRICT dst = fDevice->getAddr32(x, y);
        const uint32_t* SK_RESTRICT src = fSource->getAddr32(x - fLeft,
                                                             y - fTop);
        size_t dstRB = fDevice->rowBytes();
        size_t srcRB = fSource->rowBytes();
        SkColorFilter* colorFilter = fColorFilter;
        SkXfermode* xfermode = fXfermode;

        do {
            const SkPMColor* tmp = src;

            if (NULL != colorFilter) {
                colorFilter->filterSpan(src, width, fBuffer);
                tmp = fBuffer;
            }

            if (NULL != xfermode) {
                xfermode->xfer32(dst, tmp, width, NULL);
            } else {
                fProc32(dst, tmp, width, fAlpha);
            }

            dst = (uint32_t* SK_RESTRICT)((char*)dst + dstRB);
            src = (const uint32_t* SK_RESTRICT)((const char*)src + srcRB);
        } while (--height != 0);
    }

private:
    typedef Sprite_D32_XferFilter INHERITED;
};

static void fillbuffer(SkPMColor* SK_RESTRICT dst,
                       const SkPMColor16* SK_RESTRICT src, int count) {
    SkASSERT(count > 0);

    do {
        *dst++ = SkPixel4444ToPixel32(*src++);
    } while (--count != 0);
}

class Sprite_D32_S4444_XferFilter : public Sprite_D32_XferFilter {
public:
    Sprite_D32_S4444_XferFilter(const SkBitmap& source, const SkPaint& paint)
        : Sprite_D32_XferFilter(source, paint) {}

    virtual void blitRect(int x, int y, int width, int height) {
        SkASSERT(width > 0 && height > 0);
        SkPMColor* SK_RESTRICT dst = fDevice->getAddr32(x, y);
        const SkPMColor16* SK_RESTRICT src = fSource->getAddr16(x - fLeft,
                                                                y - fTop);
        size_t dstRB = fDevice->rowBytes();
        size_t srcRB = fSource->rowBytes();
        SkPMColor* SK_RESTRICT buffer = fBuffer;
        SkColorFilter* colorFilter = fColorFilter;
        SkXfermode* xfermode = fXfermode;

        do {
            fillbuffer(buffer, src, width);

            if (NULL != colorFilter) {
                colorFilter->filterSpan(buffer, width, buffer);
            }
            if (NULL != xfermode) {
                xfermode->xfer32(dst, buffer, width, NULL);
            } else {
                fProc32(dst, buffer, width, fAlpha);
            }

            dst = (SkPMColor* SK_RESTRICT)((char*)dst + dstRB);
            src = (const SkPMColor16* SK_RESTRICT)((const char*)src + srcRB);
        } while (--height != 0);
    }

private:
    typedef Sprite_D32_XferFilter INHERITED;
};

///////////////////////////////////////////////////////////////////////////////

static void src_row(SkPMColor* SK_RESTRICT dst,
                    const SkPMColor16* SK_RESTRICT src, int count) {
    do {
        *dst = SkPixel4444ToPixel32(*src);
        src += 1;
        dst += 1;
    } while (--count != 0);
}

class Sprite_D32_S4444_Opaque : public SkSpriteBlitter {
public:
    Sprite_D32_S4444_Opaque(const SkBitmap& source) : SkSpriteBlitter(source) {}

    virtual void blitRect(int x, int y, int width, int height) {
        SkASSERT(width > 0 && height > 0);
        SkPMColor* SK_RESTRICT dst = fDevice->getAddr32(x, y);
        const SkPMColor16* SK_RESTRICT src = fSource->getAddr16(x - fLeft,
                                                                y - fTop);
        size_t dstRB = fDevice->rowBytes();
        size_t srcRB = fSource->rowBytes();

        do {
            src_row(dst, src, width);
            dst = (SkPMColor* SK_RESTRICT)((char*)dst + dstRB);
            src = (const SkPMColor16* SK_RESTRICT)((const char*)src + srcRB);
        } while (--height != 0);
    }
};

static void srcover_row(SkPMColor* SK_RESTRICT dst,
                        const SkPMColor16* SK_RESTRICT src, int count) {
    do {
        *dst = SkPMSrcOver(SkPixel4444ToPixel32(*src), *dst);
        src += 1;
        dst += 1;
    } while (--count != 0);
}

class Sprite_D32_S4444 : public SkSpriteBlitter {
public:
    Sprite_D32_S4444(const SkBitmap& source) : SkSpriteBlitter(source) {}

    virtual void blitRect(int x, int y, int width, int height) {
        SkASSERT(width > 0 && height > 0);
        SkPMColor* SK_RESTRICT dst = fDevice->getAddr32(x, y);
        const SkPMColor16* SK_RESTRICT src = fSource->getAddr16(x - fLeft,
                                                                y - fTop);
        size_t dstRB = fDevice->rowBytes();
        size_t srcRB = fSource->rowBytes();

        do {
            srcover_row(dst, src, width);
            dst = (SkPMColor* SK_RESTRICT)((char*)dst + dstRB);
            src = (const SkPMColor16* SK_RESTRICT)((const char*)src + srcRB);
        } while (--height != 0);
    }
};

///////////////////////////////////////////////////////////////////////////////

#include "SkTemplatesPriv.h"

SkSpriteBlitter* SkSpriteBlitter::ChooseD32(const SkBitmap& source,
                                            const SkPaint& paint,
                                            void* storage, size_t storageSize) {
    if (paint.getMaskFilter() != NULL) {
        return NULL;
    }

    U8CPU       alpha = paint.getAlpha();
    SkXfermode* xfermode = paint.getXfermode();
    SkColorFilter* filter = paint.getColorFilter();
    SkSpriteBlitter* blitter = NULL;

    switch (source.getConfig()) {
        case SkBitmap::kARGB_4444_Config:
            if (alpha != 0xFF) {
                return NULL;    // we only have opaque sprites
            }
            if (xfermode || filter) {
                SK_PLACEMENT_NEW_ARGS(blitter, Sprite_D32_S4444_XferFilter,
                                      storage, storageSize, (source, paint));
            } else if (source.isOpaque()) {
                SK_PLACEMENT_NEW_ARGS(blitter, Sprite_D32_S4444_Opaque,
                                      storage, storageSize, (source));
            } else {
                SK_PLACEMENT_NEW_ARGS(blitter, Sprite_D32_S4444,
                                      storage, storageSize, (source));
            }
            break;
        case SkBitmap::kARGB_8888_Config:
            if (xfermode || filter) {
                if (255 == alpha) {
                    // this can handle xfermode or filter, but not alpha
                    SK_PLACEMENT_NEW_ARGS(blitter, Sprite_D32_S32A_XferFilter,
                                      storage, storageSize, (source, paint));
                }
            } else {
                // this can handle alpha, but not xfermode or filter
                SK_PLACEMENT_NEW_ARGS(blitter, Sprite_D32_S32,
                              storage, storageSize, (source, alpha));
            }
            break;
        case SkBitmap::kRGB_565_Config:
            if (!xfermode && !filter)
            {
                SK_PLACEMENT_NEW_ARGS(blitter, Sprite_D32_S16,
                              storage, storageSize, (source, alpha));
            }
            break;
        default:
            break;
    }
    return blitter;
}
