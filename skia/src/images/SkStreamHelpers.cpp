/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkStream.h"
#include "SkStreamHelpers.h"
#include "SkTypes.h"
#include "SkImageDecoder.h"

size_t CopyStreamToStorage(SkAutoMalloc* storage, SkStream* stream) {
    SkASSERT(storage != NULL);
    SkASSERT(stream != NULL);
    size_t max_size = MTK_MAX_SRC_BMP_FILE_SIZE;

    if (stream->hasLength()) {
        const size_t length = stream->getLength();
        if(length > max_size){
            SkDebugf("SKIA [CopyStreamToStorage] stream length out of spec  \n");
            return 0;
        }
        void* dst = storage->reset(length);
        if (stream->read(dst, length) != length) {
            return 0;
        }
        return length;
    }

    SkDynamicMemoryWStream tempStream;
    // Arbitrary buffer size.
    const size_t bufferSize = 256 * 1024; // 256KB
    char buffer[bufferSize];
    SkDEBUGCODE(size_t debugLength = 0;)
    do {
        size_t bytesRead = stream->read(buffer, bufferSize);
        tempStream.write(buffer, bytesRead);
        SkDEBUGCODE(debugLength += bytesRead);
        SkASSERT(tempStream.bytesWritten() == debugLength);
        if(tempStream.bytesWritten() > max_size){
            SkDebugf("SKIA [CopyStreamToStorage] tempStream length out of spec  \n");
            tempStream.reset();
            storage->free();
            return 0;
        }
    } while (!stream->isAtEnd());
    const size_t length = tempStream.bytesWritten();
    void* dst = storage->reset(length);
    tempStream.copyTo(dst);
    return length;
}
