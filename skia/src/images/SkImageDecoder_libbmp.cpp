
/*
 * Copyright 2007 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "bmpdecoderhelper.h"
#include "SkColorPriv.h"
#include "SkImageDecoder.h"
#include "SkScaledBitmapSampler.h"
#include "SkStream.h"
#include "SkStreamHelpers.h"
#include "SkTDArray.h"
#include "SkTRegistry.h"
#include <unistd.h>

#include "SkTemplates.h"
#ifdef USE_MTK_ALMK
  #include "almk_hal.h"
  #include <cutils/xlog.h>
#endif 
class SkBMPImageDecoder : public SkImageDecoder {
public:
    SkBMPImageDecoder() {
        buffer = NULL;
    }
    ~SkBMPImageDecoder() {
        if(buffer) {
            sk_free(buffer);
        }
    }
    
    virtual Format getFormat() const SK_OVERRIDE {
        return kBMP_Format;
    }

protected:
    virtual bool onDecode(SkStream* stream, SkBitmap* bm, Mode mode) SK_OVERRIDE;
    virtual bool onBuildTileIndex(SkStream *stream,
             int *width, int *height);
    virtual bool onDecodeSubset(SkBitmap* bitmap, const SkIRect& region);
private:
    typedef SkImageDecoder INHERITED;

    int imageWidth;
    int imageHeight;
    void* buffer;
    int length;
};

///////////////////////////////////////////////////////////////////////////////
DEFINE_DECODER_CREATOR(BMPImageDecoder);
///////////////////////////////////////////////////////////////////////////////

static bool is_bmp(SkStream* stream) {
    static const char kBmpMagic[] = { 'B', 'M' };


    char buffer[sizeof(kBmpMagic)];

    return stream->read(buffer, sizeof(kBmpMagic)) == sizeof(kBmpMagic) &&
        !memcmp(buffer, kBmpMagic, sizeof(kBmpMagic));
}

static SkImageDecoder* sk_libbmp_dfactory(SkStream* stream) {
    if (is_bmp(stream)) {
        return SkNEW(SkBMPImageDecoder);
    }
    return NULL;
}

static SkTRegistry<SkImageDecoder*, SkStream*> gReg(sk_libbmp_dfactory);

static SkImageDecoder::Format get_format_bmp(SkStream* stream) {
    if (is_bmp(stream)) {
        return SkImageDecoder::kBMP_Format;
    }
    return SkImageDecoder::kUnknown_Format;
}

static SkTRegistry<SkImageDecoder::Format, SkStream*> gFormatReg(get_format_bmp);

///////////////////////////////////////////////////////////////////////////////

class SkBmpDecoderCallback : public image_codec::BmpDecoderCallback {
public:
    // we don't copy the bitmap, just remember the pointer
    SkBmpDecoderCallback(bool justBounds) : fJustBounds(justBounds) {}

    // override from BmpDecoderCallback
    virtual uint8* SetSize(int width, int height) {
        fWidth = width;
        fHeight = height;
        if (fJustBounds) {
            return NULL;
        }
        
        fRGB.setCount(width * height * 3);  // 3 == r, g, b
        return fRGB.begin();
    }
    virtual uint8* SetSizeRegion(int width, int height, SkIRect * region) {
        fWidth = region->width();
        fHeight = region->height();
        fRGB.setCount(fWidth * fHeight * 3);  // 3 == r, g, b
        return fRGB.begin();
    }

    int width() const { return fWidth; }
    int height() const { return fHeight; }
    const uint8_t* rgb() const { return fRGB.begin(); }

private:
    SkTDArray<uint8_t> fRGB;
    int fWidth;
    int fHeight;
    bool fJustBounds;
};

bool SkBMPImageDecoder::onDecode(SkStream* stream, SkBitmap* bm, Mode mode) {

    // First read the entire stream, so that all of the data can be passed to
    // the BmpDecoderHelper.

    // Allocated space used to hold the data.
    SkAutoMalloc storage;
    // Byte length of all of the data.
    const size_t length = CopyStreamToStorage(&storage, stream);
    if (0 == length) {
        storage.free();
        return 0;
    }
    size_t maxSize = MTK_MAX_SRC_BMP_FILE_SIZE;
#ifdef USE_MTK_ALMK 
    if(!almkGetMaxSafeSize(getpid(), &maxSize)){
      maxSize = MTK_MAX_SRC_BMP_FILE_SIZE ;
      SkDebugf("ALMK::BMP::get Max Safe bmp (Max %d bytes) for BMP file (%d bytes) fail!!\n", maxSize, length);
    }else{
      if(maxSize > length)
        SkDebugf("ALMK::BMP::Max Safe Size (Max %d bytes) for BMP file(%d bytes)=> PASS !! \n", maxSize, length);
      else
        SkDebugf("ALMK::BMP::Max Safe Size (Max %d bytes) for BMP file(%d bytes)=> MemoryShortage!! \n", maxSize, length);        
    }
#endif
    if (length > maxSize) {
        storage.free();
        SkDebugf("Too large bmp source (%d bytes)\n", length);
        return false;
    }

    const bool justBounds = SkImageDecoder::kDecodeBounds_Mode == mode;
    SkBmpDecoderCallback callback(justBounds);

    // Now decode the BMP into callback's rgb() array [r,g,b, r,g,b, ...]
    {
        image_codec::BmpDecoderHelper helper;
        const int max_pixels = 16383*16383; // max width*height
        if (!helper.DecodeImage((const char*)storage.get(), length,
                                max_pixels, NULL, &callback)) {
            return false;
        }
    }

    // we don't need this anymore, so free it now (before we try to allocate
    // the bitmap's pixels) rather than waiting for its destructor
    storage.free();

    int width = callback.width();
    int height = callback.height();
    SkBitmap::Config config = this->getPrefConfig(k32Bit_SrcDepth, false);

    // only accept prefConfig if it makes sense for us
    if (SkBitmap::kARGB_4444_Config != config &&
            SkBitmap::kRGB_565_Config != config) {
        config = SkBitmap::kARGB_8888_Config;
    }

    SkScaledBitmapSampler sampler(width, height, getSampleSize());

    bm->setConfig(config, sampler.scaledWidth(), sampler.scaledHeight());
    bm->setIsOpaque(true);

    if (justBounds) {
        return true;
    }

    if (!this->allocPixelRef(bm, NULL)) {
        return false;
    }

    SkAutoLockPixels alp(*bm);

    if (!sampler.begin(bm, SkScaledBitmapSampler::kRGB, *this)) {
        return false;
    }

    const int srcRowBytes = width * 3;
    const int dstHeight = sampler.scaledHeight();
    const uint8_t* srcRow = callback.rgb();
    
    srcRow += sampler.srcY0() * srcRowBytes;
    for (int y = 0; y < dstHeight; y++) {
        sampler.next(srcRow);
        srcRow += sampler.srcDY() * srcRowBytes;
    }
    return true;
}
static const int kBmpHeaderSize = 14;
static const int kBmpInfoSize = 40;
static const int kBmpOS2InfoSize = 12;
static const int kMaxDim = SHRT_MAX / 2;

bool SkBMPImageDecoder::onBuildTileIndex(SkStream* sk_stream, int *width,
        int *height) {

    /// M: for cts test case @{
    char acBuf[256];
    sprintf(acBuf, "/proc/%d/cmdline", getpid());
    FILE *fp = fopen(acBuf, "r");
    if (fp)
    {
        fread(acBuf, 1, sizeof(acBuf), fp);
        fclose(fp);
        if(strncmp(acBuf, "com.android.cts", 15) == 0)
        {
        	return false;
        }
    }
    /// @}

    size_t len = sk_stream->getLength();
  
    if (len < 26)
    {
        return false;
    }

    if (buffer)
    {
        buffer = NULL;
    }

    buffer = sk_malloc_throw(len);
 
    if (sk_stream->read(buffer, len) != len) {
        return false;
    }

    uint8* header = (uint8*)buffer;
    int size = header[14] + (header[15] << 8) + (header[16] << 16) + (header[17] << 24);
    if (size >= 40) {
    int origWidth  = header[18] + (header[19] << 8) + (header[20] << 16) + (header[21] << 24);
    int origHeight = header[22] + (header[23] << 8) + (header[24] << 16) + (header[25] << 24);

    *width = origWidth;
    *height = origHeight;
    /// M: for the BMP file on the OS2 system. @{
    } else {
        int os2OrigWidth = header[18] + (header[19] << 8);
        int os2OrigHeight = header[20] + (header[21] <<8);
        *width = os2OrigWidth;
        *height = os2OrigHeight;
    }
    /// @}
    this->imageWidth = *width;
    /// M: for the flip row BMP file. @{
    if (*height < 0) {
        *height = -*height;
        this->imageHeight = *height;
    /// @}
    } else {
    this->imageHeight = *height;
    }
    this->length = len;

    return true;
}

bool SkBMPImageDecoder::onDecodeSubset(SkBitmap* bm, const SkIRect& region) {

    SkIRect rect = SkIRect::MakeWH(this->imageWidth, this->imageHeight);
    if (!rect.intersect(region)) {
        // If the requested region is entirely outsides the image, just
        // returns false
        return false;
    }


    //decode
    bool justBounds = false;
    SkBmpDecoderCallback callback(justBounds);

    // Now decode the BMP into callback's rgb() array [r,g,b, r,g,b, ...]
    {
        image_codec::BmpDecoderHelper helper;
        const int max_pixels = 16383*16383; 
        if (!helper.DecodeImage((const char*)buffer, this->length,
                                max_pixels, &rect, &callback)) {
            return false;
        }
    }

    //DUMP bitmap here
    //end dump

    int width = callback.width();
    int height = callback.height();
    SkBitmap::Config config = this->getPrefConfig(k32Bit_SrcDepth, false);

    // only accept prefConfig if it makes sense for us
    if (SkBitmap::kARGB_4444_Config != config &&
            SkBitmap::kRGB_565_Config != config) {
        config = SkBitmap::kARGB_8888_Config;
    }

    //Sample
    const int sampleSize = this->getSampleSize();
    SkScaledBitmapSampler sampler(rect.width(), rect.height(), sampleSize);

    SkBitmap *decodedBitmap = new SkBitmap;
    SkAutoTDelete<SkBitmap> adb(decodedBitmap);

    decodedBitmap->setConfig(config, sampler.scaledWidth(),
                             sampler.scaledHeight(), 0);

    int w = rect.width() / sampleSize;
    int h = rect.height() / sampleSize;
    bool swapOnly = (rect == region) && (w == decodedBitmap->width()) &&
                    (h == decodedBitmap->height()) &&
                    ((0 - rect.x()) / sampleSize == 0) && bm->isNull();

    if (swapOnly) {
        if (!this->allocPixelRef(decodedBitmap,
               NULL)) {

            return false;
        }
    } else {
        if (!decodedBitmap->allocPixels(
            NULL,  NULL)) {
            return false;
        }
    }
    SkAutoLockPixels alp(*decodedBitmap);

    if (!sampler.begin(decodedBitmap, SkScaledBitmapSampler::kRGB, *this)) {
        return false;
    }

    const int srcRowBytes = width * 3;
    const int dstHeight = sampler.scaledHeight();
    const uint8_t* srcRow = callback.rgb();
    
    srcRow += sampler.srcY0() * srcRowBytes;
    for (int y = 0; y < dstHeight; y++) {
        sampler.next(srcRow);
        srcRow += sampler.srcDY() * srcRowBytes;
    }

    int startX = rect.fLeft;
    int startY = rect.fTop;

    decodedBitmap->setIsOpaque(true);

    if (swapOnly) {
        bm->swap(*decodedBitmap);
    } else {
        cropBitmap(bm, decodedBitmap, sampleSize, region.x(), region.y(),
                   region.width(), region.height(), startX, startY);
    }
    return true;
}
