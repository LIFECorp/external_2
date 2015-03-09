/*
 * Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

void S32_Opaque_D32_filter_DX_shaderproc_neon(const unsigned int* image0, const unsigned int* image1,
                                        SkFixed fx, unsigned int maxX, unsigned int subY,
                                        unsigned int* colors, SkFixed dx, int count) {

    asm volatile(
            "rsb        r4, %[subY], #16            \n\t"
            "vmov.u16   d12, #16                    \n\t"   // set up constant in d16,int 16bit
            "vdup.8     d14, %[subY]                \n\t"   // duplicate y into d17
            "vdup.8     d15, r4                     \n\t"   // duplicate y into d17

            "cmp        %[count], #4                \n\t"
            "blt        loop4_end                   \n\t"

            "loop4:                                 \n\t"
            "asr        r4, %[fx], #12              \n\t"
            "asr        r5, %[fx], #16              \n\t"
            "add        %[fx], %[dx]                \n\t"
            "and        r4, #0xf                    \n\t"
            "add        r6, %[image1], r5, lsl #2   \n\t"
            "add        r5, %[image0], r5, lsl #2   \n\t"
            "vdup.16    d8, r4                      \n\t"   // subX0
            "vld1.32    d1, [r6]                    \n\t"
            "vld1.32    d0, [r5]                    \n\t"

            "asr        r4, %[fx], #12              \n\t"
            "asr        r5, %[fx], #16              \n\t"
            "add        %[fx], %[dx]                \n\t"
            "and        r4, #0xf                    \n\t"
            "add        r6, %[image1], r5, lsl #2   \n\t"
            "add        r5, %[image0], r5, lsl #2   \n\t"
            "vdup.16    d9, r4                      \n\t"   // subX1
            "vld1.32    d3, [r6]                    \n\t"
            "vld1.32    d2, [r5]                    \n\t"

            "asr        r4, %[fx], #12              \n\t"
            "asr        r5, %[fx], #16              \n\t"
            "add        %[fx], %[dx]                \n\t"
            "and        r4, #0xf                    \n\t"
            "add        r6, %[image1], r5, lsl #2   \n\t"
            "add        r5, %[image0], r5, lsl #2   \n\t"
            "vdup.16    d10, r4                     \n\t"   // subX2
            "vld1.32    d5, [r6]                    \n\t"
            "vld1.32    d4, [r5]                    \n\t"

            "asr        r4, %[fx], #12              \n\t"
            "asr        r5, %[fx], #16              \n\t"
            "and        r4, #0xf                    \n\t"
            "add        r6, %[image1], r5, lsl #2   \n\t"
            "add        r5, %[image0], r5, lsl #2   \n\t"
            "vdup.16    d11, r4                     \n\t"   // subX3
            "vld1.32    d7, [r6]                    \n\t"
            "vld1.32    d6, [r5]                    \n\t"

            // filter 4 pixle
            "vmull.u8   q8 , d0 , d15               \n\t"   // pix 0 : [a01|a00] * (16-y)
            "vmull.u8   q10, d2 , d15               \n\t"   // pix 1 : [a01|a00] * (16-y)
            "vmull.u8   q12, d4 , d15               \n\t"   // pix 2 : [a01|a00] * (16-y)
            "vmull.u8   q14, d6 , d15               \n\t"   // pix 3 : [a01|a00] * (16-y)

            "vmull.u8   q9 , d1 , d14               \n\t"   // pix 0 : [a11|a10] * y
            "vmull.u8   q11, d3 , d14               \n\t"   // pix 1 : [a11|a10] * y
            "vmull.u8   q13, d5 , d14               \n\t"   // pix 2 : [a11|a10] * y
            "vmull.u8   q15, d7 , d14               \n\t"   // pix 3 : [a11|a10] * y

            "vmul.i16   d0 , d17, d8                \n\t"   // pix 0 : a01 * x
            "vmul.i16   d1 , d21, d9                \n\t"   // pix 1 : a01 * x
            "vmul.i16   d2 , d25, d10               \n\t"   // pix 2 : a01 * x
            "vmul.i16   d3 , d29, d11               \n\t"   // pix 3 : a01 * x

            "vmla.i16   d0 , d19, d8                \n\t"   // pix 0 : + a11 * x
            "vmla.i16   d1 , d23, d9                \n\t"   // pix 1 : + a11 * x
            "vmla.i16   d2 , d27, d10               \n\t"   // pix 2 : + a11 * x
            "vmla.i16   d3 , d31, d11               \n\t"   // pix 3 : + a11 * x

            "vsub.u16   d8 , d12, d8                \n\t"   // pix 0 : 16-x
            "vsub.u16   d9 , d12, d9                \n\t"   // pix 1 : 16-x
            "vsub.u16   d10, d12, d10               \n\t"   // pix 2 : 16-x
            "vsub.u16   d11, d12, d11               \n\t"   // pix 3 : 16-x

            "vmla.i16   d0 , d16, d8                \n\t"   // pix 0 : + a00 * (16-x)
            "vmla.i16   d1 , d20, d9                \n\t"   // pix 1 : + a00 * (16-x)
            "vmla.i16   d2 , d24, d10               \n\t"   // pix 2 : + a00 * (16-x)
            "vmla.i16   d3 , d28, d11               \n\t"   // pix 3 : + a00 * (16-x)

            "vmla.i16   d0 , d18, d8                \n\t"   // pix 0 : + a10 * (16-x)
            "vmla.i16   d1 , d22, d9                \n\t"   // pix 1 : + a10 * (16-x)
            "vmla.i16   d2 , d26, d10               \n\t"   // pix 2 : + a10 * (16-x)
            "vmla.i16   d3 , d30, d11               \n\t"   // pix 3 : + a10 * (16-x)

            "sub        %[count], %[count], #4      \n\t"
            "add        %[fx], %[dx]                \n\t"

            "vshrn.i16  d0, q0, #8                  \n\t"
            "vshrn.i16  d1, q1, #8                  \n\t"

            "cmp        %[count], #4    \n\t"

            "vst1.u32   {d0, d1}, [%[colors]]!      \n\t"

            "bge        loop4                       \n\t"

            "loop4_end:                             \n\t"

            "cmp        %[count], #0                \n\t"
            "ble        tailloop_end                \n\t"

            "tailloop:                              \n\t"
            "asr        r4, %[fx], #12              \n\t"
            "asr        r5, %[fx], #16              \n\t"
            "and        r4, #0xf                    \n\t"
            "add        r6, %[image1], r5, lsl #2   \n\t"
            "add        r5, %[image0], r5, lsl #2   \n\t"
            "vdup.16    d8, r4                      \n\t"   // subX0
            "vld1.32    d1, [r6]                    \n\t"
            "vld1.32    d0, [r5]                    \n\t"

            // pix 0
            "vmull.u8   q8 , d0 , d15               \n\t"   // pix 0 : [a01|a00] * (16-y)
            "vmull.u8   q9 , d1 , d14               \n\t"   // pix 0 : [a11|a10] * y
            "vmul.i16   d0 , d17, d8                \n\t"   // pix 0 : a01 * x
            "vmla.i16   d0 , d19, d8                \n\t"   // pix 0 : + a11 * x
            "vsub.u16   d8 , d12, d8                \n\t"   // pix 0 : 16-x
            "vmla.i16   d0 , d16, d8                \n\t"   // pix 0 : + a00 * (16-x)
            "vmla.i16   d0 , d18, d8                \n\t"   // pix 0 : + a10 * (16-x)

            "subs       %[count], %[count], #1      \n\t"
            "add        %[fx], %[dx]                \n\t"

            "vshrn.i16  d0, q0, #8                  \n\t"
            "vst1.u32   d0[0], [%[colors]]!         \n\t"

            "bgt        tailloop                    \n\t"

            "tailloop_end:                          \n\t"

            : [colors] "+r" (colors)
            : [image0] "r" (image0), [image1] "r" (image1), [fx] "r" (fx), [subY] "r" (subY), [dx] "r" (dx), [count] "r" (count)
            : "cc", "memory", "r4", "r5", "r6", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15"
            );


}
