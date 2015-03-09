# Copyright 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
        alpha.c \
        analysis.c \
        backward_references.c \
        config.c \
        cost.c \
        filter.c \
        frame.c\
        histogram.c \
        iterator.c \
        layer.c \
        picture.c \
        quant.c \
        syntax.c \
        tree.c \
        token.c \
        vp8l.c \
        webpenc.c \
        ../enc_dsp/cpu.c \
        ../enc_dsp/cpu-features.c \
        ../enc_dsp/enc.c \
        ../enc_dsp/enc_neon.c \
        ../enc_dsp/enc_sse2.c \
        ../enc_dsp/lossless.c \
        ../enc_dsp/yuv.c \
        ../enc_utils/bit_writer.c \
        ../enc_utils/color_cache.c \
        ../enc_utils/filters.c \
        ../enc_utils/huffman.c \
        ../enc_utils/huffman_encode.c \
        ../enc_utils/quant_levels.c \
        ../enc_utils/rescaler.c \
        ../enc_utils/thread.c \
        ../enc_utils/utils.c

LOCAL_CFLAGS := -DANDROID -DWEBP_SWAP_16BIT_CSP

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/../../include

LOCAL_MODULE:= libwebp-encode

include $(BUILD_STATIC_LIBRARY)
