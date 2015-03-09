
/*
 * Copyright 2006 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "SkImageDecoder.h"
#include "SkBitmap.h"
#include "SkImagePriv.h"
#include "SkPixelRef.h"
#include "SkStream.h"
#include "SkTemplates.h"
#include "SkCanvas.h"
#include <stdio.h>

SK_DEFINE_INST_COUNT(SkImageDecoder::Peeker)
SK_DEFINE_INST_COUNT(SkImageDecoder::Chooser)
SK_DEFINE_INST_COUNT(SkImageDecoderFactory)

#ifdef MTK_75DISPLAY_ENHANCEMENT_SUPPORT
#include "MediaHal.h"
#endif

#ifdef MTK_89DISPLAY_ENHANCEMENT_SUPPORT
#include "DpBlitStream.h"
#endif

#include <cutils/properties.h>
#include <cutils/xlog.h>

#define LOG_TAG "skia"

#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#include <utils/Trace.h>


static SkBitmap::Config gDeviceConfig = SkBitmap::kNo_Config;

SkBitmap::Config SkImageDecoder::GetDeviceConfig()
{
    return gDeviceConfig;
}

void SkImageDecoder::SetDeviceConfig(SkBitmap::Config config)
{
    gDeviceConfig = config;
}

///////////////////////////////////////////////////////////////////////////////

SkImageDecoder::SkImageDecoder()
    : fPeeker(NULL)
    , fChooser(NULL)
    , fAllocator(NULL)
    , fSampleSize(1)
    , fDefaultPref(SkBitmap::kNo_Config)
    , fDitherImage(true)
    , fUsePrefTable(false)
    , fSkipWritingZeroes(false)
    , fPreferQualityOverSpeed(false)
    , fRequireUnpremultipliedColors(false)
    , fIsAllowMultiThreadRegionDecode(0) 
    , fPreferSize(0)
    , fPostProc(0){
    
}

SkImageDecoder::~SkImageDecoder() {
    SkSafeUnref(fPeeker);
    SkSafeUnref(fChooser);
    SkSafeUnref(fAllocator);
}

void SkImageDecoder::copyFieldsToOther(SkImageDecoder* other) {
    if (NULL == other) {
        return;
    }
    other->setPeeker(fPeeker);
    other->setChooser(fChooser);
    other->setAllocator(fAllocator);
    other->setSampleSize(fSampleSize);
    if (fUsePrefTable) {
        other->setPrefConfigTable(fPrefTable);
    } else {
        other->fDefaultPref = fDefaultPref;
    }
    other->setDitherImage(fDitherImage);
    other->setSkipWritingZeroes(fSkipWritingZeroes);
    other->setPreferQualityOverSpeed(fPreferQualityOverSpeed);
    other->setRequireUnpremultipliedColors(fRequireUnpremultipliedColors);
}

SkImageDecoder::Format SkImageDecoder::getFormat() const {
    return kUnknown_Format;
}

const char* SkImageDecoder::getFormatName() const {
    return GetFormatName(this->getFormat());
}

const char* SkImageDecoder::GetFormatName(Format format) {
    switch (format) {
        case kUnknown_Format:
            return "Unknown Format";
        case kBMP_Format:
            return "BMP";
        case kGIF_Format:
            return "GIF";
        case kICO_Format:
            return "ICO";
        case kJPEG_Format:
            return "JPEG";
        case kPNG_Format:
            return "PNG";
        case kWBMP_Format:
            return "WBMP";
        case kWEBP_Format:
            return "WEBP";
        default:
            SkASSERT(!"Invalid format type!");
    }
    return "Unknown Format";
}

SkImageDecoder::Peeker* SkImageDecoder::setPeeker(Peeker* peeker) {
    SkRefCnt_SafeAssign(fPeeker, peeker);
    return peeker;
}

SkImageDecoder::Chooser* SkImageDecoder::setChooser(Chooser* chooser) {
    SkRefCnt_SafeAssign(fChooser, chooser);
    return chooser;
}

SkBitmap::Allocator* SkImageDecoder::setAllocator(SkBitmap::Allocator* alloc) {
    SkRefCnt_SafeAssign(fAllocator, alloc);
    return alloc;
}

void SkImageDecoder::setSampleSize(int size) {
    if (size < 1) {
        size = 1;
    }

    // mutex lock for sampleSize variable protection(multi-thread region decode)
    fMutex.acquire();
    fSampleSize = size;
}

void SkImageDecoder::setPreferSize(int size) {
    if (size < 0) {
        size = 0;
    }
    fPreferSize = size;
}

void SkImageDecoder::setPostProcFlag(int flag) {

    fPostProc = flag;
}

bool SkImageDecoder::chooseFromOneChoice(SkBitmap::Config config, int width,
                                         int height) const {
    Chooser* chooser = fChooser;

    if (NULL == chooser) {    // no chooser, we just say YES to decoding :)
        return true;
    }
    chooser->begin(1);
    chooser->inspect(0, config, width, height);
    return chooser->choose() == 0;
}

bool SkImageDecoder::allocPixelRef(SkBitmap* bitmap,
                                   SkColorTable* ctable) const {
    return bitmap->allocPixels(fAllocator, ctable);
}

///////////////////////////////////////////////////////////////////////////////

void SkImageDecoder::setPrefConfigTable(const SkBitmap::Config pref[6]) {
    if (NULL == pref) {
        fUsePrefTable = false;
    } else {
        fUsePrefTable = true;
        fPrefTable.fPrefFor_8Index_NoAlpha_src = pref[0];
        fPrefTable.fPrefFor_8Index_YesAlpha_src = pref[1];
        fPrefTable.fPrefFor_8Gray_src = SkBitmap::kNo_Config;
        fPrefTable.fPrefFor_8bpc_NoAlpha_src = pref[4];
        fPrefTable.fPrefFor_8bpc_YesAlpha_src = pref[5];
    }
}

void SkImageDecoder::setPrefConfigTable(const PrefConfigTable& prefTable) {
    fUsePrefTable = true;
    fPrefTable = prefTable;
}

SkBitmap::Config SkImageDecoder::getPrefConfig(SrcDepth srcDepth,
                                               bool srcHasAlpha) const {
    SkBitmap::Config config = SkBitmap::kNo_Config;

    if (fUsePrefTable) {
        switch (srcDepth) {
            case kIndex_SrcDepth:
                config = srcHasAlpha ? fPrefTable.fPrefFor_8Index_YesAlpha_src
                                     : fPrefTable.fPrefFor_8Index_NoAlpha_src;
                break;
            case k8BitGray_SrcDepth:
                config = fPrefTable.fPrefFor_8Gray_src;
                break;
            case k32Bit_SrcDepth:
                config = srcHasAlpha ? fPrefTable.fPrefFor_8bpc_YesAlpha_src
                                     : fPrefTable.fPrefFor_8bpc_NoAlpha_src;
                break;
        }
    } else {
        config = fDefaultPref;
    }

    if (SkBitmap::kNo_Config == config) {
        config = SkImageDecoder::GetDeviceConfig();
    }
    return config;
}

#ifdef MTK_75DISPLAY_ENHANCEMENT_SUPPORT
void PostProcess(SkImageDecoder * decoder, SkBitmap* bm)
{
    unsigned long u4TimeOut = 0;
    unsigned long u4PQOpt;
    unsigned long u4Flag = 0;
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.PQ", value, "1");

    u4PQOpt = atol(value);

    if((NULL == decoder) || (NULL == bm))
    {
        XLOGD("decoder or bm is null\n");
        return ;
    }

    if(0 == u4PQOpt)
    {
        return ;
    }

    u4Flag = decoder->getPostProcFlag();
    if(0 == (0x1 & u4Flag))
    {
        return ;
    }

    if((SkImageDecoder::kPNG_Format == decoder->getFormat()) && (0 == (u4Flag >> 4)))
    {
        return ;
    }

    bm->lockPixels();

    if(NULL == bm->getPixels())
    {
        XLOGD("bm does not get any pixels\n");
        goto NULL_EXIT;
    }

    if((bm->config() == SkBitmap::kARGB_8888_Config) || 
       (bm->config() == SkBitmap::kRGB_565_Config))
    {
        mHalBltParam_t bltParam;

        memset(&bltParam, 0, sizeof(bltParam));
        
        switch(bm->config())
        {
            case SkBitmap::kARGB_8888_Config:
                bltParam.srcFormat = MHAL_FORMAT_ABGR_8888;
                bltParam.dstFormat = MHAL_FORMAT_ABGR_8888;
                break;
/*
            case SkBitmap::kYUY2_Pack_Config:
                bltParam.srcFormat = MHAL_FORMAT_YUY2;
                bltParam.dstFormat = MHAL_FORMAT_YUY2;
                break;

            case SkBitmap::kUYVY_Pack_Config:
                bltParam.srcFormat = MHAL_FORMAT_UYVY;
                bltParam.dstFormat = MHAL_FORMAT_UYVY;
                break;
*/
            case SkBitmap::kRGB_565_Config:
                bltParam.srcFormat = MHAL_FORMAT_RGB_565;
                bltParam.dstFormat = MHAL_FORMAT_RGB_565;
                break;
            default :
                goto NULL_EXIT;
            break;
        }

        bltParam.doImageProcess = ((SkImageDecoder::kJPEG_Format == decoder->getFormat() ? 1 : 2) + 0x10);
        bltParam.orientation = MHAL_BITBLT_ROT_0;
        bltParam.srcAddr = bltParam.dstAddr = (unsigned int)bm->getPixels();
        bltParam.srcX = bltParam.srcY = 0;

        if(2 == u4PQOpt)
        {
//Debug and demo mode
            bltParam.srcWStride = bm->width();
            bltParam.srcW = bltParam.dstW = (bm->width() >> 1);
        }
        else
        {
//Normal mode
            bltParam.srcW = bltParam.srcWStride = bltParam.dstW = bm->width();
        }

        bltParam.srcH = bltParam.srcHStride = bltParam.dstH = bm->height();
        bltParam.pitch = bm->rowBytesAsPixels();//bm->width();

        XLOGD("Image Processing\n");

        // start image process
        while(MHAL_NO_ERROR != mHalMdpIpc_BitBlt(&bltParam))
        {
            if(10 < u4TimeOut)
            {
                break;
            }
            else
            {
                u4TimeOut += 1;
                XLOGD("Image Process retry : %d\n" , u4TimeOut);
            }
        }

        if(10 < u4TimeOut)
        {
            XLOGD("Image Process is skipped\n");
        }
        else
        {
            XLOGD("Image Process Done\n");
        }
    }

NULL_EXIT:
    bm->unlockPixels();

    return ;

}
#endif

#ifdef MTK_89DISPLAY_ENHANCEMENT_SUPPORT
void PostProcess(SkImageDecoder * decoder, SkBitmap* bm)
{
    unsigned long u4TimeOut = 0;
    unsigned long u4PQOpt;
    unsigned long u4Flag = 0;
    unsigned long *u4RGB888pointer;
    unsigned int  *u4RGB565pointer;
    unsigned long bkPoints[8];
    
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.PQ", value, "1");

    u4PQOpt = atol(value);

    if((NULL == decoder) || (NULL == bm))
    {
        XLOGD("decoder or bm is null\n");
        return ;
    }


    if(0 == u4PQOpt)
    {
        return ;
    }

    u4Flag = decoder->getPostProcFlag();
    if(0 == (0x1 & u4Flag))
    {
        XLOGD("Flag is not 1%x" , u4Flag);
        return ;
    }

    if((SkImageDecoder::kPNG_Format == decoder->getFormat()) && (0 == (u4Flag >> 4)))
    {
        XLOGD("PNG , and flag does not force to do PQ Sharpeness %x" , u4Flag);
        return ;
    }
    
    if(bm->width()>1280)
    {
        XLOGD("Cannot support PQ Sharpeness when picture width %d > 1280 \n" , bm->width());
        return ;
    }

    bm->lockPixels();

    if(NULL == bm->getPixels())
    {
        XLOGD("bm does not get any pixels\n");
        goto NULL_EXIT;
    }

    if((bm->config() == SkBitmap::kARGB_8888_Config) || 
       (bm->config() == SkBitmap::kRGB_565_Config))
    {
        
        DpColorFormat fmt;
        
        switch(bm->config())
        {
            case SkBitmap::kARGB_8888_Config:
                fmt = eARGB8888;
                if(bm->width()%2==1)
                {     
                   u4RGB888pointer = (unsigned long *)bm->getPixels();
                   bkPoints[0] = *(u4RGB888pointer + (bm->width()*bm->height()-1));
                   bkPoints[1] = *(u4RGB888pointer + (bm->width()*bm->height()-2));
                   bkPoints[2] = *(u4RGB888pointer + (bm->width()*bm->height()-3));
                   bkPoints[3] = *(u4RGB888pointer + (bm->width()*bm->height()-4));
                   bkPoints[4] = *(u4RGB888pointer + (bm->width()*(bm->height()-1)-1));
                   bkPoints[5] = *(u4RGB888pointer + (bm->width()*(bm->height()-1)-2));
                   bkPoints[6] = *(u4RGB888pointer + (bm->width()*(bm->height()-1)-3));
                   bkPoints[7] = *(u4RGB888pointer + (bm->width()*(bm->height()-1)-4));
                }
                break;
/*
            case SkBitmap::kYUY2_Pack_Config:
                bltParam.srcFormat = MHAL_FORMAT_YUY2;
                bltParam.dstFormat = MHAL_FORMAT_YUY2;
                break;

            case SkBitmap::kUYVY_Pack_Config:
                bltParam.srcFormat = MHAL_FORMAT_UYVY;
                bltParam.dstFormat = MHAL_FORMAT_UYVY;
                break;
*/
            case SkBitmap::kRGB_565_Config:
                fmt = eRGB565;
                if(bm->width()%2==1)
                {     
                    u4RGB565pointer = (unsigned int *)bm->getPixels();
                    bkPoints[0] = (unsigned long)*(u4RGB565pointer + (bm->width()*bm->height()-1));
                    bkPoints[1] = (unsigned long)*(u4RGB565pointer + (bm->width()*bm->height()-2));
                    bkPoints[2] = (unsigned long)*(u4RGB565pointer + (bm->width()*bm->height()-3));
                    bkPoints[3] = (unsigned long)*(u4RGB565pointer + (bm->width()*bm->height()-4));
                    bkPoints[4] = (unsigned long)*(u4RGB565pointer + (bm->width()*(bm->height()-1)-1));
                    bkPoints[5] = (unsigned long)*(u4RGB565pointer + (bm->width()*(bm->height()-1)-2));
                    bkPoints[6] = (unsigned long)*(u4RGB565pointer + (bm->width()*(bm->height()-1)-3));
                    bkPoints[7] = (unsigned long)*(u4RGB565pointer + (bm->width()*(bm->height()-1)-4));
                }  
                break;
            default :
                goto NULL_EXIT;
            break;
        }

        XLOGD("Image Processing %d %d\n",bm->width(),bm->height());
        
        ////////////////////////////////////////////////////////////////////
        
        if(5 == u4PQOpt)
        {                    
            XLOGE("Output Pre-EE Result...\n");       
            FILE *fp;          
            fp = fopen("/sdcard/testOri.888", "w");
            
            if(fp!=NULL)
            fwrite(bm->getPixels(),1,bm->getSize(),fp);
            else
            XLOGE("Output Pre-EE Result fail !\n");
    
    
            if(fp!=NULL)
            {
                fclose(fp);
                XLOGE("Output Pre-EE Result Done!\n");
            }
        }
        
        DpBlitStream* stream = 0;
    
        stream = new DpBlitStream();

        stream->setSrcBuffer(bm->getPixels(), bm->getSize());
        stream->setSrcConfig(bm->width(), bm->height(), fmt);
        stream->setDstBuffer(bm->getPixels(), bm->getSize());
        stream->setDstConfig(bm->width(), bm->height(), fmt);

        stream->setRotate(0);
        stream->setTdshp(1);
        
        if(stream->invalidate())
        {
            XLOGE("TDSHP Bitblt Stream Failed!\n");
        }
        
        if((bm->width()%2==1) && fmt == eARGB8888)
        {
            XLOGD("6589 PQ EE Workaround at odd width picture\n");
            *(u4RGB888pointer + (bm->width()*bm->height()-1))      = bkPoints[0];
            *(u4RGB888pointer + (bm->width()*bm->height()-2))      = bkPoints[1];
            *(u4RGB888pointer + (bm->width()*bm->height()-3))      = bkPoints[2];
            *(u4RGB888pointer + (bm->width()*bm->height()-4))      = bkPoints[3];
            *(u4RGB888pointer + (bm->width()*(bm->height()-1)-1))  = bkPoints[4];
            *(u4RGB888pointer + (bm->width()*(bm->height()-1)-2))  = bkPoints[5];
            *(u4RGB888pointer + (bm->width()*(bm->height()-1)-3))  = bkPoints[6];
            *(u4RGB888pointer + (bm->width()*(bm->height()-1)-4))  = bkPoints[7];
        }                                                         
        else if((bm->width()%2==1) && fmt == eRGB565)
        {          
            XLOGD("6589 PQ EE Workaround at odd width picture\n");
            *(u4RGB565pointer + (bm->width()*bm->height()-1))      = bkPoints[0];
            *(u4RGB565pointer + (bm->width()*bm->height()-2))      = bkPoints[1];
            *(u4RGB565pointer + (bm->width()*bm->height()-3))      = bkPoints[2];
            *(u4RGB565pointer + (bm->width()*bm->height()-4))      = bkPoints[3];
            *(u4RGB565pointer + (bm->width()*(bm->height()-1)-1))  = bkPoints[4];
            *(u4RGB565pointer + (bm->width()*(bm->height()-1)-2))  = bkPoints[5];
            *(u4RGB565pointer + (bm->width()*(bm->height()-1)-3))  = bkPoints[6];
            *(u4RGB565pointer + (bm->width()*(bm->height()-1)-4))  = bkPoints[7];
        }   
            
            
        if(5 == u4PQOpt)
        {                    
            XLOGE("Output EE Result...\n");       
            FILE *fp;          
            fp = fopen("/sdcard/test.888", "w");
            
            if(fp!=NULL)
            fwrite(bm->getPixels(),1,bm->getSize(),fp);
            else
            XLOGE("Output EE Result fail !\n");
            
            if(fp!=NULL)
            {
                fclose(fp);
                XLOGE("Output EE Result Done!\n");
            }
        }

        delete stream;

    }

    XLOGD("Image Process Done\n");

NULL_EXIT:
    bm->unlockPixels();

    return ;
}
#endif

bool SkImageDecoder::decode(SkStream* stream, SkBitmap* bm,
                            SkBitmap::Config pref, Mode mode) {
    ATRACE_CALL();
    
    char trace_msg[128] = "NONE";

    // we reset this to false before calling onDecode
    fShouldCancelDecode = false;
    // assign this, for use by getPrefConfig(), in case fUsePrefTable is false
    fDefaultPref = pref;

    // pass a temporary bitmap, so that if we return false, we are assured of
    // leaving the caller's bitmap untouched.
    SkBitmap    tmp;
    if (!this->onDecode(stream, &tmp, mode)) {
        return false;
    }
    if(mode == kDecodeBounds_Mode)
    	sprintf(trace_msg, "%s(query):%dx%d",this->getFormatName(), tmp.width(), tmp.height());
    else
      sprintf(trace_msg, "%s(decode_scale):%dx%d",this->getFormatName(), tmp.width(), tmp.height());
      //ATRACE_BEGIN(trace_msg);
    	
#ifdef MTK_75DISPLAY_ENHANCEMENT_SUPPORT
    PostProcess(this , &tmp);
#endif

#ifdef MTK_89DISPLAY_ENHANCEMENT_SUPPORT
    PostProcess(this , &tmp);
#endif

    bm->swap(tmp);
    	
    //ATRACE_END();  
    
    
    return true;
}

bool SkImageDecoder::decodeSubset(SkBitmap* bm, const SkIRect& rect,
                                  SkBitmap::Config pref) {
    // we reset this to false before calling onDecodeSubset
    fShouldCancelDecode = false;
    // assign this, for use by getPrefConfig(), in case fUsePrefTable is false
    fDefaultPref = pref;

    if (!this->onDecodeSubset(bm, rect)) {
        return false;
    }

#ifdef MTK_75DISPLAY_ENHANCEMENT_SUPPORT
    PostProcess(this , &tmp);
#endif

#ifdef MTK_89DISPLAY_ENHANCEMENT_SUPPORT
    PostProcess(this , bm);
#endif
    
    return true;
}

bool SkImageDecoder::buildTileIndex(SkStream* stream,
                                int *width, int *height) {
    // we reset this to false before calling onBuildTileIndex
    fShouldCancelDecode = false;

    return this->onBuildTileIndex(stream, width, height);
}

bool SkImageDecoder::cropBitmap(SkBitmap *dst, SkBitmap *src, int sampleSize,
                                int dstX, int dstY, int width, int height,
                                int srcX, int srcY) {
    int w = width / sampleSize;
    int h = height / sampleSize;
    // if the destination has no pixels then we must allocate them.
    if(sampleSize > 1 && width > 0 && w == 0){
		XLOGW("Skia::cropBitmap W/H %d %d->%d %d, Sample %d, force width != 0 !!!!!!\n", width, height,w, h, sampleSize );
		w = 1;
    }
    if(sampleSize > 1 && height > 0 && h == 0){
		XLOGW("Skia::cropBitmap W/H %d %d->%d %d, Sample %d, force height != 0 !!!!!!\n", width, height,w, h, sampleSize );
		h = 1;
    }	

    if (src->getConfig() == SkBitmap::kIndex8_Config) {
        // kIndex8 does not allow drawing via an SkCanvas, as is done below.
        // Instead, use extractSubset. Note that this shares the SkPixelRef and
        // SkColorTable.
        // FIXME: Since src is discarded in practice, this holds on to more
        // pixels than is strictly necessary. Switch to a copy if memory
        // savings are more important than speed here. This also means
        // that the pixels in dst can not be reused (though there is no
        // allocation, which was already done on src).
        int x = (dstX - srcX) / sampleSize;
        int y = (dstY - srcY) / sampleSize;
        SkIRect subset = SkIRect::MakeXYWH(x, y, w, h);
        return src->extractSubset(dst, subset);
    }
    // if the destination has no pixels then we must allocate them.
    if (dst->isNull()) {
        dst->setConfig(src->getConfig(), w, h);
        dst->setIsOpaque(src->isOpaque());

        if (!this->allocPixelRef(dst, NULL)) {
            SkDEBUGF(("failed to allocate pixels needed to crop the bitmap"));
            return false;
        }
    }
    //XLOGW("Skia::cropBitmap W/H %d %d->%d %d, Sample %d !!!!!!\n", width, height,w, h, sampleSize );
    //XLOGW("Skia::cropBitmap rect X/Y : %d %d -> %d %d !!\n", srcX, srcY,destX, destY);
    //XLOGW("Skia::cropBitmap src %d %d, dst %d %d, config %d->%d !!!!!!\n", src->width(), src->height(), dest->width(), dest->height(),src->getConfig(),dest->getConfig() );
    // check to see if the destination is large enough to decode the desired
    // region. If this assert fails we will just draw as much of the source
    // into the destination that we can.
    if (dst->width() < w || dst->height() < h) {
        SkDEBUGF(("SkImageDecoder::cropBitmap does not have a large enough bitmap.\n"));
    }

    // Set the Src_Mode for the paint to prevent transparency issue in the
    // dest in the event that the dest was being re-used.
    SkPaint paint;
    paint.setXfermodeMode(SkXfermode::kSrc_Mode);

    SkCanvas canvas(*dst);
    canvas.drawSprite(*src, (srcX - dstX) / sampleSize,
                            (srcY - dstY) / sampleSize,
                            &paint);
    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool SkImageDecoder::DecodeFile(const char file[], SkBitmap* bm,
                            SkBitmap::Config pref,  Mode mode, Format* format) {
    SkASSERT(file);
    SkASSERT(bm);

    SkAutoTUnref<SkStream> stream(SkStream::NewFromFile(file));
    if (stream.get()) {
        if (SkImageDecoder::DecodeStream(stream, bm, pref, mode, format)) {
            bm->pixelRef()->setURI(file);
            return true;
        }
    }
    return false;
}

bool SkImageDecoder::DecodeMemory(const void* buffer, size_t size, SkBitmap* bm,
                          SkBitmap::Config pref, Mode mode, Format* format) {
    if (0 == size) {
        return false;
    }
    SkASSERT(buffer);

    SkMemoryStream  stream(buffer, size);
    return SkImageDecoder::DecodeStream(&stream, bm, pref, mode, format);
}

/**
 *  Special allocator used by DecodeMemoryToTarget. Uses preallocated memory
 *  provided if the bm is 8888. Otherwise, uses a heap allocator. The same
 *  allocator will be used again for a copy to 8888, when the preallocated
 *  memory will be used.
 */
class TargetAllocator : public SkBitmap::HeapAllocator {

public:
    TargetAllocator(void* target)
        : fTarget(target) {}

    virtual bool allocPixelRef(SkBitmap* bm, SkColorTable* ct) SK_OVERRIDE {
        // If the config is not 8888, allocate a pixelref using the heap.
        // fTarget will be used to store the final pixels when copied to
        // 8888.
        if (bm->config() != SkBitmap::kARGB_8888_Config) {
            return INHERITED::allocPixelRef(bm, ct);
        }
        // In kARGB_8888_Config, there is no colortable.
        SkASSERT(NULL == ct);
        bm->setPixels(fTarget);
        return true;
    }

private:
    void* fTarget;
    typedef SkBitmap::HeapAllocator INHERITED;
};

/**
 *  Helper function for DecodeMemoryToTarget. DecodeMemoryToTarget wants
 *  8888, so set the config to it. All parameters must not be null.
 *  @param decoder Decoder appropriate for this stream.
 *  @param stream Rewound stream to the encoded data.
 *  @param bitmap On success, will have its bounds set to the bounds of the
 *      encoded data, and its config set to 8888.
 *  @return True if the bounds were decoded and the bitmap is 8888 or can be
 *      copied to 8888.
 */
static bool decode_bounds_to_8888(SkImageDecoder* decoder, SkStream* stream,
                                  SkBitmap* bitmap) {
    SkASSERT(decoder != NULL);
    SkASSERT(stream != NULL);
    SkASSERT(bitmap != NULL);

    if (!decoder->decode(stream, bitmap, SkImageDecoder::kDecodeBounds_Mode)) {
        return false;
    }

    if (bitmap->config() == SkBitmap::kARGB_8888_Config) {
        return true;
    }

    if (!bitmap->canCopyTo(SkBitmap::kARGB_8888_Config)) {
        return false;
    }

    bitmap->setConfig(SkBitmap::kARGB_8888_Config, bitmap->width(), bitmap->height());
    return true;
}

/**
 *  Helper function for DecodeMemoryToTarget. Decodes the stream into bitmap, and if
 *  the bitmap is not 8888, then it is copied to 8888. Either way, the end result has
 *  its pixels stored in target. All parameters must not be null.
 *  @param decoder Decoder appropriate for this stream.
 *  @param stream Rewound stream to the encoded data.
 *  @param bitmap On success, will contain the decoded image, with its pixels stored
 *      at target.
 *  @param target Preallocated memory for storing pixels.
 *  @return bool Whether the decode (and copy, if necessary) succeeded.
 */
static bool decode_pixels_to_8888(SkImageDecoder* decoder, SkStream* stream,
                                  SkBitmap* bitmap, void* target) {
    SkASSERT(decoder != NULL);
    SkASSERT(stream != NULL);
    SkASSERT(bitmap != NULL);
    SkASSERT(target != NULL);

    TargetAllocator allocator(target);
    decoder->setAllocator(&allocator);

    bool success = decoder->decode(stream, bitmap, SkImageDecoder::kDecodePixels_Mode);
    decoder->setAllocator(NULL);

    if (!success) {
        return false;
    }

    if (bitmap->config() == SkBitmap::kARGB_8888_Config) {
        return true;
    }

    SkBitmap bm8888;
    if (!bitmap->copyTo(&bm8888, SkBitmap::kARGB_8888_Config, &allocator)) {
        return false;
    }

    bitmap->swap(bm8888);
    return true;
}

bool SkImageDecoder::DecodeMemoryToTarget(const void* buffer, size_t size,
                                          SkImage::Info* info,
                                          const SkBitmapFactory::Target* target) {
    if (NULL == info) {
        return false;
    }

    // FIXME: Just to get this working, implement in terms of existing
    // ImageDecoder calls.
    SkBitmap bm;
    SkMemoryStream stream(buffer, size);
    SkAutoTDelete<SkImageDecoder> decoder(SkImageDecoder::Factory(&stream));
    if (NULL == decoder.get()) {
        return false;
    }

    if (!decode_bounds_to_8888(decoder.get(), &stream, &bm)) {
        return false;
    }

    SkASSERT(bm.config() == SkBitmap::kARGB_8888_Config);

    // Now set info properly.
    // Since Config is SkBitmap::kARGB_8888_Config, SkBitmapToImageInfo
    // will always succeed.
    SkAssertResult(SkBitmapToImageInfo(bm, info));

    if (NULL == target) {
        return true;
    }

    if (target->fRowBytes != SkToU32(bm.rowBytes())) {
        if (target->fRowBytes < SkImageMinRowBytes(*info)) {
            SkASSERT(!"Desired row bytes is too small");
            return false;
        }
        bm.setConfig(bm.config(), bm.width(), bm.height(), target->fRowBytes);
    }

    // SkMemoryStream.rewind() will always return true.
    SkAssertResult(stream.rewind());
    return decode_pixels_to_8888(decoder.get(), &stream, &bm, target->fAddr);
}


bool SkImageDecoder::DecodeStream(SkStream* stream, SkBitmap* bm,
                          SkBitmap::Config pref, Mode mode, Format* format) {
    SkASSERT(stream);
    SkASSERT(bm);

    bool success = false;
    SkImageDecoder* codec = SkImageDecoder::Factory(stream);

    if (NULL != codec) {
        success = codec->decode(stream, bm, pref, mode);
        if (success && format) {
            *format = codec->getFormat();
            if (kUnknown_Format == *format) {
                if (stream->rewind()) {
                    *format = GetStreamFormat(stream);
                }
            }
        }
        delete codec;
    }
    return success;
}
