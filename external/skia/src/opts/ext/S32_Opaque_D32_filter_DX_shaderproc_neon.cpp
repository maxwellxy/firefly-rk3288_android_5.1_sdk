/*
 * Copyright (c) 2010,2013, The Linux Foundation. All rights reserved.
 * *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 * * Neither the name of The Linux Foundation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * */

#include "SkFixed.h"
#include "SkUtilsArm.h"

void S32_Opaque_D32_filter_DX_shaderproc_neon(const unsigned int* image0, const unsigned int* image1,
                                        SkFixed fx, unsigned int maxX, unsigned int subY,
                                         unsigned int* colors,
                                         SkFixed dx, int count) __attribute__ ((optimize ("0"))) ;
void S32_Opaque_D32_filter_DX_shaderproc_neon(const unsigned int* image0, const unsigned int* image1,
                                        SkFixed fx, unsigned int maxX, unsigned int subY,
                                         unsigned int* colors,
                                         SkFixed dx, int count) {

    asm volatile(
            "mov r3, %[count]    \n\t"    //r3 = count

            "mov r5, %[fx] \n\t"        //r5 = x = fx
            "cmp r3, #0 \n\t"
            "beq 12f \n\t"              // branch forward to endloop if r3 == 0

            "vdup.8         d17, %[subY]                \n\t"   // duplicate y into d17
            "vmov.u8        d16, #16                \n\t"   // set up constant in d16
            "vsub.u8        d18, d16, d17             \n\t"   // d18 = 16-y

            "vmov.u16       d16, #16                \n\t"   // set up constant in d16,int 16bit

#define UNROLL8
#define UNROLL2
#ifdef UNROLL8
            "cmp r3, #8 \n\t"
            "blt 20f \n\t"              // branch forward to initloop2 if r3 < 8
            ///////////////loop2 in x
            "81:    \n\t"               // beginloop8:

                /////////////////pixel 1////////////////////////////////////
                //x0 = SkClampMax((fx) >> 16, max)
                "asr r4, r5, #16 \n\t"

                "lsl r4, r4, #2 \n\t"
                "add r6, r4, %[image0] \n\t"
                "vldr.32 d4, [r6] \n\t"
                "add r6, r4, %[image1] \n\t"
                "vldr.32 d5, [r6] \n\t"

                //(((fx) >> 12) & 0xF)
                "lsr r4, r5, #12 \n\t"
                "and r4, r4, #15 \n\t"
                "vdup.16        d19, r4                \n\t"   // duplicate x into d19


                ////////////bilinear interp

                "vmull.u8       q3, d4, d18              \n\t"   // q3 = [a01|a00] * (16-y)
                "vmull.u8       q0, d5, d17              \n\t"   // q0 = [a11|a10] * y

                "vsub.u16       d20, d16, d19             \n\t"   // d20 = 16-x

                "vmul.i16       d22, d7, d19              \n\t"   // d4  = a01 * x
                "vmla.i16       d22, d1, d19              \n\t"   // d4 += a11 * x
                "vmla.i16       d22, d6, d20              \n\t"   // d4 += a00 * (16-x)
                "vmla.i16       d22, d0, d20              \n\t"   // d4 += a10 * (16-x)

                //////////////// end bilinear interp

                "add r5, r5, %[dx] \n\t"    //r5 = x += dx

                /////////////////pixel 2////////////////////////////////////
                //x0 = SkClampMax((fx) >> 16, max)
                "asr r4, r5, #16 \n\t"

                "lsl r4, r4, #2 \n\t"
                "add r6, r4, %[image0] \n\t"
                "vldr.32 d4, [r6] \n\t"
                "add r6, r4, %[image1] \n\t"
                "vldr.32 d5, [r6] \n\t"

                //(((fx) >> 12) & 0xF)
                "lsr r4, r5, #12 \n\t"
                "and r4, r4, #15 \n\t"
                "vdup.16        d19, r4                \n\t"   // duplicate x into d19


                ////////////bilinear interp

                "vmull.u8       q3, d4, d18              \n\t"   // q3 = [a01|a00] * (16-y)
                "vmull.u8       q0, d5, d17              \n\t"   // q0 = [a11|a10] * y

                "vsub.u16       d20, d16, d19             \n\t"   // d20 = 16-x

                "vmul.i16       d24, d7, d19              \n\t"   // d4  = a01 * x
                "vmla.i16       d24, d1, d19              \n\t"   // d4 += a11 * x
                "vmla.i16       d24, d6, d20              \n\t"   // d4 += a00 * (16-x)
                "vmla.i16       d24, d0, d20              \n\t"   // d4 += a10 * (16-x)

                //////////////// end bilinear interp

                "add r5, r5, %[dx] \n\t"    //r5 = x += dx

                /////////////////pixel 3////////////////////////////////
                //x0 = SkClampMax((fx) >> 16, max)
                "asr r4, r5, #16 \n\t"

                "lsl r4, r4, #2 \n\t"
                "add r6, r4, %[image0] \n\t"
                "vldr.32 d4, [r6] \n\t"
                "add r6, r4, %[image1] \n\t"
                "vldr.32 d5, [r6] \n\t"

                //(((fx) >> 12) & 0xF)
                "lsr r4, r5, #12 \n\t"
                "and r4, r4, #15 \n\t"
                "vdup.16        d19, r4                \n\t"   // duplicate x into d19


                ////////////bilinear interp

                "vmull.u8       q3, d4, d18              \n\t"   // q3 = [a01|a00] * (16-y)
                "vmull.u8       q0, d5, d17              \n\t"   // q0 = [a11|a10] * y

                "vsub.u16       d20, d16, d19             \n\t"   // d20 = 16-x

                "vmul.i16       d26, d7, d19              \n\t"   // d4  = a01 * x
                "vmla.i16       d26, d1, d19              \n\t"   // d4 += a11 * x
                "vmla.i16       d26, d6, d20              \n\t"   // d4 += a00 * (16-x)
                "vmla.i16       d26, d0, d20              \n\t"   // d4 += a10 * (16-x)

                //////////////// end bilinear interp

                "add r5, r5, %[dx] \n\t"    //r5 = x += dx

                /////////////////pixel 4////////////////////////////////
                //x0 = SkClampMax((fx) >> 16, max)
                "asr r4, r5, #16 \n\t"

                "lsl r4, r4, #2 \n\t"
                "add r6, r4, %[image0] \n\t"
                "vldr.32 d4, [r6] \n\t"
                "add r6, r4, %[image1] \n\t"
                "vldr.32 d5, [r6] \n\t"

                //(((fx) >> 12) & 0xF)
                "lsr r4, r5, #12 \n\t"
                "and r4, r4, #15 \n\t"
                "vdup.16        d19, r4                \n\t"   // duplicate x into d19


                ////////////bilinear interp

                "vmull.u8       q3, d4, d18              \n\t"   // q3 = [a01|a00] * (16-y)
                "vmull.u8       q0, d5, d17              \n\t"   // q0 = [a11|a10] * y

                "vsub.u16       d20, d16, d19             \n\t"   // d20 = 16-x

                "vmul.i16       d28, d7, d19              \n\t"   // d4  = a01 * x
                "vmla.i16       d28, d1, d19              \n\t"   // d4 += a11 * x
                "vmla.i16       d28, d6, d20              \n\t"   // d4 += a00 * (16-x)
                "vmla.i16       d28, d0, d20              \n\t"   // d4 += a10 * (16-x)

                //////////////// end bilinear interp

                "add r5, r5, %[dx] \n\t"    //r5 = x += dx

                /////////////////pixel 5////////////////////////////////////
                //x0 = SkClampMax((fx) >> 16, max)
                "asr r4, r5, #16 \n\t"

                "lsl r4, r4, #2 \n\t"
                "add r6, r4, %[image0] \n\t"
                "vldr.32 d4, [r6] \n\t"
                "add r6, r4, %[image1] \n\t"
                "vldr.32 d5, [r6] \n\t"

                //(((fx) >> 12) & 0xF)
                "lsr r4, r5, #12 \n\t"
                "and r4, r4, #15 \n\t"
                "vdup.16        d19, r4                \n\t"   // duplicate x into d19


                ////////////bilinear interp

                "vmull.u8       q3, d4, d18              \n\t"   // q3 = [a01|a00] * (16-y)
                "vmull.u8       q0, d5, d17              \n\t"   // q0 = [a11|a10] * y

                "vsub.u16       d20, d16, d19             \n\t"   // d20 = 16-x

                "vmul.i16       d23, d7, d19              \n\t"   // d4  = a01 * x
                "vmla.i16       d23, d1, d19              \n\t"   // d4 += a11 * x
                "vmla.i16       d23, d6, d20              \n\t"   // d4 += a00 * (16-x)
                "vmla.i16       d23, d0, d20              \n\t"   // d4 += a10 * (16-x)

                //////////////// end bilinear interp

                "add r5, r5, %[dx] \n\t"    //r5 = x += dx

                /////////////////pixel 6////////////////////////////////////
                //x0 = SkClampMax((fx) >> 16, max)
                "asr r4, r5, #16 \n\t"

                "lsl r4, r4, #2 \n\t"
                "add r6, r4, %[image0] \n\t"
                "vldr.32 d4, [r6] \n\t"
                "add r6, r4, %[image1] \n\t"
                "vldr.32 d5, [r6] \n\t"

                //(((fx) >> 12) & 0xF)
                "lsr r4, r5, #12 \n\t"
                "and r4, r4, #15 \n\t"
                "vdup.16        d19, r4                \n\t"   // duplicate x into d19


                ////////////bilinear interp

                "vmull.u8       q3, d4, d18              \n\t"   // q3 = [a01|a00] * (16-y)
                "vmull.u8       q0, d5, d17              \n\t"   // q0 = [a11|a10] * y

                "vsub.u16       d20, d16, d19             \n\t"   // d20 = 16-x

                "vmul.i16       d25, d7, d19              \n\t"   // d4  = a01 * x
                "vmla.i16       d25, d1, d19              \n\t"   // d4 += a11 * x
                "vmla.i16       d25, d6, d20              \n\t"   // d4 += a00 * (16-x)
                "vmla.i16       d25, d0, d20              \n\t"   // d4 += a10 * (16-x)

                //////////////// end bilinear interp

                "add r5, r5, %[dx] \n\t"    //r5 = x += dx

                /////////////////pixel 7////////////////////////////////
                //x0 = SkClampMax((fx) >> 16, max)
                "asr r4, r5, #16 \n\t"

                "lsl r4, r4, #2 \n\t"
                "add r6, r4, %[image0] \n\t"
                "vldr.32 d4, [r6] \n\t"
                "add r6, r4, %[image1] \n\t"
                "vldr.32 d5, [r6] \n\t"

                //(((fx) >> 12) & 0xF)
                "lsr r4, r5, #12 \n\t"
                "and r4, r4, #15 \n\t"
                "vdup.16        d19, r4                \n\t"   // duplicate x into d19


                ////////////bilinear interp

                "vmull.u8       q3, d4, d18              \n\t"   // q3 = [a01|a00] * (16-y)
                "vmull.u8       q0, d5, d17              \n\t"   // q0 = [a11|a10] * y

                "vsub.u16       d20, d16, d19             \n\t"   // d20 = 16-x

                "vmul.i16       d27, d7, d19              \n\t"   // d4  = a01 * x
                "vmla.i16       d27, d1, d19              \n\t"   // d4 += a11 * x
                "vmla.i16       d27, d6, d20              \n\t"   // d4 += a00 * (16-x)
                "vmla.i16       d27, d0, d20              \n\t"   // d4 += a10 * (16-x)

                //////////////// end bilinear interp

                "add r5, r5, %[dx] \n\t"    //r5 = x += dx

                /////////////////pixel 8////////////////////////////////
                //x0 = SkClampMax((fx) >> 16, max)
                "asr r4, r5, #16 \n\t"

                "lsl r4, r4, #2 \n\t"
                "add r6, r4, %[image0] \n\t"
                "vldr.32 d4, [r6] \n\t"
                "add r6, r4, %[image1] \n\t"
                "vldr.32 d5, [r6] \n\t"

                //(((fx) >> 12) & 0xF)
                "lsr r4, r5, #12 \n\t"
                "and r4, r4, #15 \n\t"
                "vdup.16        d19, r4                \n\t"   // duplicate x into d19


                ////////////bilinear interp

                "vmull.u8       q3, d4, d18              \n\t"   // q3 = [a01|a00] * (16-y)
                "vmull.u8       q0, d5, d17              \n\t"   // q0 = [a11|a10] * y

                "vsub.u16       d20, d16, d19             \n\t"   // d20 = 16-x

                "vmul.i16       d29, d7, d19              \n\t"   // d4  = a01 * x
                "vmla.i16       d29, d1, d19              \n\t"   // d4 += a11 * x
                "vmla.i16       d29, d6, d20              \n\t"   // d4 += a00 * (16-x)
                "vmla.i16       d29, d0, d20              \n\t"   // d4 += a10 * (16-x)

                //////////////// Store results///////////////////

                "vshrn.i16      d0, q11, #8              \n\t"   // shift down result by 8
                "vshrn.i16      d1, q12, #8              \n\t"   // shift down result by 8
                "vshrn.i16      d2, q13, #8              \n\t"   // shift down result by 8
                "vshrn.i16      d3, q14, #8              \n\t"   // shift down result by 8

                "vst4.u32        {d0, d1, d2, d3}, [%[colors]]!       \n\t"   // store result

                //////////////// end bilinear interp

                "sub r3, r3, #8    \n\t"    //num -=8
                "add r5, r5, %[dx] \n\t"    //r5 = x += dx
                "cmp r3, #7 \n\t"

                "bgt        81b  \n\t"      // branch backward to beginloop8 if r3 > 7

            "82:    \n\t"                   // endloop8:
            ////////////////end loop in x
#endif    //UNROLL8



#ifdef UNROLL2
            "20: \n\t"                      // initloop2:
            "cmp r3, #2 \n\t"
            "blt 10f \n\t"                  // branch forward to initloop if r3 < 2
            ///////////////loop2 in x
            "21:    \n\t"                   // beginloop2:


                //x0 = SkClampMax((fx) >> 16, max)
                "asr r4, r5, #16 \n\t"

                "lsl r4, r4, #2 \n\t"
                "add r6, r4, %[image0] \n\t"
                "vldr.32 d4, [r6] \n\t"
                "add r6, r4, %[image1] \n\t"
                "vldr.32 d5, [r6] \n\t"

                //(((fx) >> 12) & 0xF)
                "lsr r4, r5, #12 \n\t"
                "and r4, r4, #15 \n\t"
                "vdup.16        d19, r4                \n\t"   // duplicate x into d19


                ////////////bilinear interp

                "vmull.u8       q3, d4, d18              \n\t"   // q3 = [a01|a00] * (16-y)
                "vmull.u8       q0, d5, d17              \n\t"   // q0 = [a11|a10] * y

                "vsub.u16       d20, d16, d19             \n\t"   // d20 = 16-x

                "vmul.i16       d22, d7, d19              \n\t"   // d4  = a01 * x
                "vmla.i16       d22, d1, d19              \n\t"   // d4 += a11 * x
                "vmla.i16       d22, d6, d20              \n\t"   // d4 += a00 * (16-x)
                "vmla.i16       d22, d0, d20              \n\t"   // d4 += a10 * (16-x)

                //////////////// end bilinear interp

                "add r5, r5, %[dx] \n\t"    //r5 = x += dx

                /////////////////second half////////////////////////////////
                //x0 = SkClampMax((fx) >> 16, max)
                "asr r4, r5, #16 \n\t"

                "lsl r4, r4, #2 \n\t"
                "add r6, r4, %[image0] \n\t"
                "vldr.32 d4, [r6] \n\t"
                "add r6, r4, %[image1] \n\t"
                "vldr.32 d5, [r6] \n\t"

                //(((fx) >> 12) & 0xF)
                "lsr r4, r5, #12 \n\t"
                "and r4, r4, #15 \n\t"
                "vdup.16        d19, r4                \n\t"   // duplicate x into d19


                ////////////bilinear interp

                "vmull.u8       q3, d4, d18              \n\t"   // q3 = [a01|a00] * (16-y)
                "vmull.u8       q0, d5, d17              \n\t"   // q0 = [a11|a10] * y

                "vsub.u16       d20, d16, d19             \n\t"   // d20 = 16-x

                "vmul.i16       d23, d7, d19              \n\t"   // d4  = a01 * x
                "vmla.i16       d23, d1, d19              \n\t"   // d4 += a11 * x
                "vmla.i16       d23, d6, d20              \n\t"   // d4 += a00 * (16-x)
                "vmla.i16       d23, d0, d20              \n\t"   // d4 += a10 * (16-x)
                "vshrn.i16      d0, q11, #8              \n\t"   // shift down result by 8

                "vst1.u32        {d0}, [%[colors]]!       \n\t"   // store result

                //////////////// end bilinear interp

                "sub r3, r3, #2    \n\t"    //num -=2
                "add r5, r5, %[dx] \n\t"    //r5 = x += dx
                "cmp r3, #1 \n\t"

                "bgt        21b  \n\t"                            // branch backward to beginloop2 if r3 > 1

            "22:    \n\t"                                         // endloop2:
            ////////////////end loop in x
#endif    //UNROLL2

#if defined (UNROLL2) || defined (UNROLL8)
            "10: \n\t"                                            // initloop:
            "cmp r3, #0 \n\t"
            "ble 12f \n\t"                                        // branch forward to endloop if r3 <= 0
#endif    //defined (UNROLL2) || defined (UNROLL8)

            ///////////////loop in x
            "11:    \n\t"                                         // beginloop:


                //x0 = SkClampMax((fx) >> 16, max)
                "asr r4, r5, #16 \n\t"

                "lsl r4, r4, #2 \n\t"
                "add r6, r4, %[image0] \n\t"
                "vldr.32 d4, [r6] \n\t"
                "add r6, r4, %[image1] \n\t"
                "vldr.32 d5, [r6] \n\t"

                //(((fx) >> 12) & 0xF)
                "lsr r4, r5, #12 \n\t"
                "and r4, r4, #15 \n\t"
                "vdup.16        d19, r4                \n\t"   // duplicate x into d19


                ////////////bilinear interp

                "vmull.u8       q3, d4, d18              \n\t"   // q3 = [a01|a00] * (16-y)
                "vmull.u8       q0, d5, d17              \n\t"   // q0 = [a11|a10] * y

                "vsub.u16       d20, d16, d19             \n\t"   // d20 = 16-x

                "vmul.i16       d4, d7, d19              \n\t"   // d4  = a01 * x
                "vmla.i16       d4, d1, d19              \n\t"   // d4 += a11 * x
                "vmla.i16       d4, d6, d20              \n\t"   // d4 += a00 * (16-x)
                "vmla.i16       d4, d0, d20              \n\t"   // d4 += a10 * (16-x)
                "vshrn.i16      d0, q2, #8              \n\t"   // shift down result by 8

                "vst1.u32        {d0[0]}, [%[colors]]!       \n\t"   // store result

                //////////////// end bilinear interp

                "sub r3, r3, #1    \n\t"    //num -=1
                "add r5, r5, %[dx] \n\t"    //r5 = x += dx
                "cmp r3, #0 \n\t"
                "bgt        11b  \n\t"      // branch backward to beginloop if r3 > 0

            "12:    \n\t"                   // endloop:
            ////////////////end loop in x
            : [colors] "+r" (colors)
            : [image0] "r" (image0), [image1] "r" (image1), [fx] "r" (fx), [maxX] "r" (maxX), [subY] "r" (subY),
             [dx] "r" (dx), [count] "r" (count)
            : "cc", "memory", "r3", "r4", "r5", "r6", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27", "d28", "d29"
            );


}
