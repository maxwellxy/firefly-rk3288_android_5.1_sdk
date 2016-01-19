//created by Victoria Zhislina, the Senior Application Engineer, Intel Corporation,  victoria.zhislina@intel.com

//*** Copyright (C) 2012-2014 Intel Corporation.  All rights reserved.

//IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.

//By downloading, copying, installing or using the software you agree to this license.
//If you do not agree to this license, do not download, install, copy or use the software.

//                              License Agreement

//Permission to use, copy, modify, and/or distribute this software for any
//purpose with or without fee is hereby granted, provided that the above
//copyright notice and this permission notice appear in all copies.

//THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
//REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
//INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
//LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
//OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//PERFORMANCE OF THIS SOFTWARE.

//*****************************************************************************************
// This file is intended to simplify ARM->IA32 porting
// It makes the correspondence between ARM NEON intrinsics (as defined in "arm_neon.h")
// and x86 SSE(up to SSE4.2) intrinsic functions as defined in headers files below
// MMX instruction set is not used due to performance overhead and the necessity to use the
// EMMS instruction (_mm_empty())for mmx-x87 floating point switching
//*****************************************************************************************

//!!!!!!!  To use this file in your project that uses ARM NEON intinsics just keep arm_neon.h included and complile it as usual.
//!!!!!!!  Please pay attention at #define USE_SSSE3 and USE_SSE4 below - you need to define them for newest Intel platforms for
//!!!!!!!  greater performance. It can be done by -mssse3 or -msse4.2 (which also implies -mssse3) compiler switch.

#ifndef NEON2SSE_H
#define NEON2SSE_H

#ifndef USE_SSE4
    #if defined(__SSE4_2__)
        #define USE_SSE4
        #define USE_SSSE3
    #endif
#endif

#ifndef USE_SSSE3
    #if defined(__SSSE3__)
        #define USE_SSSE3
    #endif
#endif

#include <xmmintrin.h>     //SSE
#include <emmintrin.h>     //SSE2
#include <pmmintrin.h>     //SSE3

#ifdef USE_SSSE3
    #include <tmmintrin.h>     //SSSE3
#else
# warning "Some functions require SSSE3 or higher."
#endif

#ifdef USE_SSE4
    #include <smmintrin.h>     //SSE4.1
    #include <nmmintrin.h>     //SSE4.2
#endif

/*********************************************************************************************************************/
//    data types conversion
/*********************************************************************************************************************/

typedef __m128 float32x4_t;

typedef __m128 float16x8_t;         //not supported by IA, for compartibility

typedef __m128i int8x16_t;
typedef __m128i int16x8_t;
typedef __m128i int32x4_t;
typedef __m128i int64x2_t;
typedef __m128i uint8x16_t;
typedef __m128i uint16x8_t;
typedef __m128i uint32x4_t;
typedef __m128i uint64x2_t;
typedef __m128i poly8x16_t;
typedef __m128i poly16x8_t;

#if defined(_MSC_VER) && (_MSC_VER < 1300)
    typedef signed char int8_t;
    typedef unsigned char uint8_t;
    typedef signed short int16_t;
    typedef unsigned short uint16_t;
    typedef signed int int32_t;
    typedef unsigned int uint32_t;
    typedef signed long long int64_t;
    typedef unsigned long long uint64_t;
#elif defined(_MSC_VER)
    typedef signed __int8 int8_t;
    typedef unsigned __int8 uint8_t;
    typedef signed __int16 int16_t;
    typedef unsigned __int16 uint16_t;
    typedef signed __int32 int32_t;
    typedef unsigned __int32 uint32_t;

typedef signed long long int64_t;
typedef unsigned long long uint64_t;
#else
    #include <stdint.h>
    #include <limits.h>
#endif
#if defined(_MSC_VER)
#define SINT_MIN     (-2147483647 - 1)    /* min signed int value */
#define SINT_MAX       2147483647         /* max signed int value */
#else
#define SINT_MIN     INT_MIN              /* min signed int value */
#define SINT_MAX     INT_MAX              /* max signed int value */
#endif

typedef   float float32_t;
#if !defined(__clang__)
typedef   float __fp16;
#endif

typedef  uint8_t poly8_t;
typedef  uint16_t poly16_t;

//MSVC compilers (tested up to 2012 VS version) doesn't allow using structures or arrays of __m128x type  as functions arguments resulting in
//error C2719: 'src': formal parameter with __declspec(align('16')) won't be aligned.  To avoid it we need the special trick for functions that use these types

//Unfortunately we are unable to merge two 64-bits in on 128 bit register because user should be able to access val[n] members explicitly!!!
struct int8x16x2_t {
    int8x16_t val[2];
};
struct int16x8x2_t {
    int16x8_t val[2];
};
struct int32x4x2_t {
    int32x4_t val[2];
};
struct int64x2x2_t {
    int64x2_t val[2];
};

typedef struct int8x16x2_t int8x16x2_t;         //for C compilers to make them happy
typedef struct int16x8x2_t int16x8x2_t;         //for C compilers to make them happy
typedef struct int32x4x2_t int32x4x2_t;         //for C compilers to make them happy
typedef struct int64x2x2_t int64x2x2_t;         //for C compilers to make them happy
//to avoid pointers conversion
typedef  int8x16x2_t int8x8x2_t;
typedef  int16x8x2_t int16x4x2_t;
typedef  int32x4x2_t int32x2x2_t;
typedef  int64x2x2_t int64x1x2_t;

/* to avoid pointer conversions the following unsigned integers structures are defined via the corresponding signed integers structures above */
typedef struct int8x16x2_t uint8x16x2_t;
typedef struct int16x8x2_t uint16x8x2_t;
typedef struct int32x4x2_t uint32x4x2_t;
typedef struct int64x2x2_t uint64x2x2_t;
typedef struct int8x16x2_t poly8x16x2_t;
typedef struct int16x8x2_t poly16x8x2_t;

typedef  int8x8x2_t uint8x8x2_t;
typedef  int16x4x2_t uint16x4x2_t;
typedef  int32x2x2_t uint32x2x2_t;
typedef  int64x1x2_t uint64x1x2_t;
typedef  int8x8x2_t poly8x8x2_t;
typedef  int16x4x2_t poly16x4x2_t;

//float
struct float32x4x2_t {
    float32x4_t val[2];
};
struct float16x8x2_t {
    float16x8_t val[2];
};
typedef struct float32x4x2_t float32x4x2_t;         //for C compilers to make them happy
typedef struct float16x8x2_t float16x8x2_t;         //for C compilers to make them happy
typedef  float32x4x2_t float32x2x2_t;
typedef  float16x8x2_t float16x4x2_t;

//4
struct int8x16x4_t {
    int8x16_t val[4];
};
struct int16x8x4_t {
    int16x8_t val[4];
};
struct int32x4x4_t {
    int32x4_t val[4];
};
struct int64x2x4_t {
    int64x2_t val[4];
};

typedef struct int8x16x4_t int8x16x4_t;         //for C compilers to make them happy
typedef struct int16x8x4_t int16x8x4_t;         //for C compilers to make them happy
typedef struct int32x4x4_t int32x4x4_t;         //for C compilers to make them happy
typedef struct int64x2x4_t int64x2x4_t;         //for C compilers to make them happy
typedef  int8x16x4_t int8x8x4_t;
typedef  int16x8x4_t int16x4x4_t;
typedef  int32x4x4_t int32x2x4_t;
typedef  int64x2x4_t int64x1x4_t;

/* to avoid pointer conversions the following unsigned integers structures are defined via the corresponding signed integers dealing structures above:*/
typedef int8x8x4_t uint8x8x4_t;
typedef int16x4x4_t uint16x4x4_t;
typedef int32x2x4_t uint32x2x4_t;
typedef int64x1x4_t uint64x1x4_t;
typedef uint8x8x4_t poly8x8x4_t;
typedef uint16x4x4_t poly16x4x4_t;

typedef struct int8x16x4_t uint8x16x4_t;
typedef struct int16x8x4_t uint16x8x4_t;
typedef struct int32x4x4_t uint32x4x4_t;
typedef struct int64x2x4_t uint64x2x4_t;
typedef struct int8x16x4_t poly8x16x4_t;
typedef struct int16x8x4_t poly16x8x4_t;

struct float32x4x4_t {
    float32x4_t val[4];
};
struct float16x8x4_t {
    float16x8_t val[4];
};

typedef struct float32x4x4_t float32x4x4_t;         //for C compilers to make them happy
typedef struct float16x8x4_t float16x8x4_t;         //for C compilers to make them happy
typedef  float32x4x4_t float32x2x4_t;
typedef  float16x8x4_t float16x4x4_t;

//3
struct int16x8x3_t {
    int16x8_t val[3];
};
struct int32x4x3_t {
    int32x4_t val[3];
};
struct int64x2x3_t {
    int64x2_t val[3];
};
struct int8x16x3_t {
    int8x16_t val[3];
};

typedef struct int16x8x3_t int16x8x3_t;         //for C compilers to make them happy
typedef struct int32x4x3_t int32x4x3_t;         //for C compilers to make them happy
typedef struct int64x2x3_t int64x2x3_t;         //for C compilers to make them happy
typedef struct int8x16x3_t int8x16x3_t;         //for C compilers to make them happy
typedef  int16x8x3_t int16x4x3_t;
typedef  int32x4x3_t int32x2x3_t;
typedef  int64x2x3_t int64x1x3_t;
typedef  int8x16x3_t int8x8x3_t;

/* to avoid pointer conversions the following unsigned integers structures are defined via the corresponding signed integers dealing structures above:*/
typedef struct int8x16x3_t uint8x16x3_t;
typedef struct int16x8x3_t uint16x8x3_t;
typedef struct int32x4x3_t uint32x4x3_t;
typedef struct int64x2x3_t uint64x2x3_t;
typedef struct int8x16x3_t poly8x16x3_t;
typedef struct int16x8x3_t poly16x8x3_t;
typedef int8x8x3_t uint8x8x3_t;
typedef int16x4x3_t uint16x4x3_t;
typedef int32x2x3_t uint32x2x3_t;
typedef int64x1x3_t uint64x1x3_t;
typedef int8x8x3_t poly8x8x3_t;
typedef int16x4x3_t poly16x4x3_t;

//float
struct float32x4x3_t {
    float32x4_t val[3];
};
struct float16x8x3_t {
    float16x8_t val[3];
};

typedef struct float32x4x3_t float32x4x3_t;         //for C compilers to make them happy
typedef struct float16x8x3_t float16x8x3_t;         //for C compilers to make them happy
typedef  float32x4x3_t float32x2x3_t;
typedef  float16x8x3_t float16x4x3_t;

//****************************************************************************
//****** Porting auxiliary macros ********************************************
#define _M128i(a) (*(__m128i*)&(a))
#define _M128d(a) (*(__m128d*)&(a))
#define _M128(a) (*(__m128*)&(a))
#define _Ui64(a) (*(uint64_t*)&(a))
#define _UNSIGNED_T(a) u##a

#define _SIGNBIT64 ((uint64_t)1 << 63)
#define _SWAP_HI_LOW32  (2 | (3 << 2) | (0 << 4) | (1 << 6))
#define _INSERTPS_NDX(srcField, dstField) (((srcField) << 6) | ((dstField) << 4) )

#define  _NEON2SSE_REASON_SLOW_SERIAL "The function may be very slow due to the serial implementation, please try to avoid it"
#define  _NEON2SSE_REASON_SLOW_UNEFFECTIVE "The function may be slow due to inefficient x86 SIMD implementation, please try to avoid it"

//***************  functions attributes  ********************************************
//***********************************************************************************
#ifdef __GNUC__
    #define _GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
    #define _NEON2SSE_ALIGN_16  __attribute__((aligned(16)))
    #define _NEON2SSE_INLINE extern inline __attribute__((__gnu_inline__, __always_inline__, __artificial__))
    #if _GCC_VERSION <  40500
        #define _NEON2SSE_PERFORMANCE_WARNING(function, explanation)   __attribute__((deprecated)) function
    #else
        #define _NEON2SSE_PERFORMANCE_WARNING(function, explanation)   __attribute__((deprecated(explanation))) function
    #endif
#elif defined(_MSC_VER)|| defined (__INTEL_COMPILER)
    #define _NEON2SSE_ALIGN_16  __declspec(align(16))
    #define _NEON2SSE_INLINE __inline
    #define _NEON2SSE_PERFORMANCE_WARNING(function, EXPLANATION) __declspec(deprecated(EXPLANATION)) function
#else
    #define _NEON2SSE_ALIGN_16  __declspec(align(16))
    #define _NEON2SSE_INLINE inline
    #define _NEON2SSE_PERFORMANCE_WARNING(function, explanation)  function
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define __constrange(min,max)  const
#define __transfersize(size)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//*************************************************************************
//*************************************************************************
//*********  Functions declarations as declared in original arm_neon.h *****
//*************************************************************************
//Vector add: vadd -> Vr[i]:=Va[i]+Vb[i], Vr, Va, Vb have equal lane sizes.

int8x16_t vaddq_s8(int8x16_t a, int8x16_t b);         // VADD.I8 q0,q0,q0
int16x8_t vaddq_s16(int16x8_t a, int16x8_t b);         // VADD.I16 q0,q0,q0
int32x4_t vaddq_s32(int32x4_t a, int32x4_t b);         // VADD.I32 q0,q0,q0
int64x2_t vaddq_s64(int64x2_t a, int64x2_t b);         // VADD.I64 q0,q0,q0
float32x4_t vaddq_f32(float32x4_t a, float32x4_t b);         // VADD.F32 q0,q0,q0
uint8x16_t vaddq_u8(uint8x16_t a, uint8x16_t b);         // VADD.I8 q0,q0,q0
uint16x8_t vaddq_u16(uint16x8_t a, uint16x8_t b);         // VADD.I16 q0,q0,q0
uint32x4_t vaddq_u32(uint32x4_t a, uint32x4_t b);         // VADD.I32 q0,q0,q0
uint64x2_t vaddq_u64(uint64x2_t a, uint64x2_t b);         // VADD.I64 q0,q0,q0
//Vector long add: vaddl -> Vr[i]:=Va[i]+Vb[i], Va, Vb have equal lane sizes, result is a 128 bit vector of lanes that are twice the width.

//Vector wide addw: vadd -> Vr[i]:=Va[i]+Vb[i]

//Vector halving add: vhadd -> Vr[i]:=(Va[i]+Vb[i])>>1

int8x16_t vhaddq_s8(int8x16_t a, int8x16_t b);         // VHADD.S8 q0,q0,q0
int16x8_t vhaddq_s16(int16x8_t a, int16x8_t b);         // VHADD.S16 q0,q0,q0
int32x4_t vhaddq_s32(int32x4_t a, int32x4_t b);         // VHADD.S32 q0,q0,q0
uint8x16_t vhaddq_u8(uint8x16_t a, uint8x16_t b);         // VHADD.U8 q0,q0,q0
uint16x8_t vhaddq_u16(uint16x8_t a, uint16x8_t b);         // VHADD.U16 q0,q0,q0
uint32x4_t vhaddq_u32(uint32x4_t a, uint32x4_t b);         // VHADD.U32 q0,q0,q0
//Vector rounding halving add: vrhadd -> Vr[i]:=(Va[i]+Vb[i]+1)>>1

int8x16_t vrhaddq_s8(int8x16_t a, int8x16_t b);         // VRHADD.S8 q0,q0,q0
int16x8_t vrhaddq_s16(int16x8_t a, int16x8_t b);         // VRHADD.S16 q0,q0,q0
int32x4_t vrhaddq_s32(int32x4_t a, int32x4_t b);         // VRHADD.S32 q0,q0,q0
uint8x16_t vrhaddq_u8(uint8x16_t a, uint8x16_t b);         // VRHADD.U8 q0,q0,q0
uint16x8_t vrhaddq_u16(uint16x8_t a, uint16x8_t b);         // VRHADD.U16 q0,q0,q0
uint32x4_t vrhaddq_u32(uint32x4_t a, uint32x4_t b);         // VRHADD.U32 q0,q0,q0
//Vector saturating add: vqadd -> Vr[i]:=sat<size>(Va[i]+Vb[i])

int8x16_t vqaddq_s8(int8x16_t a, int8x16_t b);         // VQADD.S8 q0,q0,q0
int16x8_t vqaddq_s16(int16x8_t a, int16x8_t b);         // VQADD.S16 q0,q0,q0
int32x4_t vqaddq_s32(int32x4_t a, int32x4_t b);         // VQADD.S32 q0,q0,q0
int64x2_t vqaddq_s64(int64x2_t a, int64x2_t b);         // VQADD.S64 q0,q0,q0
uint8x16_t vqaddq_u8(uint8x16_t a, uint8x16_t b);         // VQADD.U8 q0,q0,q0
uint16x8_t vqaddq_u16(uint16x8_t a, uint16x8_t b);         // VQADD.U16 q0,q0,q0
uint32x4_t vqaddq_u32(uint32x4_t a, uint32x4_t b);         // VQADD.U32 q0,q0,q0
uint64x2_t vqaddq_u64(uint64x2_t a, uint64x2_t b);         // VQADD.U64 q0,q0,q0
//Vector add high half: vaddhn-> Vr[i]:=Va[i]+Vb[i]

//Vector rounding add high half: vraddhn

//Multiplication
//Vector multiply: vmul -> Vr[i] := Va[i] * Vb[i]

int8x16_t vmulq_s8(int8x16_t a, int8x16_t b);         // VMUL.I8 q0,q0,q0
int16x8_t vmulq_s16(int16x8_t a, int16x8_t b);         // VMUL.I16 q0,q0,q0
int32x4_t vmulq_s32(int32x4_t a, int32x4_t b);         // VMUL.I32 q0,q0,q0
float32x4_t vmulq_f32(float32x4_t a, float32x4_t b);         // VMUL.F32 q0,q0,q0
uint8x16_t vmulq_u8(uint8x16_t a, uint8x16_t b);         // VMUL.I8 q0,q0,q0
uint16x8_t vmulq_u16(uint16x8_t a, uint16x8_t b);         // VMUL.I16 q0,q0,q0
uint32x4_t vmulq_u32(uint32x4_t a, uint32x4_t b);         // VMUL.I32 q0,q0,q0
poly8x16_t vmulq_p8(poly8x16_t a, poly8x16_t b);         // VMUL.P8 q0,q0,q0
//Vector multiply accumulate: vmla -> Vr[i] := Va[i] + Vb[i] * Vc[i]

int8x16_t vmlaq_s8(int8x16_t a, int8x16_t b, int8x16_t c);         // VMLA.I8 q0,q0,q0
int16x8_t vmlaq_s16(int16x8_t a, int16x8_t b, int16x8_t c);         // VMLA.I16 q0,q0,q0
int32x4_t vmlaq_s32(int32x4_t a, int32x4_t b, int32x4_t c);         // VMLA.I32 q0,q0,q0
float32x4_t vmlaq_f32(float32x4_t a, float32x4_t b, float32x4_t c);         // VMLA.F32 q0,q0,q0
uint8x16_t vmlaq_u8(uint8x16_t a, uint8x16_t b, uint8x16_t c);         // VMLA.I8 q0,q0,q0
uint16x8_t vmlaq_u16(uint16x8_t a, uint16x8_t b, uint16x8_t c);         // VMLA.I16 q0,q0,q0
uint32x4_t vmlaq_u32(uint32x4_t a, uint32x4_t b, uint32x4_t c);         // VMLA.I32 q0,q0,q0
//Vector multiply accumulate long: vmlal -> Vr[i] := Va[i] + Vb[i] * Vc[i]

//Vector multiply subtract: vmls -> Vr[i] := Va[i] - Vb[i] * Vc[i]

int8x16_t vmlsq_s8(int8x16_t a, int8x16_t b, int8x16_t c);         // VMLS.I8 q0,q0,q0
int16x8_t vmlsq_s16(int16x8_t a, int16x8_t b, int16x8_t c);         // VMLS.I16 q0,q0,q0
int32x4_t vmlsq_s32(int32x4_t a, int32x4_t b, int32x4_t c);         // VMLS.I32 q0,q0,q0
float32x4_t vmlsq_f32(float32x4_t a, float32x4_t b, float32x4_t c);         // VMLS.F32 q0,q0,q0
uint8x16_t vmlsq_u8(uint8x16_t a, uint8x16_t b, uint8x16_t c);         // VMLS.I8 q0,q0,q0
uint16x8_t vmlsq_u16(uint16x8_t a, uint16x8_t b, uint16x8_t c);         // VMLS.I16 q0,q0,q0
uint32x4_t vmlsq_u32(uint32x4_t a, uint32x4_t b, uint32x4_t c);         // VMLS.I32 q0,q0,q0
//Vector multiply subtract long

//Vector saturating doubling multiply high

int16x8_t vqdmulhq_s16(int16x8_t a, int16x8_t b);         // VQDMULH.S16 q0,q0,q0
int32x4_t vqdmulhq_s32(int32x4_t a, int32x4_t b);         // VQDMULH.S32 q0,q0,q0
//Vector saturating rounding doubling multiply high

int16x8_t vqrdmulhq_s16(int16x8_t a, int16x8_t b);         // VQRDMULH.S16 q0,q0,q0
int32x4_t vqrdmulhq_s32(int32x4_t a, int32x4_t b);         // VQRDMULH.S32 q0,q0,q0
//Vector saturating doubling multiply accumulate long

//Vector saturating doubling multiply subtract long

//Vector long multiply

//Vector saturating doubling long multiply

//Subtraction
//Vector subtract

int8x16_t vsubq_s8(int8x16_t a, int8x16_t b);         // VSUB.I8 q0,q0,q0
int16x8_t vsubq_s16(int16x8_t a, int16x8_t b);         // VSUB.I16 q0,q0,q0
int32x4_t vsubq_s32(int32x4_t a, int32x4_t b);         // VSUB.I32 q0,q0,q0
int64x2_t vsubq_s64(int64x2_t a, int64x2_t b);         // VSUB.I64 q0,q0,q0
float32x4_t vsubq_f32(float32x4_t a, float32x4_t b);         // VSUB.F32 q0,q0,q0
uint8x16_t vsubq_u8(uint8x16_t a, uint8x16_t b);         // VSUB.I8 q0,q0,q0
uint16x8_t vsubq_u16(uint16x8_t a, uint16x8_t b);         // VSUB.I16 q0,q0,q0
uint32x4_t vsubq_u32(uint32x4_t a, uint32x4_t b);         // VSUB.I32 q0,q0,q0
uint64x2_t vsubq_u64(uint64x2_t a, uint64x2_t b);         // VSUB.I64 q0,q0,q0
//Vector long subtract: vsub -> Vr[i]:=Va[i]+Vb[i]

//Vector wide subtract: vsub -> Vr[i]:=Va[i]+Vb[i]

//Vector saturating subtract

int8x16_t vqsubq_s8(int8x16_t a, int8x16_t b);         // VQSUB.S8 q0,q0,q0
int16x8_t vqsubq_s16(int16x8_t a, int16x8_t b);         // VQSUB.S16 q0,q0,q0
int32x4_t vqsubq_s32(int32x4_t a, int32x4_t b);         // VQSUB.S32 q0,q0,q0
int64x2_t vqsubq_s64(int64x2_t a, int64x2_t b);         // VQSUB.S64 q0,q0,q0
uint8x16_t vqsubq_u8(uint8x16_t a, uint8x16_t b);         // VQSUB.U8 q0,q0,q0
uint16x8_t vqsubq_u16(uint16x8_t a, uint16x8_t b);         // VQSUB.U16 q0,q0,q0
uint32x4_t vqsubq_u32(uint32x4_t a, uint32x4_t b);         // VQSUB.U32 q0,q0,q0
uint64x2_t vqsubq_u64(uint64x2_t a, uint64x2_t b);         // VQSUB.U64 q0,q0,q0
//Vector halving subtract

int8x16_t vhsubq_s8(int8x16_t a, int8x16_t b);         // VHSUB.S8 q0,q0,q0
int16x8_t vhsubq_s16(int16x8_t a, int16x8_t b);         // VHSUB.S16 q0,q0,q0
int32x4_t vhsubq_s32(int32x4_t a, int32x4_t b);         // VHSUB.S32 q0,q0,q0
uint8x16_t vhsubq_u8(uint8x16_t a, uint8x16_t b);         // VHSUB.U8 q0,q0,q0
uint16x8_t vhsubq_u16(uint16x8_t a, uint16x8_t b);         // VHSUB.U16 q0,q0,q0
uint32x4_t vhsubq_u32(uint32x4_t a, uint32x4_t b);         // VHSUB.U32 q0,q0,q0
//Vector subtract high half

//Vector rounding subtract high half

//Comparison
//Vector compare equal

uint8x16_t vceqq_s8(int8x16_t a, int8x16_t b);         // VCEQ.I8 q0, q0, q0
uint16x8_t vceqq_s16(int16x8_t a, int16x8_t b);         // VCEQ.I16 q0, q0, q0
uint32x4_t vceqq_s32(int32x4_t a, int32x4_t b);         // VCEQ.I32 q0, q0, q0
uint32x4_t vceqq_f32(float32x4_t a, float32x4_t b);         // VCEQ.F32 q0, q0, q0
uint8x16_t vceqq_u8(uint8x16_t a, uint8x16_t b);         // VCEQ.I8 q0, q0, q0
uint16x8_t vceqq_u16(uint16x8_t a, uint16x8_t b);         // VCEQ.I16 q0, q0, q0
uint32x4_t vceqq_u32(uint32x4_t a, uint32x4_t b);         // VCEQ.I32 q0, q0, q0
uint8x16_t vceqq_p8(poly8x16_t a, poly8x16_t b);         // VCEQ.I8 q0, q0, q0
//Vector compare greater-than or equal

uint8x16_t vcgeq_s8(int8x16_t a, int8x16_t b);         // VCGE.S8 q0, q0, q0
uint16x8_t vcgeq_s16(int16x8_t a, int16x8_t b);         // VCGE.S16 q0, q0, q0
uint32x4_t vcgeq_s32(int32x4_t a, int32x4_t b);         // VCGE.S32 q0, q0, q0
uint32x4_t vcgeq_f32(float32x4_t a, float32x4_t b);         // VCGE.F32 q0, q0, q0
uint8x16_t vcgeq_u8(uint8x16_t a, uint8x16_t b);         // VCGE.U8 q0, q0, q0
uint16x8_t vcgeq_u16(uint16x8_t a, uint16x8_t b);         // VCGE.U16 q0, q0, q0
uint32x4_t vcgeq_u32(uint32x4_t a, uint32x4_t b);         // VCGE.U32 q0, q0, q0
//Vector compare less-than or equal

uint8x16_t vcleq_s8(int8x16_t a, int8x16_t b);         // VCGE.S8 q0, q0, q0
uint16x8_t vcleq_s16(int16x8_t a, int16x8_t b);         // VCGE.S16 q0, q0, q0
uint32x4_t vcleq_s32(int32x4_t a, int32x4_t b);         // VCGE.S32 q0, q0, q0
uint32x4_t vcleq_f32(float32x4_t a, float32x4_t b);         // VCGE.F32 q0, q0, q0
uint8x16_t vcleq_u8(uint8x16_t a, uint8x16_t b);         // VCGE.U8 q0, q0, q0
uint16x8_t vcleq_u16(uint16x8_t a, uint16x8_t b);         // VCGE.U16 q0, q0, q0
uint32x4_t vcleq_u32(uint32x4_t a, uint32x4_t b);         // VCGE.U32 q0, q0, q0
//Vector compare greater-than

uint8x16_t vcgtq_s8(int8x16_t a, int8x16_t b);         // VCGT.S8 q0, q0, q0
uint16x8_t vcgtq_s16(int16x8_t a, int16x8_t b);         // VCGT.S16 q0, q0, q0
uint32x4_t vcgtq_s32(int32x4_t a, int32x4_t b);         // VCGT.S32 q0, q0, q0
uint32x4_t vcgtq_f32(float32x4_t a, float32x4_t b);         // VCGT.F32 q0, q0, q0
uint8x16_t vcgtq_u8(uint8x16_t a, uint8x16_t b);         // VCGT.U8 q0, q0, q0
uint16x8_t vcgtq_u16(uint16x8_t a, uint16x8_t b);         // VCGT.U16 q0, q0, q0
uint32x4_t vcgtq_u32(uint32x4_t a, uint32x4_t b);         // VCGT.U32 q0, q0, q0
//Vector compare less-than

uint8x16_t vcltq_s8(int8x16_t a, int8x16_t b);         // VCGT.S8 q0, q0, q0
uint16x8_t vcltq_s16(int16x8_t a, int16x8_t b);         // VCGT.S16 q0, q0, q0
uint32x4_t vcltq_s32(int32x4_t a, int32x4_t b);         // VCGT.S32 q0, q0, q0
uint32x4_t vcltq_f32(float32x4_t a, float32x4_t b);         // VCGT.F32 q0, q0, q0
uint8x16_t vcltq_u8(uint8x16_t a, uint8x16_t b);         // VCGT.U8 q0, q0, q0
uint16x8_t vcltq_u16(uint16x8_t a, uint16x8_t b);         // VCGT.U16 q0, q0, q0
uint32x4_t vcltq_u32(uint32x4_t a, uint32x4_t b);         // VCGT.U32 q0, q0, q0
//Vector compare absolute greater-than or equal

uint32x4_t vcageq_f32(float32x4_t a, float32x4_t b);         // VACGE.F32 q0, q0, q0
//Vector compare absolute less-than or equal

uint32x4_t vcaleq_f32(float32x4_t a, float32x4_t b);         // VACGE.F32 q0, q0, q0
//Vector compare absolute greater-than

uint32x4_t vcagtq_f32(float32x4_t a, float32x4_t b);         // VACGT.F32 q0, q0, q0
//Vector compare absolute less-than

uint32x4_t vcaltq_f32(float32x4_t a, float32x4_t b);         // VACGT.F32 q0, q0, q0
//Vector test bits

uint8x16_t vtstq_s8(int8x16_t a, int8x16_t b);         // VTST.8 q0, q0, q0
uint16x8_t vtstq_s16(int16x8_t a, int16x8_t b);         // VTST.16 q0, q0, q0
uint32x4_t vtstq_s32(int32x4_t a, int32x4_t b);         // VTST.32 q0, q0, q0
uint8x16_t vtstq_u8(uint8x16_t a, uint8x16_t b);         // VTST.8 q0, q0, q0
uint16x8_t vtstq_u16(uint16x8_t a, uint16x8_t b);         // VTST.16 q0, q0, q0
uint32x4_t vtstq_u32(uint32x4_t a, uint32x4_t b);         // VTST.32 q0, q0, q0
uint8x16_t vtstq_p8(poly8x16_t a, poly8x16_t b);         // VTST.8 q0, q0, q0
//Absolute difference
//Absolute difference between the arguments: Vr[i] = | Va[i] - Vb[i] |

int8x16_t vabdq_s8(int8x16_t a, int8x16_t b);         // VABD.S8 q0,q0,q0
int16x8_t vabdq_s16(int16x8_t a, int16x8_t b);         // VABD.S16 q0,q0,q0
int32x4_t vabdq_s32(int32x4_t a, int32x4_t b);         // VABD.S32 q0,q0,q0
uint8x16_t vabdq_u8(uint8x16_t a, uint8x16_t b);         // VABD.U8 q0,q0,q0
uint16x8_t vabdq_u16(uint16x8_t a, uint16x8_t b);         // VABD.U16 q0,q0,q0
uint32x4_t vabdq_u32(uint32x4_t a, uint32x4_t b);         // VABD.U32 q0,q0,q0
float32x4_t vabdq_f32(float32x4_t a, float32x4_t b);         // VABD.F32 q0,q0,q0
//Absolute difference - long

//Absolute difference and accumulate: Vr[i] = Va[i] + | Vb[i] - Vc[i] |

int8x16_t vabaq_s8(int8x16_t a, int8x16_t b, int8x16_t c);         // VABA.S8 q0,q0,q0
int16x8_t vabaq_s16(int16x8_t a, int16x8_t b, int16x8_t c);         // VABA.S16 q0,q0,q0
int32x4_t vabaq_s32(int32x4_t a, int32x4_t b, int32x4_t c);         // VABA.S32 q0,q0,q0
uint8x16_t vabaq_u8(uint8x16_t a, uint8x16_t b, uint8x16_t c);         // VABA.U8 q0,q0,q0
uint16x8_t vabaq_u16(uint16x8_t a, uint16x8_t b, uint16x8_t c);         // VABA.U16 q0,q0,q0
uint32x4_t vabaq_u32(uint32x4_t a, uint32x4_t b, uint32x4_t c);         // VABA.U32 q0,q0,q0
//Absolute difference and accumulate - long

//Max/Min
//vmax -> Vr[i] := (Va[i] >= Vb[i]) ? Va[i] : Vb[i]

int8x16_t vmaxq_s8(int8x16_t a, int8x16_t b);         // VMAX.S8 q0,q0,q0
int16x8_t vmaxq_s16(int16x8_t a, int16x8_t b);         // VMAX.S16 q0,q0,q0
int32x4_t vmaxq_s32(int32x4_t a, int32x4_t b);         // VMAX.S32 q0,q0,q0
uint8x16_t vmaxq_u8(uint8x16_t a, uint8x16_t b);         // VMAX.U8 q0,q0,q0
uint16x8_t vmaxq_u16(uint16x8_t a, uint16x8_t b);         // VMAX.U16 q0,q0,q0
uint32x4_t vmaxq_u32(uint32x4_t a, uint32x4_t b);         // VMAX.U32 q0,q0,q0
float32x4_t vmaxq_f32(float32x4_t a, float32x4_t b);         // VMAX.F32 q0,q0,q0
//vmin -> Vr[i] := (Va[i] >= Vb[i]) ? Vb[i] : Va[i]

int8x16_t vminq_s8(int8x16_t a, int8x16_t b);         // VMIN.S8 q0,q0,q0
int16x8_t vminq_s16(int16x8_t a, int16x8_t b);         // VMIN.S16 q0,q0,q0
int32x4_t vminq_s32(int32x4_t a, int32x4_t b);         // VMIN.S32 q0,q0,q0
uint8x16_t vminq_u8(uint8x16_t a, uint8x16_t b);         // VMIN.U8 q0,q0,q0
uint16x8_t vminq_u16(uint16x8_t a, uint16x8_t b);         // VMIN.U16 q0,q0,q0
uint32x4_t vminq_u32(uint32x4_t a, uint32x4_t b);         // VMIN.U32 q0,q0,q0
float32x4_t vminq_f32(float32x4_t a, float32x4_t b);         // VMIN.F32 q0,q0,q0
//Pairwise addition
//Pairwise add

//Long pairwise add

int16x8_t vpaddlq_s8(int8x16_t a);         // VPADDL.S8 q0,q0
int32x4_t vpaddlq_s16(int16x8_t a);         // VPADDL.S16 q0,q0
int64x2_t vpaddlq_s32(int32x4_t a);         // VPADDL.S32 q0,q0
uint16x8_t vpaddlq_u8(uint8x16_t a);         // VPADDL.U8 q0,q0
uint32x4_t vpaddlq_u16(uint16x8_t a);         // VPADDL.U16 q0,q0
uint64x2_t vpaddlq_u32(uint32x4_t a);         // VPADDL.U32 q0,q0
//Long pairwise add and accumulate

int16x8_t vpadalq_s8(int16x8_t a, int8x16_t b);         // VPADAL.S8 q0,q0
int32x4_t vpadalq_s16(int32x4_t a, int16x8_t b);         // VPADAL.S16 q0,q0
int64x2_t vpadalq_s32(int64x2_t a, int32x4_t b);         // VPADAL.S32 q0,q0
uint16x8_t vpadalq_u8(uint16x8_t a, uint8x16_t b);         // VPADAL.U8 q0,q0
uint32x4_t vpadalq_u16(uint32x4_t a, uint16x8_t b);         // VPADAL.U16 q0,q0
uint64x2_t vpadalq_u32(uint64x2_t a, uint32x4_t b);         // VPADAL.U32 q0,q0
//Folding maximum vpmax -> takes maximum of adjacent pairs

//Folding minimum vpmin -> takes minimum of adjacent pairs

//Reciprocal/Sqrt

float32x4_t vrecpsq_f32(float32x4_t a, float32x4_t b);         // VRECPS.F32 q0, q0, q0

float32x4_t vrsqrtsq_f32(float32x4_t a, float32x4_t b);         // VRSQRTS.F32 q0, q0, q0
//Shifts by signed variable
//Vector shift left: Vr[i] := Va[i] << Vb[i] (negative values shift right)

int8x16_t vshlq_s8(int8x16_t a, int8x16_t b);         // VSHL.S8 q0,q0,q0
int16x8_t vshlq_s16(int16x8_t a, int16x8_t b);         // VSHL.S16 q0,q0,q0
int32x4_t vshlq_s32(int32x4_t a, int32x4_t b);         // VSHL.S32 q0,q0,q0
int64x2_t vshlq_s64(int64x2_t a, int64x2_t b);         // VSHL.S64 q0,q0,q0
uint8x16_t vshlq_u8(uint8x16_t a, int8x16_t b);         // VSHL.U8 q0,q0,q0
uint16x8_t vshlq_u16(uint16x8_t a, int16x8_t b);         // VSHL.U16 q0,q0,q0
uint32x4_t vshlq_u32(uint32x4_t a, int32x4_t b);         // VSHL.U32 q0,q0,q0
uint64x2_t vshlq_u64(uint64x2_t a, int64x2_t b);         // VSHL.U64 q0,q0,q0
//Vector saturating shift left: (negative values shift right)

int8x16_t vqshlq_s8(int8x16_t a, int8x16_t b);         // VQSHL.S8 q0,q0,q0
int16x8_t vqshlq_s16(int16x8_t a, int16x8_t b);         // VQSHL.S16 q0,q0,q0
int32x4_t vqshlq_s32(int32x4_t a, int32x4_t b);         // VQSHL.S32 q0,q0,q0
int64x2_t vqshlq_s64(int64x2_t a, int64x2_t b);         // VQSHL.S64 q0,q0,q0
uint8x16_t vqshlq_u8(uint8x16_t a, int8x16_t b);         // VQSHL.U8 q0,q0,q0
uint16x8_t vqshlq_u16(uint16x8_t a, int16x8_t b);         // VQSHL.U16 q0,q0,q0
uint32x4_t vqshlq_u32(uint32x4_t a, int32x4_t b);         // VQSHL.U32 q0,q0,q0
uint64x2_t vqshlq_u64(uint64x2_t a, int64x2_t b);         // VQSHL.U64 q0,q0,q0
//Vector rounding shift left: (negative values shift right)

int8x16_t vrshlq_s8(int8x16_t a, int8x16_t b);         // VRSHL.S8 q0,q0,q0
int16x8_t vrshlq_s16(int16x8_t a, int16x8_t b);         // VRSHL.S16 q0,q0,q0
int32x4_t vrshlq_s32(int32x4_t a, int32x4_t b);         // VRSHL.S32 q0,q0,q0
int64x2_t vrshlq_s64(int64x2_t a, int64x2_t b);         // VRSHL.S64 q0,q0,q0
uint8x16_t vrshlq_u8(uint8x16_t a, int8x16_t b);         // VRSHL.U8 q0,q0,q0
uint16x8_t vrshlq_u16(uint16x8_t a, int16x8_t b);         // VRSHL.U16 q0,q0,q0
uint32x4_t vrshlq_u32(uint32x4_t a, int32x4_t b);         // VRSHL.U32 q0,q0,q0
uint64x2_t vrshlq_u64(uint64x2_t a, int64x2_t b);         // VRSHL.U64 q0,q0,q0
//Vector saturating rounding shift left: (negative values shift right)

int8x16_t vqrshlq_s8(int8x16_t a, int8x16_t b);         // VQRSHL.S8 q0,q0,q0
int16x8_t vqrshlq_s16(int16x8_t a, int16x8_t b);         // VQRSHL.S16 q0,q0,q0
int32x4_t vqrshlq_s32(int32x4_t a, int32x4_t b);         // VQRSHL.S32 q0,q0,q0
int64x2_t vqrshlq_s64(int64x2_t a, int64x2_t b);         // VQRSHL.S64 q0,q0,q0
uint8x16_t vqrshlq_u8(uint8x16_t a, int8x16_t b);         // VQRSHL.U8 q0,q0,q0
uint16x8_t vqrshlq_u16(uint16x8_t a, int16x8_t b);         // VQRSHL.U16 q0,q0,q0
uint32x4_t vqrshlq_u32(uint32x4_t a, int32x4_t b);         // VQRSHL.U32 q0,q0,q0
uint64x2_t vqrshlq_u64(uint64x2_t a, int64x2_t b);         // VQRSHL.U64 q0,q0,q0
//Shifts by a constant
//Vector shift right by constant

int8x16_t vshrq_n_s8(int8x16_t a, __constrange(1,8) int b);         // VSHR.S8 q0,q0,#8
int16x8_t vshrq_n_s16(int16x8_t a, __constrange(1,16) int b);         // VSHR.S16 q0,q0,#16
int32x4_t vshrq_n_s32(int32x4_t a, __constrange(1,32) int b);         // VSHR.S32 q0,q0,#32
int64x2_t vshrq_n_s64(int64x2_t a, __constrange(1,64) int b);         // VSHR.S64 q0,q0,#64
uint8x16_t vshrq_n_u8(uint8x16_t a, __constrange(1,8) int b);         // VSHR.U8 q0,q0,#8
uint16x8_t vshrq_n_u16(uint16x8_t a, __constrange(1,16) int b);         // VSHR.U16 q0,q0,#16
uint32x4_t vshrq_n_u32(uint32x4_t a, __constrange(1,32) int b);         // VSHR.U32 q0,q0,#32
uint64x2_t vshrq_n_u64(uint64x2_t a, __constrange(1,64) int b);         // VSHR.U64 q0,q0,#64
//Vector shift left by constant

int8x16_t vshlq_n_s8(int8x16_t a, __constrange(0,7) int b);         // VSHL.I8 q0,q0,#0
int16x8_t vshlq_n_s16(int16x8_t a, __constrange(0,15) int b);         // VSHL.I16 q0,q0,#0
int32x4_t vshlq_n_s32(int32x4_t a, __constrange(0,31) int b);         // VSHL.I32 q0,q0,#0
int64x2_t vshlq_n_s64(int64x2_t a, __constrange(0,63) int b);         // VSHL.I64 q0,q0,#0
uint8x16_t vshlq_n_u8(uint8x16_t a, __constrange(0,7) int b);         // VSHL.I8 q0,q0,#0
uint16x8_t vshlq_n_u16(uint16x8_t a, __constrange(0,15) int b);         // VSHL.I16 q0,q0,#0
uint32x4_t vshlq_n_u32(uint32x4_t a, __constrange(0,31) int b);         // VSHL.I32 q0,q0,#0
uint64x2_t vshlq_n_u64(uint64x2_t a, __constrange(0,63) int b);         // VSHL.I64 q0,q0,#0
//Vector rounding shift right by constant

int8x16_t vrshrq_n_s8(int8x16_t a, __constrange(1,8) int b);         // VRSHR.S8 q0,q0,#8
int16x8_t vrshrq_n_s16(int16x8_t a, __constrange(1,16) int b);         // VRSHR.S16 q0,q0,#16
int32x4_t vrshrq_n_s32(int32x4_t a, __constrange(1,32) int b);         // VRSHR.S32 q0,q0,#32
int64x2_t vrshrq_n_s64(int64x2_t a, __constrange(1,64) int b);         // VRSHR.S64 q0,q0,#64
uint8x16_t vrshrq_n_u8(uint8x16_t a, __constrange(1,8) int b);         // VRSHR.U8 q0,q0,#8
uint16x8_t vrshrq_n_u16(uint16x8_t a, __constrange(1,16) int b);         // VRSHR.U16 q0,q0,#16
uint32x4_t vrshrq_n_u32(uint32x4_t a, __constrange(1,32) int b);         // VRSHR.U32 q0,q0,#32
uint64x2_t vrshrq_n_u64(uint64x2_t a, __constrange(1,64) int b);         // VRSHR.U64 q0,q0,#64
//Vector shift right by constant and accumulate

int8x16_t vsraq_n_s8(int8x16_t a, int8x16_t b, __constrange(1,8) int c);         // VSRA.S8 q0,q0,#8
int16x8_t vsraq_n_s16(int16x8_t a, int16x8_t b, __constrange(1,16) int c);         // VSRA.S16 q0,q0,#16
int32x4_t vsraq_n_s32(int32x4_t a, int32x4_t b, __constrange(1,32) int c);         // VSRA.S32 q0,q0,#32
int64x2_t vsraq_n_s64(int64x2_t a, int64x2_t b, __constrange(1,64) int c);         // VSRA.S64 q0,q0,#64
uint8x16_t vsraq_n_u8(uint8x16_t a, uint8x16_t b, __constrange(1,8) int c);         // VSRA.U8 q0,q0,#8
uint16x8_t vsraq_n_u16(uint16x8_t a, uint16x8_t b, __constrange(1,16) int c);         // VSRA.U16 q0,q0,#16
uint32x4_t vsraq_n_u32(uint32x4_t a, uint32x4_t b, __constrange(1,32) int c);         // VSRA.U32 q0,q0,#32
uint64x2_t vsraq_n_u64(uint64x2_t a, uint64x2_t b, __constrange(1,64) int c);         // VSRA.U64 q0,q0,#64
//Vector rounding shift right by constant and accumulate

int8x16_t vrsraq_n_s8(int8x16_t a, int8x16_t b, __constrange(1,8) int c);         // VRSRA.S8 q0,q0,#8
int16x8_t vrsraq_n_s16(int16x8_t a, int16x8_t b, __constrange(1,16) int c);         // VRSRA.S16 q0,q0,#16
int32x4_t vrsraq_n_s32(int32x4_t a, int32x4_t b, __constrange(1,32) int c);         // VRSRA.S32 q0,q0,#32
int64x2_t vrsraq_n_s64(int64x2_t a, int64x2_t b, __constrange(1,64) int c);         // VRSRA.S64 q0,q0,#64
uint8x16_t vrsraq_n_u8(uint8x16_t a, uint8x16_t b, __constrange(1,8) int c);         // VRSRA.U8 q0,q0,#8
uint16x8_t vrsraq_n_u16(uint16x8_t a, uint16x8_t b, __constrange(1,16) int c);         // VRSRA.U16 q0,q0,#16
uint32x4_t vrsraq_n_u32(uint32x4_t a, uint32x4_t b, __constrange(1,32) int c);         // VRSRA.U32 q0,q0,#32
uint64x2_t vrsraq_n_u64(uint64x2_t a, uint64x2_t b, __constrange(1,64) int c);         // VRSRA.U64 q0,q0,#64
//Vector saturating shift left by constant

int8x16_t vqshlq_n_s8(int8x16_t a, __constrange(0,7) int b);         // VQSHL.S8 q0,q0,#0
int16x8_t vqshlq_n_s16(int16x8_t a, __constrange(0,15) int b);         // VQSHL.S16 q0,q0,#0
int32x4_t vqshlq_n_s32(int32x4_t a, __constrange(0,31) int b);         // VQSHL.S32 q0,q0,#0
int64x2_t vqshlq_n_s64(int64x2_t a, __constrange(0,63) int b);         // VQSHL.S64 q0,q0,#0
uint8x16_t vqshlq_n_u8(uint8x16_t a, __constrange(0,7) int b);         // VQSHL.U8 q0,q0,#0
uint16x8_t vqshlq_n_u16(uint16x8_t a, __constrange(0,15) int b);         // VQSHL.U16 q0,q0,#0
uint32x4_t vqshlq_n_u32(uint32x4_t a, __constrange(0,31) int b);         // VQSHL.U32 q0,q0,#0
uint64x2_t vqshlq_n_u64(uint64x2_t a, __constrange(0,63) int b);         // VQSHL.U64 q0,q0,#0
//Vector signed->unsigned saturating shift left by constant

uint8x16_t vqshluq_n_s8(int8x16_t a, __constrange(0,7) int b);         // VQSHLU.S8 q0,q0,#0
uint16x8_t vqshluq_n_s16(int16x8_t a, __constrange(0,15) int b);         // VQSHLU.S16 q0,q0,#0
uint32x4_t vqshluq_n_s32(int32x4_t a, __constrange(0,31) int b);         // VQSHLU.S32 q0,q0,#0
uint64x2_t vqshluq_n_s64(int64x2_t a, __constrange(0,63) int b);         // VQSHLU.S64 q0,q0,#0
//Vector narrowing shift right by constant

//Vector signed->unsigned narrowing saturating shift right by constant

//Vector signed->unsigned rounding narrowing saturating shift right by constant

//Vector narrowing saturating shift right by constant

//Vector rounding narrowing shift right by constant

//Vector rounding narrowing saturating shift right by constant

//Vector widening shift left by constant

//Shifts with insert
//Vector shift right and insert

int8x16_t vsriq_n_s8(int8x16_t a, int8x16_t b, __constrange(1,8) int c);         // VSRI.8 q0,q0,#8
int16x8_t vsriq_n_s16(int16x8_t a, int16x8_t b, __constrange(1,16) int c);         // VSRI.16 q0,q0,#16
int32x4_t vsriq_n_s32(int32x4_t a, int32x4_t b, __constrange(1,32) int c);         // VSRI.32 q0,q0,#32
int64x2_t vsriq_n_s64(int64x2_t a, int64x2_t b, __constrange(1,64) int c);         // VSRI.64 q0,q0,#64
uint8x16_t vsriq_n_u8(uint8x16_t a, uint8x16_t b, __constrange(1,8) int c);         // VSRI.8 q0,q0,#8
uint16x8_t vsriq_n_u16(uint16x8_t a, uint16x8_t b, __constrange(1,16) int c);         // VSRI.16 q0,q0,#16
uint32x4_t vsriq_n_u32(uint32x4_t a, uint32x4_t b, __constrange(1,32) int c);         // VSRI.32 q0,q0,#32
uint64x2_t vsriq_n_u64(uint64x2_t a, uint64x2_t b, __constrange(1,64) int c);         // VSRI.64 q0,q0,#64
poly8x16_t vsriq_n_p8(poly8x16_t a, poly8x16_t b, __constrange(1,8) int c);         // VSRI.8 q0,q0,#8
poly16x8_t vsriq_n_p16(poly16x8_t a, poly16x8_t b, __constrange(1,16) int c);         // VSRI.16 q0,q0,#16
//Vector shift left and insert

int8x16_t vsliq_n_s8(int8x16_t a, int8x16_t b, __constrange(0,7) int c);         // VSLI.8 q0,q0,#0
int16x8_t vsliq_n_s16(int16x8_t a, int16x8_t b, __constrange(0,15) int c);         // VSLI.16 q0,q0,#0
int32x4_t vsliq_n_s32(int32x4_t a, int32x4_t b, __constrange(0,31) int c);         // VSLI.32 q0,q0,#0
int64x2_t vsliq_n_s64(int64x2_t a, int64x2_t b, __constrange(0,63) int c);         // VSLI.64 q0,q0,#0
uint8x16_t vsliq_n_u8(uint8x16_t a, uint8x16_t b, __constrange(0,7) int c);         // VSLI.8 q0,q0,#0
uint16x8_t vsliq_n_u16(uint16x8_t a, uint16x8_t b, __constrange(0,15) int c);         // VSLI.16 q0,q0,#0
uint32x4_t vsliq_n_u32(uint32x4_t a, uint32x4_t b, __constrange(0,31) int c);         // VSLI.32 q0,q0,#0
uint64x2_t vsliq_n_u64(uint64x2_t a, uint64x2_t b, __constrange(0,63) int c);         // VSLI.64 q0,q0,#0
poly8x16_t vsliq_n_p8(poly8x16_t a, poly8x16_t b, __constrange(0,7) int c);         // VSLI.8 q0,q0,#0
poly16x8_t vsliq_n_p16(poly16x8_t a, poly16x8_t b, __constrange(0,15) int c);         // VSLI.16 q0,q0,#0
//Loads of a single vector or lane. Perform loads and stores of a single vector of some type.
//Load a single vector from memory
uint8x16_t vld1q_u8(__transfersize(16) uint8_t const * ptr);         // VLD1.8 {d0, d1}, [r0]
uint16x8_t vld1q_u16(__transfersize(8) uint16_t const * ptr);         // VLD1.16 {d0, d1}, [r0]
uint32x4_t vld1q_u32(__transfersize(4) uint32_t const * ptr);         // VLD1.32 {d0, d1}, [r0]
uint64x2_t vld1q_u64(__transfersize(2) uint64_t const * ptr);         // VLD1.64 {d0, d1}, [r0]
int8x16_t vld1q_s8(__transfersize(16) int8_t const * ptr);         // VLD1.8 {d0, d1}, [r0]
int16x8_t vld1q_s16(__transfersize(8) int16_t const * ptr);         // VLD1.16 {d0, d1}, [r0]
int32x4_t vld1q_s32(__transfersize(4) int32_t const * ptr);         // VLD1.32 {d0, d1}, [r0]
int64x2_t vld1q_s64(__transfersize(2) int64_t const * ptr);         // VLD1.64 {d0, d1}, [r0]
float16x8_t vld1q_f16(__transfersize(8) __fp16 const * ptr);         // VLD1.16 {d0, d1}, [r0]
float32x4_t vld1q_f32(__transfersize(4) float32_t const * ptr);         // VLD1.32 {d0, d1}, [r0]
poly8x16_t vld1q_p8(__transfersize(16) poly8_t const * ptr);         // VLD1.8 {d0, d1}, [r0]
poly16x8_t vld1q_p16(__transfersize(8) poly16_t const * ptr);         // VLD1.16 {d0, d1}, [r0]

//Load a single lane from memory
uint8x16_t vld1q_lane_u8(__transfersize(1) uint8_t const * ptr, uint8x16_t vec, __constrange(0,15) int lane);         //VLD1.8 {d0[0]}, [r0]
uint16x8_t vld1q_lane_u16(__transfersize(1) uint16_t const * ptr, uint16x8_t vec, __constrange(0,7) int lane);         // VLD1.16 {d0[0]}, [r0]
uint32x4_t vld1q_lane_u32(__transfersize(1) uint32_t const * ptr, uint32x4_t vec, __constrange(0,3) int lane);         // VLD1.32 {d0[0]}, [r0]
uint64x2_t vld1q_lane_u64(__transfersize(1) uint64_t const * ptr, uint64x2_t vec, __constrange(0,1) int lane);         // VLD1.64 {d0}, [r0]
int8x16_t vld1q_lane_s8(__transfersize(1) int8_t const * ptr, int8x16_t vec, __constrange(0,15) int lane);         //VLD1.8 {d0[0]}, [r0]
int16x8_t vld1q_lane_s16(__transfersize(1) int16_t const * ptr, int16x8_t vec, __constrange(0,7) int lane);         //VLD1.16 {d0[0]}, [r0]
int32x4_t vld1q_lane_s32(__transfersize(1) int32_t const * ptr, int32x4_t vec, __constrange(0,3) int lane);         //VLD1.32 {d0[0]}, [r0]
float32x4_t vld1q_lane_f32(__transfersize(1) float32_t const * ptr, float32x4_t vec, __constrange(0,3) int lane);         // VLD1.32 {d0[0]}, [r0]
int64x2_t vld1q_lane_s64(__transfersize(1) int64_t const * ptr, int64x2_t vec, __constrange(0,1) int lane);         //VLD1.64 {d0}, [r0]
poly8x16_t vld1q_lane_p8(__transfersize(1) poly8_t const * ptr, poly8x16_t vec, __constrange(0,15) int lane);         //VLD1.8 {d0[0]}, [r0]
poly16x8_t vld1q_lane_p16(__transfersize(1) poly16_t const * ptr, poly16x8_t vec, __constrange(0,7) int lane);         // VLD1.16 {d0[0]}, [r0]

//Load all lanes of vector with same value from memory
uint8x16_t vld1q_dup_u8(__transfersize(1) uint8_t const * ptr);         // VLD1.8 {d0[]}, [r0]
uint16x8_t vld1q_dup_u16(__transfersize(1) uint16_t const * ptr);         // VLD1.16 {d0[]}, [r0]
uint32x4_t vld1q_dup_u32(__transfersize(1) uint32_t const * ptr);         // VLD1.32 {d0[]}, [r0]
uint64x2_t vld1q_dup_u64(__transfersize(1) uint64_t const * ptr);         // VLD1.64 {d0}, [r0]
int8x16_t vld1q_dup_s8(__transfersize(1) int8_t const * ptr);         // VLD1.8 {d0[]}, [r0]
int16x8_t vld1q_dup_s16(__transfersize(1) int16_t const * ptr);         // VLD1.16 {d0[]}, [r0]
int32x4_t vld1q_dup_s32(__transfersize(1) int32_t const * ptr);         // VLD1.32 {d0[]}, [r0]
int64x2_t vld1q_dup_s64(__transfersize(1) int64_t const * ptr);         // VLD1.64 {d0}, [r0]
float16x8_t vld1q_dup_f16(__transfersize(1) __fp16 const * ptr);         // VLD1.16 {d0[]}, [r0]
float32x4_t vld1q_dup_f32(__transfersize(1) float32_t const * ptr);         // VLD1.32 {d0[]}, [r0]
poly8x16_t vld1q_dup_p8(__transfersize(1) poly8_t const * ptr);         // VLD1.8 {d0[]}, [r0]
poly16x8_t vld1q_dup_p16(__transfersize(1) poly16_t const * ptr);         // VLD1.16 {d0[]}, [r0]

//Store a single vector or lane. Stores all lanes or a single lane of a vector.
//Store a single vector into memory
void vst1q_u8(__transfersize(16) uint8_t * ptr, uint8x16_t val);         // VST1.8 {d0, d1}, [r0]
void vst1q_u16(__transfersize(8) uint16_t * ptr, uint16x8_t val);         // VST1.16 {d0, d1}, [r0]
void vst1q_u32(__transfersize(4) uint32_t * ptr, uint32x4_t val);         // VST1.32 {d0, d1}, [r0]
void vst1q_u64(__transfersize(2) uint64_t * ptr, uint64x2_t val);         // VST1.64 {d0, d1}, [r0]
void vst1q_s8(__transfersize(16) int8_t * ptr, int8x16_t val);         // VST1.8 {d0, d1}, [r0]
void vst1q_s16(__transfersize(8) int16_t * ptr, int16x8_t val);         // VST1.16 {d0, d1}, [r0]
void vst1q_s32(__transfersize(4) int32_t * ptr, int32x4_t val);         // VST1.32 {d0, d1}, [r0]
void vst1q_s64(__transfersize(2) int64_t * ptr, int64x2_t val);         // VST1.64 {d0, d1}, [r0]
void vst1q_f16(__transfersize(8) __fp16 * ptr, float16x8_t val);         // VST1.16 {d0, d1}, [r0]
void vst1q_f32(__transfersize(4) float32_t * ptr, float32x4_t val);         // VST1.32 {d0, d1}, [r0]
void vst1q_p8(__transfersize(16) poly8_t * ptr, poly8x16_t val);         // VST1.8 {d0, d1}, [r0]
void vst1q_p16(__transfersize(8) poly16_t * ptr, poly16x8_t val);         // VST1.16 {d0, d1}, [r0]

//Store a lane of a vector into memory
//Loads of an N-element structure
//Load N-element structure from memory
uint8x16x2_t vld2q_u8(__transfersize(32) uint8_t const * ptr);         // VLD2.8 {d0, d2}, [r0]
uint16x8x2_t vld2q_u16(__transfersize(16) uint16_t const * ptr);         // VLD2.16 {d0, d2}, [r0]
uint32x4x2_t vld2q_u32(__transfersize(8) uint32_t const * ptr);         // VLD2.32 {d0, d2}, [r0]
int8x16x2_t vld2q_s8(__transfersize(32) int8_t const * ptr);         // VLD2.8 {d0, d2}, [r0]
int16x8x2_t vld2q_s16(__transfersize(16) int16_t const * ptr);         // VLD2.16 {d0, d2}, [r0]
int32x4x2_t vld2q_s32(__transfersize(8) int32_t const * ptr);         // VLD2.32 {d0, d2}, [r0]
float16x8x2_t vld2q_f16(__transfersize(16) __fp16 const * ptr);         // VLD2.16 {d0, d2}, [r0]
float32x4x2_t vld2q_f32(__transfersize(8) float32_t const * ptr);         // VLD2.32 {d0, d2}, [r0]
poly8x16x2_t vld2q_p8(__transfersize(32) poly8_t const * ptr);         // VLD2.8 {d0, d2}, [r0]
poly16x8x2_t vld2q_p16(__transfersize(16) poly16_t const * ptr);         // VLD2.16 {d0, d2}, [r0]
uint8x8x2_t vld2_u8(__transfersize(16) uint8_t const * ptr);         // VLD2.8 {d0, d1}, [r0]
uint16x4x2_t vld2_u16(__transfersize(8) uint16_t const * ptr);         // VLD2.16 {d0, d1}, [r0]
uint32x2x2_t vld2_u32(__transfersize(4) uint32_t const * ptr);         // VLD2.32 {d0, d1}, [r0]
uint64x1x2_t vld2_u64(__transfersize(2) uint64_t const * ptr);         // VLD1.64 {d0, d1}, [r0]
int8x8x2_t vld2_s8(__transfersize(16) int8_t const * ptr);         // VLD2.8 {d0, d1}, [r0]
int16x4x2_t vld2_s16(__transfersize(8) int16_t const * ptr);         // VLD2.16 {d0, d1}, [r0]
int32x2x2_t vld2_s32(__transfersize(4) int32_t const * ptr);         // VLD2.32 {d0, d1}, [r0]
int64x1x2_t vld2_s64(__transfersize(2) int64_t const * ptr);         // VLD1.64 {d0, d1}, [r0]
//float16x4x2_t vld2_f16(__transfersize(8) __fp16 const * ptr); // VLD2.16 {d0, d1}, [r0]
float32x2x2_t vld2_f32(__transfersize(4) float32_t const * ptr);         // VLD2.32 {d0, d1}, [r0]
poly8x8x2_t vld2_p8(__transfersize(16) poly8_t const * ptr);         // VLD2.8 {d0, d1}, [r0]
poly16x4x2_t vld2_p16(__transfersize(8) poly16_t const * ptr);         // VLD2.16 {d0, d1}, [r0]
uint8x16x3_t vld3q_u8(__transfersize(48) uint8_t const * ptr);         // VLD3.8 {d0, d2, d4}, [r0]
uint16x8x3_t vld3q_u16(__transfersize(24) uint16_t const * ptr);         // VLD3.16 {d0, d2, d4}, [r0]
uint32x4x3_t vld3q_u32(__transfersize(12) uint32_t const * ptr);         // VLD3.32 {d0, d2, d4}, [r0]
int8x16x3_t vld3q_s8(__transfersize(48) int8_t const * ptr);         // VLD3.8 {d0, d2, d4}, [r0]
int16x8x3_t vld3q_s16(__transfersize(24) int16_t const * ptr);         // VLD3.16 {d0, d2, d4}, [r0]
int32x4x3_t vld3q_s32(__transfersize(12) int32_t const * ptr);         // VLD3.32 {d0, d2, d4}, [r0]
float16x8x3_t vld3q_f16(__transfersize(24) __fp16 const * ptr);         // VLD3.16 {d0, d2, d4}, [r0]
float32x4x3_t vld3q_f32(__transfersize(12) float32_t const * ptr);         // VLD3.32 {d0, d2, d4}, [r0]
poly8x16x3_t vld3q_p8(__transfersize(48) poly8_t const * ptr);         // VLD3.8 {d0, d2, d4}, [r0]
poly16x8x3_t vld3q_p16(__transfersize(24) poly16_t const * ptr);         // VLD3.16 {d0, d2, d4}, [r0]
uint8x8x3_t vld3_u8(__transfersize(24) uint8_t const * ptr);         // VLD3.8 {d0, d1, d2}, [r0]
uint16x4x3_t vld3_u16(__transfersize(12) uint16_t const * ptr);         // VLD3.16 {d0, d1, d2}, [r0]
uint32x2x3_t vld3_u32(__transfersize(6) uint32_t const * ptr);         // VLD3.32 {d0, d1, d2}, [r0]
uint64x1x3_t vld3_u64(__transfersize(3) uint64_t const * ptr);         // VLD1.64 {d0, d1, d2}, [r0]
int8x8x3_t vld3_s8(__transfersize(24) int8_t const * ptr);         // VLD3.8 {d0, d1, d2}, [r0]
int16x4x3_t vld3_s16(__transfersize(12) int16_t const * ptr);         // VLD3.16 {d0, d1, d2}, [r0]
int32x2x3_t vld3_s32(__transfersize(6) int32_t const * ptr);         // VLD3.32 {d0, d1, d2}, [r0]
int64x1x3_t vld3_s64(__transfersize(3) int64_t const * ptr);         // VLD1.64 {d0, d1, d2}, [r0]
float16x4x3_t vld3_f16(__transfersize(12) __fp16 const * ptr);         // VLD3.16 {d0, d1, d2}, [r0]
float32x2x3_t vld3_f32(__transfersize(6) float32_t const * ptr);         // VLD3.32 {d0, d1, d2}, [r0]
poly8x8x3_t vld3_p8(__transfersize(24) poly8_t const * ptr);         // VLD3.8 {d0, d1, d2}, [r0]
poly16x4x3_t vld3_p16(__transfersize(12) poly16_t const * ptr);         // VLD3.16 {d0, d1, d2}, [r0]
uint8x16x4_t vld4q_u8(__transfersize(64) uint8_t const * ptr);         // VLD4.8 {d0, d2, d4, d6}, [r0]
uint16x8x4_t vld4q_u16(__transfersize(32) uint16_t const * ptr);         // VLD4.16 {d0, d2, d4, d6}, [r0]
uint32x4x4_t vld4q_u32(__transfersize(16) uint32_t const * ptr);         // VLD4.32 {d0, d2, d4, d6}, [r0]
int8x16x4_t vld4q_s8(__transfersize(64) int8_t const * ptr);         // VLD4.8 {d0, d2, d4, d6}, [r0]
int16x8x4_t vld4q_s16(__transfersize(32) int16_t const * ptr);         // VLD4.16 {d0, d2, d4, d6}, [r0]
int32x4x4_t vld4q_s32(__transfersize(16) int32_t const * ptr);         // VLD4.32 {d0, d2, d4, d6}, [r0]
float16x8x4_t vld4q_f16(__transfersize(32) __fp16 const * ptr);         // VLD4.16 {d0, d2, d4, d6}, [r0]
float32x4x4_t vld4q_f32(__transfersize(16) float32_t const * ptr);         // VLD4.32 {d0, d2, d4, d6}, [r0]
poly8x16x4_t vld4q_p8(__transfersize(64) poly8_t const * ptr);         // VLD4.8 {d0, d2, d4, d6}, [r0]
poly16x8x4_t vld4q_p16(__transfersize(32) poly16_t const * ptr);         // VLD4.16 {d0, d2, d4, d6}, [r0]
uint8x8x4_t vld4_u8(__transfersize(32) uint8_t const * ptr);         // VLD4.8 {d0, d1, d2, d3}, [r0]
uint16x4x4_t vld4_u16(__transfersize(16) uint16_t const * ptr);         // VLD4.16 {d0, d1, d2, d3}, [r0]
uint32x2x4_t vld4_u32(__transfersize(8) uint32_t const * ptr);         // VLD4.32 {d0, d1, d2, d3}, [r0]
uint64x1x4_t vld4_u64(__transfersize(4) uint64_t const * ptr);         // VLD1.64 {d0, d1, d2, d3}, [r0]
int8x8x4_t vld4_s8(__transfersize(32) int8_t const * ptr);         // VLD4.8 {d0, d1, d2, d3}, [r0]
int16x4x4_t vld4_s16(__transfersize(16) int16_t const * ptr);         // VLD4.16 {d0, d1, d2, d3}, [r0]
int32x2x4_t vld4_s32(__transfersize(8) int32_t const * ptr);         // VLD4.32 {d0, d1, d2, d3}, [r0]
int64x1x4_t vld4_s64(__transfersize(4) int64_t const * ptr);         // VLD1.64 {d0, d1, d2, d3}, [r0]
float16x4x4_t vld4_f16(__transfersize(16) __fp16 const * ptr);         // VLD4.16 {d0, d1, d2, d3}, [r0]
float32x2x4_t vld4_f32(__transfersize(8) float32_t const * ptr);         // VLD4.32 {d0, d1, d2, d3}, [r0]
poly8x8x4_t vld4_p8(__transfersize(32) poly8_t const * ptr);         // VLD4.8 {d0, d1, d2, d3}, [r0]
poly16x4x4_t vld4_p16(__transfersize(16) poly16_t const * ptr);         // VLD4.16 {d0, d1, d2, d3}, [r0]
//Load all lanes of N-element structure with same value from memory
uint8x8x2_t vld2_dup_u8(__transfersize(2) uint8_t const * ptr);         // VLD2.8 {d0[], d1[]}, [r0]
uint16x4x2_t vld2_dup_u16(__transfersize(2) uint16_t const * ptr);         // VLD2.16 {d0[], d1[]}, [r0]
uint32x2x2_t vld2_dup_u32(__transfersize(2) uint32_t const * ptr);         // VLD2.32 {d0[], d1[]}, [r0]
uint64x1x2_t vld2_dup_u64(__transfersize(2) uint64_t const * ptr);         // VLD1.64 {d0, d1}, [r0]
int8x8x2_t vld2_dup_s8(__transfersize(2) int8_t const * ptr);         // VLD2.8 {d0[], d1[]}, [r0]
int16x4x2_t vld2_dup_s16(__transfersize(2) int16_t const * ptr);         // VLD2.16 {d0[], d1[]}, [r0]
int32x2x2_t vld2_dup_s32(__transfersize(2) int32_t const * ptr);         // VLD2.32 {d0[], d1[]}, [r0]
int64x1x2_t vld2_dup_s64(__transfersize(2) int64_t const * ptr);         // VLD1.64 {d0, d1}, [r0]
//float16x4x2_t vld2_dup_f16(__transfersize(2) __fp16 const * ptr); // VLD2.16 {d0[], d1[]}, [r0]
float32x2x2_t vld2_dup_f32(__transfersize(2) float32_t const * ptr);         // VLD2.32 {d0[], d1[]}, [r0]
poly8x8x2_t vld2_dup_p8(__transfersize(2) poly8_t const * ptr);         // VLD2.8 {d0[], d1[]}, [r0]
poly16x4x2_t vld2_dup_p16(__transfersize(2) poly16_t const * ptr);         // VLD2.16 {d0[], d1[]}, [r0]
uint8x8x3_t vld3_dup_u8(__transfersize(3) uint8_t const * ptr);         // VLD3.8 {d0[], d1[], d2[]}, [r0]
uint16x4x3_t vld3_dup_u16(__transfersize(3) uint16_t const * ptr);         // VLD3.16 {d0[], d1[], d2[]}, [r0]
uint32x2x3_t vld3_dup_u32(__transfersize(3) uint32_t const * ptr);         // VLD3.32 {d0[], d1[], d2[]}, [r0]
uint64x1x3_t vld3_dup_u64(__transfersize(3) uint64_t const * ptr);         // VLD1.64 {d0, d1, d2}, [r0]
int8x8x3_t vld3_dup_s8(__transfersize(3) int8_t const * ptr);         // VLD3.8 {d0[], d1[], d2[]}, [r0]
int16x4x3_t vld3_dup_s16(__transfersize(3) int16_t const * ptr);         // VLD3.16 {d0[], d1[], d2[]}, [r0]
int32x2x3_t vld3_dup_s32(__transfersize(3) int32_t const * ptr);         // VLD3.32 {d0[], d1[], d2[]}, [r0]
int64x1x3_t vld3_dup_s64(__transfersize(3) int64_t const * ptr);         // VLD1.64 {d0, d1, d2}, [r0]
float16x4x3_t vld3_dup_f16(__transfersize(3) __fp16 const * ptr);         // VLD3.16 {d0[], d1[], d2[]}, [r0]
float32x2x3_t vld3_dup_f32(__transfersize(3) float32_t const * ptr);         // VLD3.32 {d0[], d1[], d2[]}, [r0]
poly8x8x3_t vld3_dup_p8(__transfersize(3) poly8_t const * ptr);         // VLD3.8 {d0[], d1[], d2[]}, [r0]
poly16x4x3_t vld3_dup_p16(__transfersize(3) poly16_t const * ptr);         // VLD3.16 {d0[], d1[], d2[]}, [r0]
uint8x8x4_t vld4_dup_u8(__transfersize(4) uint8_t const * ptr);         // VLD4.8 {d0[], d1[], d2[], d3[]}, [r0]
uint16x4x4_t vld4_dup_u16(__transfersize(4) uint16_t const * ptr);         // VLD4.16 {d0[], d1[], d2[], d3[]}, [r0]
uint32x2x4_t vld4_dup_u32(__transfersize(4) uint32_t const * ptr);         // VLD4.32 {d0[], d1[], d2[], d3[]}, [r0]
uint64x1x4_t vld4_dup_u64(__transfersize(4) uint64_t const * ptr);         // VLD1.64 {d0, d1, d2, d3}, [r0]
int8x8x4_t vld4_dup_s8(__transfersize(4) int8_t const * ptr);         // VLD4.8 {d0[], d1[], d2[], d3[]}, [r0]
int16x4x4_t vld4_dup_s16(__transfersize(4) int16_t const * ptr);         // VLD4.16 {d0[], d1[], d2[], d3[]}, [r0]
int32x2x4_t vld4_dup_s32(__transfersize(4) int32_t const * ptr);         // VLD4.32 {d0[], d1[], d2[], d3[]}, [r0]
int64x1x4_t vld4_dup_s64(__transfersize(4) int64_t const * ptr);         // VLD1.64 {d0, d1, d2, d3}, [r0]
float16x4x4_t vld4_dup_f16(__transfersize(4) __fp16 const * ptr);         // VLD4.16 {d0[], d1[], d2[], d3[]}, [r0]
float32x2x4_t vld4_dup_f32(__transfersize(4) float32_t const * ptr);         // VLD4.32 {d0[], d1[], d2[], d3[]}, [r0]
poly8x8x4_t vld4_dup_p8(__transfersize(4) poly8_t const * ptr);         // VLD4.8 {d0[], d1[], d2[], d3[]}, [r0]
poly16x4x4_t vld4_dup_p16(__transfersize(4) poly16_t const * ptr);         // VLD4.16 {d0[], d1[], d2[], d3[]}, [r0]
//Load a single lane of N-element structure from memory
//the functions below are modified to deal with the error C2719: 'src': formal parameter with __declspec(align('16')) won't be aligned
uint16x8x2_t vld2q_lane_u16_ptr(__transfersize(2) uint16_t const * ptr, uint16x8x2_t * src, __constrange(0,7) int lane);         // VLD2.16 {d0[0], d2[0]}, [r0]
uint32x4x2_t vld2q_lane_u32_ptr(__transfersize(2) uint32_t const * ptr, uint32x4x2_t * src, __constrange(0,3) int lane);         // VLD2.32 {d0[0], d2[0]}, [r0]
int16x8x2_t vld2q_lane_s16_ptr(__transfersize(2) int16_t const * ptr, int16x8x2_t * src, __constrange(0,7) int lane);         // VLD2.16 {d0[0], d2[0]}, [r0]
int32x4x2_t vld2q_lane_s32_ptr(__transfersize(2) int32_t const * ptr, int32x4x2_t * src, __constrange(0,3) int lane);         // VLD2.32 {d0[0], d2[0]}, [r0]
float16x8x2_t vld2q_lane_f16_ptr(__transfersize(2) __fp16 const * ptr, float16x8x2_t * src, __constrange(0,7) int lane);         // VLD2.16 {d0[0], d2[0]}, [r0]
float32x4x2_t vld2q_lane_f32_ptr(__transfersize(2) float32_t const * ptr, float32x4x2_t * src, __constrange(0,3) int lane);         // VLD2.32 {d0[0], d2[0]}, [r0]
poly16x8x2_t vld2q_lane_p16_ptr(__transfersize(2) poly16_t const * ptr, poly16x8x2_t * src, __constrange(0,7) int lane);         // VLD2.16 {d0[0], d2[0]}, [r0]
uint8x8x2_t vld2_lane_u8_ptr(__transfersize(2) uint8_t const * ptr, uint8x8x2_t * src, __constrange(0,7) int lane);         //VLD2.8 {d0[0], d1[0]}, [r0]
uint16x4x2_t vld2_lane_u16_ptr(__transfersize(2) uint16_t const * ptr, uint16x4x2_t * src, __constrange(0,3) int lane);         // VLD2.16 {d0[0], d1[0]}, [r0]
uint32x2x2_t vld2_lane_u32_ptr(__transfersize(2) uint32_t const * ptr, uint32x2x2_t * src, __constrange(0,1) int lane);         // VLD2.32 {d0[0], d1[0]}, [r0]
int8x8x2_t vld2_lane_s8_ptr(__transfersize(2) int8_t const * ptr, int8x8x2_t * src, __constrange(0,7) int lane);         //VLD2.8 {d0[0], d1[0]}, [r0]
int16x4x2_t vld2_lane_s16_ptr(__transfersize(2) int16_t const * ptr, int16x4x2_t * src, __constrange(0,3) int lane);         //VLD2.16 {d0[0], d1[0]}, [r0]
int32x2x2_t vld2_lane_s32_ptr(__transfersize(2) int32_t const * ptr, int32x2x2_t * src, __constrange(0,1) int lane);         //VLD2.32 {d0[0], d1[0]}, [r0]
//float16x4x2_t vld2_lane_f16_ptr(__transfersize(2) __fp16 const * ptr, float16x4x2_t * src, __constrange(0,3) int lane); // VLD2.16 {d0[0], d1[0]}, [r0]
float32x2x2_t vld2_lane_f32_ptr(__transfersize(2) float32_t const * ptr, float32x2x2_t * src, __constrange(0,1) int lane);         // VLD2.32 {d0[0], d1[0]}, [r0]
poly8x8x2_t vld2_lane_p8_ptr(__transfersize(2) poly8_t const * ptr, poly8x8x2_t * src, __constrange(0,7) int lane);         //VLD2.8 {d0[0], d1[0]}, [r0]
poly16x4x2_t vld2_lane_p16_ptr(__transfersize(2) poly16_t const * ptr, poly16x4x2_t * src, __constrange(0,3) int lane);         // VLD2.16 {d0[0], d1[0]}, [r0]
uint16x8x3_t vld3q_lane_u16_ptr(__transfersize(3) uint16_t const * ptr, uint16x8x3_t * src, __constrange(0,7) int lane);         // VLD3.16 {d0[0], d2[0], d4[0]}, [r0]
uint32x4x3_t vld3q_lane_u32_ptr(__transfersize(3) uint32_t const * ptr, uint32x4x3_t * src, __constrange(0,3) int lane);         // VLD3.32 {d0[0], d2[0], d4[0]}, [r0]
int16x8x3_t vld3q_lane_s16_ptr(__transfersize(3) int16_t const * ptr, int16x8x3_t * src, __constrange(0,7) int lane);         // VLD3.16 {d0[0], d2[0], d4[0]}, [r0]
int32x4x3_t vld3q_lane_s32_ptr(__transfersize(3) int32_t const * ptr, int32x4x3_t * src, __constrange(0,3) int lane);         // VLD3.32 {d0[0], d2[0], d4[0]}, [r0]
float16x8x3_t vld3q_lane_f16_ptr(__transfersize(3) __fp16 const * ptr, float16x8x3_t * src, __constrange(0,7) int lane);         // VLD3.16 {d0[0], d2[0], d4[0]}, [r0]
float32x4x3_t vld3q_lane_f32_ptr(__transfersize(3) float32_t const * ptr, float32x4x3_t * src, __constrange(0,3) int lane);         // VLD3.32 {d0[0], d2[0], d4[0]}, [r0]
poly16x8x3_t vld3q_lane_p16_ptr(__transfersize(3) poly16_t const * ptr, poly16x8x3_t * src, __constrange(0,7) int lane);         // VLD3.16 {d0[0], d2[0], d4[0]}, [r0]
uint8x8x3_t vld3_lane_u8_ptr(__transfersize(3) uint8_t const * ptr, uint8x8x3_t * src, __constrange(0,7) int lane);         //VLD3.8 {d0[0], d1[0], d2[0]}, [r0]
uint16x4x3_t vld3_lane_u16_ptr(__transfersize(3) uint16_t const * ptr, uint16x4x3_t * src, __constrange(0,3) int lane);         // VLD3.16 {d0[0], d1[0], d2[0]}, [r0]
uint32x2x3_t vld3_lane_u32_ptr(__transfersize(3) uint32_t const * ptr, uint32x2x3_t * src, __constrange(0,1) int lane);         // VLD3.32 {d0[0], d1[0], d2[0]}, [r0]
int8x8x3_t vld3_lane_s8_ptr(__transfersize(3) int8_t const * ptr, int8x8x3_t * src, __constrange(0,7) int lane);         //VLD3.8 {d0[0], d1[0], d2[0]}, [r0]
int16x4x3_t vld3_lane_s16_ptr(__transfersize(3) int16_t const * ptr, int16x4x3_t * src, __constrange(0,3) int lane);         //VLD3.16 {d0[0], d1[0], d2[0]}, [r0]
int32x2x3_t vld3_lane_s32_ptr(__transfersize(3) int32_t const * ptr, int32x2x3_t * src, __constrange(0,1) int lane);         //VLD3.32 {d0[0], d1[0], d2[0]}, [r0]
float16x4x3_t vld3_lane_f16_ptr(__transfersize(3) __fp16 const * ptr, float16x4x3_t * src, __constrange(0,3) int lane);         // VLD3.16 {d0[0], d1[0], d2[0]}, [r0]
float32x2x3_t vld3_lane_f32_ptr(__transfersize(3) float32_t const * ptr, float32x2x3_t * src, __constrange(0,1) int lane);         // VLD3.32 {d0[0], d1[0], d2[0]}, [r0]
poly8x8x3_t vld3_lane_p8_ptr(__transfersize(3) poly8_t const * ptr, poly8x8x3_t * src, __constrange(0,7) int lane);         //VLD3.8 {d0[0], d1[0], d2[0]}, [r0]
poly16x4x3_t vld3_lane_p16_ptr(__transfersize(3) poly16_t const * ptr, poly16x4x3_t * src, __constrange(0,3) int lane);         // VLD3.16 {d0[0], d1[0], d2[0]}, [r0]
uint16x8x4_t vld4q_lane_u16_ptr(__transfersize(4) uint16_t const * ptr, uint16x8x4_t * src, __constrange(0,7) int lane);         // VLD4.16 {d0[0], d2[0], d4[0], d6[0]}, [r0]
uint32x4x4_t vld4q_lane_u32_ptr(__transfersize(4) uint32_t const * ptr, uint32x4x4_t * src, __constrange(0,3) int lane);         // VLD4.32 {d0[0], d2[0], d4[0], d6[0]}, [r0]
int16x8x4_t vld4q_lane_s16_ptr(__transfersize(4) int16_t const * ptr, int16x8x4_t * src, __constrange(0,7) int lane);         // VLD4.16 {d0[0], d2[0], d4[0], d6[0]}, [r0]
int32x4x4_t vld4q_lane_s32_ptr(__transfersize(4) int32_t const * ptr, int32x4x4_t * src, __constrange(0,3) int lane);         // VLD4.32 {d0[0], d2[0], d4[0], d6[0]}, [r0]
float16x8x4_t vld4q_lane_f16_ptr(__transfersize(4) __fp16 const * ptr, float16x8x4_t * src, __constrange(0,7) int lane);         // VLD4.16 {d0[0], d2[0], d4[0], d6[0]}, [r0]
float32x4x4_t vld4q_lane_f32_ptr(__transfersize(4) float32_t const * ptr, float32x4x4_t * src, __constrange(0,3) int lane);         // VLD4.32 {d0[0], d2[0], d4[0], d6[0]}, [r0]
poly16x8x4_t vld4q_lane_p16_ptr(__transfersize(4) poly16_t const * ptr, poly16x8x4_t * src, __constrange(0,7) int lane);         // VLD4.16 {d0[0], d2[0], d4[0], d6[0]}, [r0]
uint8x8x4_t vld4_lane_u8_ptr(__transfersize(4) uint8_t const * ptr, uint8x8x4_t * src, __constrange(0,7) int lane);         //VLD4.8 {d0[0], d1[0], d2[0], d3[0]}, [r0]
uint16x4x4_t vld4_lane_u16_ptr(__transfersize(4) uint16_t const * ptr, uint16x4x4_t * src, __constrange(0,3) int lane);         // VLD4.16 {d0[0], d1[0], d2[0], d3[0]}, [r0]
uint32x2x4_t vld4_lane_u32_ptr(__transfersize(4) uint32_t const * ptr, uint32x2x4_t * src, __constrange(0,1) int lane);         // VLD4.32 {d0[0], d1[0], d2[0], d3[0]}, [r0]
int8x8x4_t vld4_lane_s8_ptr(__transfersize(4) int8_t const * ptr, int8x8x4_t * src, __constrange(0,7) int lane);         //VLD4.8 {d0[0], d1[0], d2[0], d3[0]}, [r0]
int16x4x4_t vld4_lane_s16_ptr(__transfersize(4) int16_t const * ptr, int16x4x4_t * src, __constrange(0,3) int lane);         //VLD4.16 {d0[0], d1[0], d2[0], d3[0]}, [r0]
int32x2x4_t vld4_lane_s32_ptr(__transfersize(4) int32_t const * ptr, int32x2x4_t * src, __constrange(0,1) int lane);         //VLD4.32 {d0[0], d1[0], d2[0], d3[0]}, [r0]
float16x4x4_t vld4_lane_f16_ptr(__transfersize(4) __fp16 const * ptr, float16x4x4_t * src, __constrange(0,3) int lane);         // VLD4.16 {d0[0], d1[0], d2[0], d3[0]}, [r0]
float32x2x4_t vld4_lane_f32_ptr(__transfersize(4) float32_t const * ptr, float32x2x4_t * src, __constrange(0,1) int lane);         // VLD4.32 {d0[0], d1[0], d2[0], d3[0]}, [r0]
poly8x8x4_t vld4_lane_p8_ptr(__transfersize(4) poly8_t const * ptr, poly8x8x4_t * src, __constrange(0,7) int lane);         //VLD4.8 {d0[0], d1[0], d2[0], d3[0]}, [r0]
poly16x4x4_t vld4_lane_p16_ptr(__transfersize(4) poly16_t const * ptr, poly16x4x4_t * src, __constrange(0,3) int lane);         // VLD4.16 {d0[0], d1[0], d2[0], d3[0]}, [r0]
//Store N-element structure to memory
void vst2q_u8_ptr(__transfersize(32) uint8_t * ptr, uint8x16x2_t * val);         // VST2.8 {d0, d2}, [r0]
void vst2q_u16_ptr(__transfersize(16) uint16_t * ptr, uint16x8x2_t * val);         // VST2.16 {d0, d2}, [r0]
void vst2q_u32_ptr(__transfersize(8) uint32_t * ptr, uint32x4x2_t * val);         // VST2.32 {d0, d2}, [r0]
void vst2q_s8_ptr(__transfersize(32) int8_t * ptr, int8x16x2_t * val);         // VST2.8 {d0, d2}, [r0]
void vst2q_s16_ptr(__transfersize(16) int16_t * ptr, int16x8x2_t * val);         // VST2.16 {d0, d2}, [r0]
void vst2q_s32_ptr(__transfersize(8) int32_t * ptr, int32x4x2_t * val);         // VST2.32 {d0, d2}, [r0]
void vst2q_f16_ptr(__transfersize(16) __fp16 * ptr, float16x8x2_t * val);         // VST2.16 {d0, d2}, [r0]
void vst2q_f32_ptr(__transfersize(8) float32_t * ptr, float32x4x2_t * val);         // VST2.32 {d0, d2}, [r0]
void vst2q_p8_ptr(__transfersize(32) poly8_t * ptr, poly8x16x2_t * val);         // VST2.8 {d0, d2}, [r0]
void vst2q_p16_ptr(__transfersize(16) poly16_t * ptr, poly16x8x2_t * val);         // VST2.16 {d0, d2}, [r0]
void vst2_u8_ptr(__transfersize(16) uint8_t * ptr, uint8x8x2_t * val);         // VST2.8 {d0, d1}, [r0]
void vst2_u16_ptr(__transfersize(8) uint16_t * ptr, uint16x4x2_t * val);         // VST2.16 {d0, d1}, [r0]
void vst2_u32_ptr(__transfersize(4) uint32_t * ptr, uint32x2x2_t * val);         // VST2.32 {d0, d1}, [r0]
void vst2_u64_ptr(__transfersize(2) uint64_t * ptr, uint64x1x2_t * val);         // VST1.64 {d0, d1}, [r0]
void vst2_s8_ptr(__transfersize(16) int8_t * ptr, int8x8x2_t * val);         // VST2.8 {d0, d1}, [r0]
void vst2_s16_ptr(__transfersize(8) int16_t * ptr, int16x4x2_t * val);         // VST2.16 {d0, d1}, [r0]
void vst2_s32_ptr(__transfersize(4) int32_t * ptr, int32x2x2_t * val);         // VST2.32 {d0, d1}, [r0]
void vst2_s64_ptr(__transfersize(2) int64_t * ptr, int64x1x2_t * val);         // VST1.64 {d0, d1}, [r0]
//void vst2_f16_ptr(__transfersize(8) __fp16 * ptr, float16x4x2_t * val); // VST2.16 {d0, d1}, [r0]
void vst2_f32_ptr(__transfersize(4) float32_t * ptr, float32x2x2_t * val);         // VST2.32 {d0, d1}, [r0]
void vst2_p8_ptr(__transfersize(16) poly8_t * ptr, poly8x8x2_t * val);         // VST2.8 {d0, d1}, [r0]
void vst2_p16_ptr(__transfersize(8) poly16_t * ptr, poly16x4x2_t * val);         // VST2.16 {d0, d1}, [r0]
void vst3q_u8_ptr(__transfersize(48) uint8_t * ptr, uint8x16x3_t * val);         // VST3.8 {d0, d2, d4}, [r0]
void vst3q_u16_ptr(__transfersize(24) uint16_t * ptr, uint16x8x3_t * val);         // VST3.16 {d0, d2, d4}, [r0]
void vst3q_u32_ptr(__transfersize(12) uint32_t * ptr, uint32x4x3_t * val);         // VST3.32 {d0, d2, d4}, [r0]
void vst3q_s8_ptr(__transfersize(48) int8_t * ptr, int8x16x3_t * val);         // VST3.8 {d0, d2, d4}, [r0]
void vst3q_s16_ptr(__transfersize(24) int16_t * ptr, int16x8x3_t * val);         // VST3.16 {d0, d2, d4}, [r0]
void vst3q_s32_ptr(__transfersize(12) int32_t * ptr, int32x4x3_t * val);         // VST3.32 {d0, d2, d4}, [r0]
void vst3q_f16_ptr(__transfersize(24) __fp16 * ptr, float16x8x3_t * val);         // VST3.16 {d0, d2, d4}, [r0]
void vst3q_f32_ptr(__transfersize(12) float32_t * ptr, float32x4x3_t * val);         // VST3.32 {d0, d2, d4}, [r0]
void vst3q_p8_ptr(__transfersize(48) poly8_t * ptr, poly8x16x3_t * val);         // VST3.8 {d0, d2, d4}, [r0]
void vst3q_p16_ptr(__transfersize(24) poly16_t * ptr, poly16x8x3_t * val);         // VST3.16 {d0, d2, d4}, [r0]
void vst3_u8_ptr(__transfersize(24) uint8_t * ptr, uint8x8x3_t * val);         // VST3.8 {d0, d1, d2}, [r0]
void vst3_u16_ptr(__transfersize(12) uint16_t * ptr, uint16x4x3_t * val);         // VST3.16 {d0, d1, d2}, [r0]
void vst3_u32_ptr(__transfersize(6) uint32_t * ptr, uint32x2x3_t * val);         // VST3.32 {d0, d1, d2}, [r0]
void vst3_u64_ptr(__transfersize(3) uint64_t * ptr, uint64x1x3_t * val);         // VST1.64 {d0, d1, d2}, [r0]
void vst3_s8_ptr(__transfersize(24) int8_t * ptr, int8x8x3_t * val);         // VST3.8 {d0, d1, d2}, [r0]
void vst3_s16_ptr(__transfersize(12) int16_t * ptr, int16x4x3_t * val);         // VST3.16 {d0, d1, d2}, [r0]
void vst3_s32_ptr(__transfersize(6) int32_t * ptr, int32x2x3_t * val);         // VST3.32 {d0, d1, d2}, [r0]
void vst3_s64_ptr(__transfersize(3) int64_t * ptr, int64x1x3_t * val);         // VST1.64 {d0, d1, d2}, [r0]
void vst3_f16_ptr(__transfersize(12) __fp16 * ptr, float16x4x3_t * val);         // VST3.16 {d0, d1, d2}, [r0]
void vst3_f32_ptr(__transfersize(6) float32_t * ptr, float32x2x3_t * val);         // VST3.32 {d0, d1, d2}, [r0]
void vst3_p8_ptr(__transfersize(24) poly8_t * ptr, poly8x8x3_t * val);         // VST3.8 {d0, d1, d2}, [r0]
void vst3_p16_ptr(__transfersize(12) poly16_t * ptr, poly16x4x3_t * val);         // VST3.16 {d0, d1, d2}, [r0]
void vst4q_u8_ptr(__transfersize(64) uint8_t * ptr, uint8x16x4_t * val);         // VST4.8 {d0, d2, d4, d6}, [r0]
void vst4q_u16_ptr(__transfersize(32) uint16_t * ptr, uint16x8x4_t * val);         // VST4.16 {d0, d2, d4, d6}, [r0]
void vst4q_u32_ptr(__transfersize(16) uint32_t * ptr, uint32x4x4_t * val);         // VST4.32 {d0, d2, d4, d6}, [r0]
void vst4q_s8_ptr(__transfersize(64) int8_t * ptr, int8x16x4_t * val);         // VST4.8 {d0, d2, d4, d6}, [r0]
void vst4q_s16_ptr(__transfersize(32) int16_t * ptr, int16x8x4_t * val);         // VST4.16 {d0, d2, d4, d6}, [r0]
void vst4q_s32_ptr(__transfersize(16) int32_t * ptr, int32x4x4_t * val);         // VST4.32 {d0, d2, d4, d6}, [r0]
void vst4q_f16_ptr(__transfersize(32) __fp16 * ptr, float16x8x4_t * val);         // VST4.16 {d0, d2, d4, d6}, [r0]
void vst4q_f32_ptr(__transfersize(16) float32_t * ptr, float32x4x4_t * val);         // VST4.32 {d0, d2, d4, d6}, [r0]
void vst4q_p8_ptr(__transfersize(64) poly8_t * ptr, poly8x16x4_t * val);         // VST4.8 {d0, d2, d4, d6}, [r0]
void vst4q_p16_ptr(__transfersize(32) poly16_t * ptr, poly16x8x4_t * val);         // VST4.16 {d0, d2, d4, d6}, [r0]
void vst4_u8_ptr(__transfersize(32) uint8_t * ptr, uint8x8x4_t * val);         // VST4.8 {d0, d1, d2, d3}, [r0]
void vst4_u16_ptr(__transfersize(16) uint16_t * ptr, uint16x4x4_t * val);         // VST4.16 {d0, d1, d2, d3}, [r0]
void vst4_u32_ptr(__transfersize(8) uint32_t * ptr, uint32x2x4_t * val);         // VST4.32 {d0, d1, d2, d3}, [r0]
void vst4_u64_ptr(__transfersize(4) uint64_t * ptr, uint64x1x4_t * val);         // VST1.64 {d0, d1, d2, d3}, [r0]
void vst4_s8_ptr(__transfersize(32) int8_t * ptr, int8x8x4_t * val);         // VST4.8 {d0, d1, d2, d3}, [r0]
void vst4_s16_ptr(__transfersize(16) int16_t * ptr, int16x4x4_t * val);         // VST4.16 {d0, d1, d2, d3}, [r0]
void vst4_s32_ptr(__transfersize(8) int32_t * ptr, int32x2x4_t * val);         // VST4.32 {d0, d1, d2, d3}, [r0]
void vst4_s64_ptr(__transfersize(4) int64_t * ptr, int64x1x4_t * val);         // VST1.64 {d0, d1, d2, d3}, [r0]
void vst4_f16_ptr(__transfersize(16) __fp16 * ptr, float16x4x4_t * val);         // VST4.16 {d0, d1, d2, d3}, [r0]
void vst4_f32_ptr(__transfersize(8) float32_t * ptr, float32x2x4_t * val);         // VST4.32 {d0, d1, d2, d3}, [r0]
void vst4_p8_ptr(__transfersize(32) poly8_t * ptr, poly8x8x4_t * val);         // VST4.8 {d0, d1, d2, d3}, [r0]
void vst4_p16_ptr(__transfersize(16) poly16_t * ptr, poly16x4x4_t * val);         // VST4.16 {d0, d1, d2, d3}, [r0]
//Store a single lane of N-element structure to memory
void vst2q_lane_u16_ptr(__transfersize(2) uint16_t * ptr, uint16x8x2_t * val, __constrange(0,7) int lane);         // VST2.16{d0[0], d2[0]}, [r0]
void vst2q_lane_u32_ptr(__transfersize(2) uint32_t * ptr, uint32x4x2_t * val, __constrange(0,3) int lane);         // VST2.32{d0[0], d2[0]}, [r0]
void vst2q_lane_s16_ptr(__transfersize(2) int16_t * ptr, int16x8x2_t * val, __constrange(0,7) int lane);         // VST2.16{d0[0], d2[0]}, [r0]
void vst2q_lane_s32_ptr(__transfersize(2) int32_t * ptr, int32x4x2_t * val, __constrange(0,3) int lane);         // VST2.32{d0[0], d2[0]}, [r0]
void vst2q_lane_f16_ptr(__transfersize(2) __fp16 * ptr, float16x8x2_t * val, __constrange(0,7) int lane);         // VST2.16{d0[0], d2[0]}, [r0]
void vst2q_lane_f32_ptr(__transfersize(2) float32_t * ptr, float32x4x2_t * val, __constrange(0,3) int lane);         //VST2.32 {d0[0], d2[0]}, [r0]
void vst2q_lane_p16_ptr(__transfersize(2) poly16_t * ptr, poly16x8x2_t * val, __constrange(0,7) int lane);         // VST2.16{d0[0], d2[0]}, [r0]
void vst2_lane_u8_ptr(__transfersize(2) uint8_t * ptr, uint8x8x2_t * val, __constrange(0,7) int lane);         // VST2.8{d0[0], d1[0]}, [r0]
void vst2_lane_u16_ptr(__transfersize(2) uint16_t * ptr, uint16x4x2_t * val, __constrange(0,3) int lane);         // VST2.16{d0[0], d1[0]}, [r0]
void vst2_lane_u32_ptr(__transfersize(2) uint32_t * ptr, uint32x2x2_t * val, __constrange(0,1) int lane);         // VST2.32{d0[0], d1[0]}, [r0]
void vst2_lane_s8_ptr(__transfersize(2) int8_t * ptr, int8x8x2_t * val, __constrange(0,7) int lane);         // VST2.8 {d0[0],d1[0]}, [r0]
void vst2_lane_s16_ptr(__transfersize(2) int16_t * ptr, int16x4x2_t * val, __constrange(0,3) int lane);         // VST2.16{d0[0], d1[0]}, [r0]
void vst2_lane_s32_ptr(__transfersize(2) int32_t * ptr, int32x2x2_t * val, __constrange(0,1) int lane);         // VST2.32{d0[0], d1[0]}, [r0]
void vst2_lane_f16_ptr(__transfersize(2) __fp16 * ptr, float16x4x2_t * val, __constrange(0,3) int lane);         // VST2.16{d0[0], d1[0]}, [r0]
void vst2_lane_f32_ptr(__transfersize(2) float32_t * ptr, float32x2x2_t * val, __constrange(0,1) int lane);         // VST2.32{d0[0], d1[0]}, [r0]
void vst2_lane_p8_ptr(__transfersize(2) poly8_t * ptr, poly8x8x2_t * val, __constrange(0,7) int lane);         // VST2.8{d0[0], d1[0]}, [r0]
void vst2_lane_p16_ptr(__transfersize(2) poly16_t * ptr, poly16x4x2_t * val, __constrange(0,3) int lane);         // VST2.16{d0[0], d1[0]}, [r0]
void vst3q_lane_u16_ptr(__transfersize(3) uint16_t * ptr, uint16x8x3_t * val, __constrange(0,7) int lane);         // VST3.16{d0[0], d2[0], d4[0]}, [r0]
void vst3q_lane_u32_ptr(__transfersize(3) uint32_t * ptr, uint32x4x3_t * val, __constrange(0,3) int lane);         // VST3.32{d0[0], d2[0], d4[0]}, [r0]
void vst3q_lane_s16_ptr(__transfersize(3) int16_t * ptr, int16x8x3_t * val, __constrange(0,7) int lane);         // VST3.16{d0[0], d2[0], d4[0]}, [r0]
void vst3q_lane_s32_ptr(__transfersize(3) int32_t * ptr, int32x4x3_t * val, __constrange(0,3) int lane);         // VST3.32{d0[0], d2[0], d4[0]}, [r0]
void vst3q_lane_f16_ptr(__transfersize(3) __fp16 * ptr, float16x8x3_t * val, __constrange(0,7) int lane);         // VST3.16{d0[0], d2[0], d4[0]}, [r0]
void vst3q_lane_f32_ptr(__transfersize(3) float32_t * ptr, float32x4x3_t * val, __constrange(0,3) int lane);         //VST3.32 {d0[0], d2[0], d4[0]}, [r0]
void vst3q_lane_p16_ptr(__transfersize(3) poly16_t * ptr, poly16x8x3_t * val, __constrange(0,7) int lane);         // VST3.16{d0[0], d2[0], d4[0]}, [r0]
void vst3_lane_u8_ptr(__transfersize(3) uint8_t * ptr, uint8x8x3_t * val, __constrange(0,7) int lane);         // VST3.8{d0[0], d1[0], d2[0]}, [r0]
void vst3_lane_u16_ptr(__transfersize(3) uint16_t * ptr, uint16x4x3_t * val, __constrange(0,3) int lane);         // VST3.16{d0[0], d1[0], d2[0]}, [r0]
void vst3_lane_u32_ptr(__transfersize(3) uint32_t * ptr, uint32x2x3_t * val, __constrange(0,1) int lane);         // VST3.32{d0[0], d1[0], d2[0]}, [r0]
void vst3_lane_s8_ptr(__transfersize(3) int8_t * ptr, int8x8x3_t * val, __constrange(0,7) int lane);         // VST3.8 {d0[0],d1[0], d2[0]}, [r0]
void vst3_lane_s16_ptr(__transfersize(3) int16_t * ptr, int16x4x3_t * val, __constrange(0,3) int lane);         // VST3.16{d0[0], d1[0], d2[0]}, [r0]
void vst3_lane_s32_ptr(__transfersize(3) int32_t * ptr, int32x2x3_t * val, __constrange(0,1) int lane);         // VST3.32{d0[0], d1[0], d2[0]}, [r0]
void vst3_lane_f16_ptr(__transfersize(3) __fp16 * ptr, float16x4x3_t * val, __constrange(0,3) int lane);         // VST3.16{d0[0], d1[0], d2[0]}, [r0]
void vst3_lane_f32_ptr(__transfersize(3) float32_t * ptr, float32x2x3_t * val, __constrange(0,1) int lane);         // VST3.32{d0[0], d1[0], d2[0]}, [r0]
void vst3_lane_p8_ptr(__transfersize(3) poly8_t * ptr, poly8x8x3_t * val, __constrange(0,7) int lane);         // VST3.8{d0[0], d1[0], d2[0]}, [r0]
void vst3_lane_p16_ptr(__transfersize(3) poly16_t * ptr, poly16x4x3_t * val, __constrange(0,3) int lane);         // VST3.16{d0[0], d1[0], d2[0]}, [r0]
void vst4q_lane_u16_ptr(__transfersize(4) uint16_t * ptr, uint16x8x4_t * val, __constrange(0,7) int lane);         // VST4.16{d0[0], d2[0], d4[0], d6[0]}, [r0]
void vst4q_lane_u32_ptr(__transfersize(4) uint32_t * ptr, uint32x4x4_t * val, __constrange(0,3) int lane);         // VST4.32{d0[0], d2[0], d4[0], d6[0]}, [r0]
void vst4q_lane_s16_ptr(__transfersize(4) int16_t * ptr, int16x8x4_t * val, __constrange(0,7) int lane);         // VST4.16{d0[0], d2[0], d4[0], d6[0]}, [r0]
void vst4q_lane_s32_ptr(__transfersize(4) int32_t * ptr, int32x4x4_t * val, __constrange(0,3) int lane);         // VST4.32{d0[0], d2[0], d4[0], d6[0]}, [r0]
void vst4q_lane_f16_ptr(__transfersize(4) __fp16 * ptr, float16x8x4_t * val, __constrange(0,7) int lane);         // VST4.16{d0[0], d2[0], d4[0], d6[0]}, [r0]
void vst4q_lane_f32_ptr(__transfersize(4) float32_t * ptr, float32x4x4_t * val, __constrange(0,3) int lane);         //VST4.32 {d0[0], d2[0], d4[0], d6[0]}, [r0]
void vst4q_lane_p16_ptr(__transfersize(4) poly16_t * ptr, poly16x8x4_t * val, __constrange(0,7) int lane);         // VST4.16{d0[0], d2[0], d4[0], d6[0]}, [r0]
void vst4_lane_u8_ptr(__transfersize(4) uint8_t * ptr, uint8x8x4_t * val, __constrange(0,7) int lane);         // VST4.8{d0[0], d1[0], d2[0], d3[0]}, [r0]
void vst4_lane_u16_ptr(__transfersize(4) uint16_t * ptr, uint16x4x4_t * val, __constrange(0,3) int lane);         // VST4.16{d0[0], d1[0], d2[0], d3[0]}, [r0]
void vst4_lane_u32_ptr(__transfersize(4) uint32_t * ptr, uint32x2x4_t * val, __constrange(0,1) int lane);         // VST4.32{d0[0], d1[0], d2[0], d3[0]}, [r0]
void vst4_lane_s8_ptr(__transfersize(4) int8_t * ptr, int8x8x4_t * val, __constrange(0,7) int lane);         // VST4.8 {d0[0],d1[0], d2[0], d3[0]}, [r0]
void vst4_lane_s16_ptr(__transfersize(4) int16_t * ptr, int16x4x4_t * val, __constrange(0,3) int lane);         // VST4.16{d0[0], d1[0], d2[0], d3[0]}, [r0]
void vst4_lane_s32_ptr(__transfersize(4) int32_t * ptr, int32x2x4_t * val, __constrange(0,1) int lane);         // VST4.32{d0[0], d1[0], d2[0], d3[0]}, [r0]
void vst4_lane_f16_ptr(__transfersize(4) __fp16 * ptr, float16x4x4_t * val, __constrange(0,3) int lane);         // VST4.16{d0[0], d1[0], d2[0], d3[0]}, [r0]
void vst4_lane_f32_ptr(__transfersize(4) float32_t * ptr, float32x2x4_t * val, __constrange(0,1) int lane);         // VST4.32{d0[0], d1[0], d2[0], d3[0]}, [r0]
void vst4_lane_p8_ptr(__transfersize(4) poly8_t * ptr, poly8x8x4_t * val, __constrange(0,7) int lane);         // VST4.8{d0[0], d1[0], d2[0], d3[0]}, [r0]
void vst4_lane_p16_ptr(__transfersize(4) poly16_t * ptr, poly16x4x4_t * val, __constrange(0,3) int lane);         // VST4.16{d0[0], d1[0], d2[0], d3[0]}, [r0]
//Extract lanes from a vector and put into a register. These intrinsics extract a single lane (element) from a vector.

uint8_t vgetq_lane_u8(uint8x16_t vec, __constrange(0,15) int lane);         // VMOV.U8 r0, d0[0]
uint16_t vgetq_lane_u16(uint16x8_t vec, __constrange(0,7) int lane);         // VMOV.U16 r0, d0[0]
uint32_t vgetq_lane_u32(uint32x4_t vec, __constrange(0,3) int lane);         // VMOV.32 r0, d0[0]
int8_t vgetq_lane_s8(int8x16_t vec, __constrange(0,15) int lane);         // VMOV.S8 r0, d0[0]
int16_t vgetq_lane_s16(int16x8_t vec, __constrange(0,7) int lane);         // VMOV.S16 r0, d0[0]
int32_t vgetq_lane_s32(int32x4_t vec, __constrange(0,3) int lane);         // VMOV.32 r0, d0[0]
poly8_t vgetq_lane_p8(poly8x16_t vec, __constrange(0,15) int lane);         // VMOV.U8 r0, d0[0]
poly16_t vgetq_lane_p16(poly16x8_t vec, __constrange(0,7) int lane);         // VMOV.U16 r0, d0[0]
float32_t vgetq_lane_f32(float32x4_t vec, __constrange(0,3) int lane);         // VMOV.32 r0, d0[0]

int64_t vgetq_lane_s64(int64x2_t vec, __constrange(0,1) int lane);         // VMOV r0,r0,d0
uint64_t vgetq_lane_u64(uint64x2_t vec, __constrange(0,1) int lane);         // VMOV r0,r0,d0
//Load a single lane of a vector from a literal. These intrinsics set a single lane (element) within a vector.

uint8x16_t vsetq_lane_u8(uint8_t value, uint8x16_t vec, __constrange(0,15) int lane);         // VMOV.8 d0[0],r0
uint16x8_t vsetq_lane_u16(uint16_t value, uint16x8_t vec, __constrange(0,7) int lane);         // VMOV.16 d0[0],r0
uint32x4_t vsetq_lane_u32(uint32_t value, uint32x4_t vec, __constrange(0,3) int lane);         // VMOV.32 d0[0],r0
int8x16_t vsetq_lane_s8(int8_t value, int8x16_t vec, __constrange(0,15) int lane);         // VMOV.8 d0[0],r0
int16x8_t vsetq_lane_s16(int16_t value, int16x8_t vec, __constrange(0,7) int lane);         // VMOV.16 d0[0],r0
int32x4_t vsetq_lane_s32(int32_t value, int32x4_t vec, __constrange(0,3) int lane);         // VMOV.32 d0[0],r0
poly8x16_t vsetq_lane_p8(poly8_t value, poly8x16_t vec, __constrange(0,15) int lane);         // VMOV.8 d0[0],r0
poly16x8_t vsetq_lane_p16(poly16_t value, poly16x8_t vec, __constrange(0,7) int lane);         // VMOV.16 d0[0],r0
float32x4_t vsetq_lane_f32(float32_t value, float32x4_t vec, __constrange(0,3) int lane);         // VMOV.32 d0[0],r0

int64x2_t vsetq_lane_s64(int64_t value, int64x2_t vec, __constrange(0,1) int lane);         // VMOV d0,r0,r0
uint64x2_t vsetq_lane_u64(uint64_t value, uint64x2_t vec, __constrange(0,1) int lane);         // VMOV d0,r0,r0
//Initialize a vector from a literal bit pattern.

//Set all lanes to same value
//Load all lanes of vector to the same literal value

uint8x16_t vdupq_n_u8(uint8_t value);         // VDUP.8 q0,r0
uint16x8_t vdupq_n_u16(uint16_t value);         // VDUP.16 q0,r0
uint32x4_t vdupq_n_u32(uint32_t value);         // VDUP.32 q0,r0
int8x16_t vdupq_n_s8(int8_t value);         // VDUP.8 q0,r0
int16x8_t vdupq_n_s16(int16_t value);         // VDUP.16 q0,r0
int32x4_t vdupq_n_s32(int32_t value);         // VDUP.32 q0,r0
poly8x16_t vdupq_n_p8(poly8_t value);         // VDUP.8 q0,r0
poly16x8_t vdupq_n_p16(poly16_t value);         // VDUP.16 q0,r0
float32x4_t vdupq_n_f32(float32_t value);         // VDUP.32 q0,r0

int64x2_t vdupq_n_s64(int64_t value);         // VMOV d0,r0,r0
uint64x2_t vdupq_n_u64(uint64_t value);         // VMOV d0,r0,r0

uint8x16_t vmovq_n_u8(uint8_t value);         // VDUP.8 q0,r0
uint16x8_t vmovq_n_u16(uint16_t value);         // VDUP.16 q0,r0
uint32x4_t vmovq_n_u32(uint32_t value);         // VDUP.32 q0,r0
int8x16_t vmovq_n_s8(int8_t value);         // VDUP.8 q0,r0
int16x8_t vmovq_n_s16(int16_t value);         // VDUP.16 q0,r0
int32x4_t vmovq_n_s32(int32_t value);         // VDUP.32 q0,r0
poly8x16_t vmovq_n_p8(poly8_t value);         // VDUP.8 q0,r0
poly16x8_t vmovq_n_p16(poly16_t value);         // VDUP.16 q0,r0
float32x4_t vmovq_n_f32(float32_t value);         // VDUP.32 q0,r0

int64x2_t vmovq_n_s64(int64_t value);         // VMOV d0,r0,r0
uint64x2_t vmovq_n_u64(uint64_t value);         // VMOV d0,r0,r0
//Load all lanes of the vector to the value of a lane of a vector

//Combining vectors. These intrinsics join two 64 bit vectors into a single 128bit vector.

//Splitting vectors. These intrinsics split a 128 bit vector into 2 component 64 bit vectors

//Converting vectors. These intrinsics are used to convert vectors.
//Convert from float

int32x4_t vcvtq_s32_f32(float32x4_t a);         // VCVT.S32.F32 q0, q0
uint32x4_t vcvtq_u32_f32(float32x4_t a);         // VCVT.U32.F32 q0, q0

int32x4_t vcvtq_n_s32_f32(float32x4_t a, __constrange(1,32) int b);         // VCVT.S32.F32 q0, q0, #32
uint32x4_t vcvtq_n_u32_f32(float32x4_t a, __constrange(1,32) int b);         // VCVT.U32.F32 q0, q0, #32
//Convert to float

float32x4_t vcvtq_f32_s32(int32x4_t a);         // VCVT.F32.S32 q0, q0
float32x4_t vcvtq_f32_u32(uint32x4_t a);         // VCVT.F32.U32 q0, q0

float32x4_t vcvtq_n_f32_s32(int32x4_t a, __constrange(1,32) int b);         // VCVT.F32.S32 q0, q0, #32
float32x4_t vcvtq_n_f32_u32(uint32x4_t a, __constrange(1,32) int b);         // VCVT.F32.U32 q0, q0, #32
//Convert between floats

//Vector narrow integer

//Vector long move

//Vector saturating narrow integer

//Vector saturating narrow integer signed->unsigned

//Table look up

//Extended table look up intrinsics

//Operations with a scalar value
//Vector multiply accumulate with scalar

//Vector widening multiply accumulate with scalar

//Vector widening saturating doubling multiply accumulate with scalar

//Vector multiply subtract with scalar

//Vector widening multiply subtract with scalar

//Vector widening saturating doubling multiply subtract with scalar

//Vector multiply by scalar

int16x8_t vmulq_n_s16(int16x8_t a, int16_t b);         // VMUL.I16 q0,q0,d0[0]
int32x4_t vmulq_n_s32(int32x4_t a, int32_t b);         // VMUL.I32 q0,q0,d0[0]
float32x4_t vmulq_n_f32(float32x4_t a, float32_t b);         // VMUL.F32 q0,q0,d0[0]
uint16x8_t vmulq_n_u16(uint16x8_t a, uint16_t b);         // VMUL.I16 q0,q0,d0[0]
uint32x4_t vmulq_n_u32(uint32x4_t a, uint32_t b);         // VMUL.I32 q0,q0,d0[0]
//Vector long multiply with scalar

//Vector long multiply by scalar

//Vector saturating doubling long multiply with scalar

//Vector saturating doubling long multiply by scalar

//Vector saturating doubling multiply high with scalar

int16x8_t vqdmulhq_n_s16(int16x8_t vec1, int16_t val2);         // VQDMULH.S16 q0,q0,d0[0]
int32x4_t vqdmulhq_n_s32(int32x4_t vec1, int32_t val2);         // VQDMULH.S32 q0,q0,d0[0]
//Vector saturating doubling multiply high by scalar

//Vector saturating rounding doubling multiply high with scalar

int16x8_t vqrdmulhq_n_s16(int16x8_t vec1, int16_t val2);         // VQRDMULH.S16 q0,q0,d0[0]
int32x4_t vqrdmulhq_n_s32(int32x4_t vec1, int32_t val2);         // VQRDMULH.S32 q0,q0,d0[0]
//Vector rounding saturating doubling multiply high by scalar

//Vector multiply accumulate with scalar

int16x8_t vmlaq_n_s16(int16x8_t a, int16x8_t b, int16_t c);         // VMLA.I16 q0, q0, d0[0]
int32x4_t vmlaq_n_s32(int32x4_t a, int32x4_t b, int32_t c);         // VMLA.I32 q0, q0, d0[0]
uint16x8_t vmlaq_n_u16(uint16x8_t a, uint16x8_t b, uint16_t c);         // VMLA.I16 q0, q0, d0[0]
uint32x4_t vmlaq_n_u32(uint32x4_t a, uint32x4_t b, uint32_t c);         // VMLA.I32 q0, q0, d0[0]
float32x4_t vmlaq_n_f32(float32x4_t a, float32x4_t b, float32_t c);         // VMLA.F32 q0, q0, d0[0]
//Vector widening multiply accumulate with scalar

//Vector widening saturating doubling multiply accumulate with scalar

//Vector multiply subtract with scalar

int16x8_t vmlsq_n_s16(int16x8_t a, int16x8_t b, int16_t c);         // VMLS.I16 q0, q0, d0[0]
int32x4_t vmlsq_n_s32(int32x4_t a, int32x4_t b, int32_t c);         // VMLS.I32 q0, q0, d0[0]
uint16x8_t vmlsq_n_u16(uint16x8_t a, uint16x8_t b, uint16_t c);         // VMLS.I16 q0, q0, d0[0]
uint32x4_t vmlsq_n_u32(uint32x4_t a, uint32x4_t b, uint32_t c);         // VMLS.I32 q0, q0, d0[0]
float32x4_t vmlsq_n_f32(float32x4_t a, float32x4_t b, float32_t c);         // VMLS.F32 q0, q0, d0[0]
//Vector widening multiply subtract with scalar

//Vector widening saturating doubling multiply subtract with scalar

//Vector extract

int8x16_t vextq_s8(int8x16_t a, int8x16_t b, __constrange(0,15) int c);         // VEXT.8 q0,q0,q0,#0
uint8x16_t vextq_u8(uint8x16_t a, uint8x16_t b, __constrange(0,15) int c);         // VEXT.8 q0,q0,q0,#0
poly8x16_t vextq_p8(poly8x16_t a, poly8x16_t b, __constrange(0,15) int c);         // VEXT.8 q0,q0,q0,#0
int16x8_t vextq_s16(int16x8_t a, int16x8_t b, __constrange(0,7) int c);         // VEXT.16 q0,q0,q0,#0
uint16x8_t vextq_u16(uint16x8_t a, uint16x8_t b, __constrange(0,7) int c);         // VEXT.16 q0,q0,q0,#0
poly16x8_t vextq_p16(poly16x8_t a, poly16x8_t b, __constrange(0,7) int c);         // VEXT.16 q0,q0,q0,#0
int32x4_t vextq_s32(int32x4_t a, int32x4_t b, __constrange(0,3) int c);         // VEXT.32 q0,q0,q0,#0
uint32x4_t vextq_u32(uint32x4_t a, uint32x4_t b, __constrange(0,3) int c);         // VEXT.32 q0,q0,q0,#0
int64x2_t vextq_s64(int64x2_t a, int64x2_t b, __constrange(0,1) int c);         // VEXT.64 q0,q0,q0,#0
uint64x2_t vextq_u64(uint64x2_t a, uint64x2_t b, __constrange(0,1) int c);         // VEXT.64 q0,q0,q0,#0
//Reverse vector elements (swap endianness). VREVn.m reverses the order of the m-bit lanes within a set that is n bits wide.

int8x16_t vrev64q_s8(int8x16_t vec);         // VREV64.8 q0,q0
int16x8_t vrev64q_s16(int16x8_t vec);         // VREV64.16 q0,q0
int32x4_t vrev64q_s32(int32x4_t vec);         // VREV64.32 q0,q0
uint8x16_t vrev64q_u8(uint8x16_t vec);         // VREV64.8 q0,q0
uint16x8_t vrev64q_u16(uint16x8_t vec);         // VREV64.16 q0,q0
uint32x4_t vrev64q_u32(uint32x4_t vec);         // VREV64.32 q0,q0
poly8x16_t vrev64q_p8(poly8x16_t vec);         // VREV64.8 q0,q0
poly16x8_t vrev64q_p16(poly16x8_t vec);         // VREV64.16 q0,q0
float32x4_t vrev64q_f32(float32x4_t vec);         // VREV64.32 q0,q0

int8x16_t vrev32q_s8(int8x16_t vec);         // VREV32.8 q0,q0
int16x8_t vrev32q_s16(int16x8_t vec);         // VREV32.16 q0,q0
uint8x16_t vrev32q_u8(uint8x16_t vec);         // VREV32.8 q0,q0
uint16x8_t vrev32q_u16(uint16x8_t vec);         // VREV32.16 q0,q0
poly8x16_t vrev32q_p8(poly8x16_t vec);         // VREV32.8 q0,q0

int8x16_t vrev16q_s8(int8x16_t vec);         // VREV16.8 q0,q0
uint8x16_t vrev16q_u8(uint8x16_t vec);         // VREV16.8 q0,q0
poly8x16_t vrev16q_p8(poly8x16_t vec);         // VREV16.8 q0,q0
//Other single operand arithmetic
//Absolute: Vd[i] = |Va[i]|

int8x16_t vabsq_s8(int8x16_t a);         // VABS.S8 q0,q0
int16x8_t vabsq_s16(int16x8_t a);         // VABS.S16 q0,q0
int32x4_t vabsq_s32(int32x4_t a);         // VABS.S32 q0,q0
float32x4_t vabsq_f32(float32x4_t a);         // VABS.F32 q0,q0
//Saturating absolute: Vd[i] = sat(|Va[i]|)

int8x16_t vqabsq_s8(int8x16_t a);         // VQABS.S8 q0,q0
int16x8_t vqabsq_s16(int16x8_t a);         // VQABS.S16 q0,q0
int32x4_t vqabsq_s32(int32x4_t a);         // VQABS.S32 q0,q0
//Negate: Vd[i] = - Va[i]

int8x16_t vnegq_s8(int8x16_t a);         // VNE//q0,q0
int16x8_t vnegq_s16(int16x8_t a);         // VNE//q0,q0
int32x4_t vnegq_s32(int32x4_t a);         // VNE//q0,q0
float32x4_t vnegq_f32(float32x4_t a);         // VNE//q0,q0
//Saturating Negate: sat(Vd[i] = - Va[i])

int8x16_t vqnegq_s8(int8x16_t a);         // VQNE//q0,q0
int16x8_t vqnegq_s16(int16x8_t a);         // VQNE//q0,q0
int32x4_t vqnegq_s32(int32x4_t a);         // VQNE//q0,q0
//Count leading sign bits

int8x16_t vclsq_s8(int8x16_t a);         // VCLS.S8 q0,q0
int16x8_t vclsq_s16(int16x8_t a);         // VCLS.S16 q0,q0
int32x4_t vclsq_s32(int32x4_t a);         // VCLS.S32 q0,q0
//Count leading zeros

int8x16_t vclzq_s8(int8x16_t a);         // VCLZ.I8 q0,q0
int16x8_t vclzq_s16(int16x8_t a);         // VCLZ.I16 q0,q0
int32x4_t vclzq_s32(int32x4_t a);         // VCLZ.I32 q0,q0
uint8x16_t vclzq_u8(uint8x16_t a);         // VCLZ.I8 q0,q0
uint16x8_t vclzq_u16(uint16x8_t a);         // VCLZ.I16 q0,q0
uint32x4_t vclzq_u32(uint32x4_t a);         // VCLZ.I32 q0,q0
//Count number of set bits

uint8x16_t vcntq_u8(uint8x16_t a);         // VCNT.8 q0,q0
int8x16_t vcntq_s8(int8x16_t a);         // VCNT.8 q0,q0
poly8x16_t vcntq_p8(poly8x16_t a);         // VCNT.8 q0,q0
//Reciprocal estimate

float32x4_t vrecpeq_f32(float32x4_t a);         // VRECPE.F32 q0,q0
uint32x4_t vrecpeq_u32(uint32x4_t a);         // VRECPE.U32 q0,q0
//Reciprocal square root estimate

float32x4_t vrsqrteq_f32(float32x4_t a);         // VRSQRTE.F32 q0,q0
uint32x4_t vrsqrteq_u32(uint32x4_t a);         // VRSQRTE.U32 q0,q0
//Logical operations
//Bitwise not

int8x16_t vmvnq_s8(int8x16_t a);         // VMVN q0,q0
int16x8_t vmvnq_s16(int16x8_t a);         // VMVN q0,q0
int32x4_t vmvnq_s32(int32x4_t a);         // VMVN q0,q0
uint8x16_t vmvnq_u8(uint8x16_t a);         // VMVN q0,q0
uint16x8_t vmvnq_u16(uint16x8_t a);         // VMVN q0,q0
uint32x4_t vmvnq_u32(uint32x4_t a);         // VMVN q0,q0
poly8x16_t vmvnq_p8(poly8x16_t a);         // VMVN q0,q0
//Bitwise and

int8x16_t vandq_s8(int8x16_t a, int8x16_t b);         // VAND q0,q0,q0
int16x8_t vandq_s16(int16x8_t a, int16x8_t b);         // VAND q0,q0,q0
int32x4_t vandq_s32(int32x4_t a, int32x4_t b);         // VAND q0,q0,q0
int64x2_t vandq_s64(int64x2_t a, int64x2_t b);         // VAND q0,q0,q0
uint8x16_t vandq_u8(uint8x16_t a, uint8x16_t b);         // VAND q0,q0,q0
uint16x8_t vandq_u16(uint16x8_t a, uint16x8_t b);         // VAND q0,q0,q0
uint32x4_t vandq_u32(uint32x4_t a, uint32x4_t b);         // VAND q0,q0,q0
uint64x2_t vandq_u64(uint64x2_t a, uint64x2_t b);         // VAND q0,q0,q0
//Bitwise or

int8x16_t vorrq_s8(int8x16_t a, int8x16_t b);         // VORR q0,q0,q0
int16x8_t vorrq_s16(int16x8_t a, int16x8_t b);         // VORR q0,q0,q0
int32x4_t vorrq_s32(int32x4_t a, int32x4_t b);         // VORR q0,q0,q0
int64x2_t vorrq_s64(int64x2_t a, int64x2_t b);         // VORR q0,q0,q0
uint8x16_t vorrq_u8(uint8x16_t a, uint8x16_t b);         // VORR q0,q0,q0
uint16x8_t vorrq_u16(uint16x8_t a, uint16x8_t b);         // VORR q0,q0,q0
uint32x4_t vorrq_u32(uint32x4_t a, uint32x4_t b);         // VORR q0,q0,q0
uint64x2_t vorrq_u64(uint64x2_t a, uint64x2_t b);         // VORR q0,q0,q0
//Bitwise exclusive or (EOR or XOR)

int8x16_t veorq_s8(int8x16_t a, int8x16_t b);         // VEOR q0,q0,q0
int16x8_t veorq_s16(int16x8_t a, int16x8_t b);         // VEOR q0,q0,q0
int32x4_t veorq_s32(int32x4_t a, int32x4_t b);         // VEOR q0,q0,q0
int64x2_t veorq_s64(int64x2_t a, int64x2_t b);         // VEOR q0,q0,q0
uint8x16_t veorq_u8(uint8x16_t a, uint8x16_t b);         // VEOR q0,q0,q0
uint16x8_t veorq_u16(uint16x8_t a, uint16x8_t b);         // VEOR q0,q0,q0
uint32x4_t veorq_u32(uint32x4_t a, uint32x4_t b);         // VEOR q0,q0,q0
uint64x2_t veorq_u64(uint64x2_t a, uint64x2_t b);         // VEOR q0,q0,q0
//Bit Clear

int8x16_t vbicq_s8(int8x16_t a, int8x16_t b);         // VBIC q0,q0,q0
int16x8_t vbicq_s16(int16x8_t a, int16x8_t b);         // VBIC q0,q0,q0
int32x4_t vbicq_s32(int32x4_t a, int32x4_t b);         // VBIC q0,q0,q0
int64x2_t vbicq_s64(int64x2_t a, int64x2_t b);         // VBIC q0,q0,q0
uint8x16_t vbicq_u8(uint8x16_t a, uint8x16_t b);         // VBIC q0,q0,q0
uint16x8_t vbicq_u16(uint16x8_t a, uint16x8_t b);         // VBIC q0,q0,q0
uint32x4_t vbicq_u32(uint32x4_t a, uint32x4_t b);         // VBIC q0,q0,q0
uint64x2_t vbicq_u64(uint64x2_t a, uint64x2_t b);         // VBIC q0,q0,q0
//Bitwise OR complement

int8x16_t vornq_s8(int8x16_t a, int8x16_t b);         // VORN q0,q0,q0
int16x8_t vornq_s16(int16x8_t a, int16x8_t b);         // VORN q0,q0,q0
int32x4_t vornq_s32(int32x4_t a, int32x4_t b);         // VORN q0,q0,q0
int64x2_t vornq_s64(int64x2_t a, int64x2_t b);         // VORN q0,q0,q0
uint8x16_t vornq_u8(uint8x16_t a, uint8x16_t b);         // VORN q0,q0,q0
uint16x8_t vornq_u16(uint16x8_t a, uint16x8_t b);         // VORN q0,q0,q0
uint32x4_t vornq_u32(uint32x4_t a, uint32x4_t b);         // VORN q0,q0,q0
uint64x2_t vornq_u64(uint64x2_t a, uint64x2_t b);         // VORN q0,q0,q0
//Bitwise Select

int8x16_t vbslq_s8(uint8x16_t a, int8x16_t b, int8x16_t c);         // VBSL q0,q0,q0
int16x8_t vbslq_s16(uint16x8_t a, int16x8_t b, int16x8_t c);         // VBSL q0,q0,q0
int32x4_t vbslq_s32(uint32x4_t a, int32x4_t b, int32x4_t c);         // VBSL q0,q0,q0
int64x2_t vbslq_s64(uint64x2_t a, int64x2_t b, int64x2_t c);         // VBSL q0,q0,q0
uint8x16_t vbslq_u8(uint8x16_t a, uint8x16_t b, uint8x16_t c);         // VBSL q0,q0,q0
uint16x8_t vbslq_u16(uint16x8_t a, uint16x8_t b, uint16x8_t c);         // VBSL q0,q0,q0
uint32x4_t vbslq_u32(uint32x4_t a, uint32x4_t b, uint32x4_t c);         // VBSL q0,q0,q0
uint64x2_t vbslq_u64(uint64x2_t a, uint64x2_t b, uint64x2_t c);         // VBSL q0,q0,q0
float32x4_t vbslq_f32(uint32x4_t a, float32x4_t b, float32x4_t c);         // VBSL q0,q0,q0
poly8x16_t vbslq_p8(uint8x16_t a, poly8x16_t b, poly8x16_t c);         // VBSL q0,q0,q0
poly16x8_t vbslq_p16(uint16x8_t a, poly16x8_t b, poly16x8_t c);         // VBSL q0,q0,q0
//Transposition operations
//Transpose elements

int8x16x2_t vtrnq_s8(int8x16_t a, int8x16_t b);         // VTRN.8 q0,q0
int16x8x2_t vtrnq_s16(int16x8_t a, int16x8_t b);         // VTRN.16 q0,q0
int32x4x2_t vtrnq_s32(int32x4_t a, int32x4_t b);         // VTRN.32 q0,q0
uint8x16x2_t vtrnq_u8(uint8x16_t a, uint8x16_t b);         // VTRN.8 q0,q0
uint16x8x2_t vtrnq_u16(uint16x8_t a, uint16x8_t b);         // VTRN.16 q0,q0
uint32x4x2_t vtrnq_u32(uint32x4_t a, uint32x4_t b);         // VTRN.32 q0,q0
float32x4x2_t vtrnq_f32(float32x4_t a, float32x4_t b);         // VTRN.32 q0,q0
poly8x16x2_t vtrnq_p8(poly8x16_t a, poly8x16_t b);         // VTRN.8 q0,q0
poly16x8x2_t vtrnq_p16(poly16x8_t a, poly16x8_t b);         // VTRN.16 q0,q0
//Interleave elements

int8x16x2_t vzipq_s8(int8x16_t a, int8x16_t b);         // VZIP.8 q0,q0
int16x8x2_t vzipq_s16(int16x8_t a, int16x8_t b);         // VZIP.16 q0,q0
int32x4x2_t vzipq_s32(int32x4_t a, int32x4_t b);         // VZIP.32 q0,q0
uint8x16x2_t vzipq_u8(uint8x16_t a, uint8x16_t b);         // VZIP.8 q0,q0
uint16x8x2_t vzipq_u16(uint16x8_t a, uint16x8_t b);         // VZIP.16 q0,q0
uint32x4x2_t vzipq_u32(uint32x4_t a, uint32x4_t b);         // VZIP.32 q0,q0
float32x4x2_t vzipq_f32(float32x4_t a, float32x4_t b);         // VZIP.32 q0,q0
poly8x16x2_t vzipq_p8(poly8x16_t a, poly8x16_t b);         // VZIP.8 q0,q0
poly16x8x2_t vzipq_p16(poly16x8_t a, poly16x8_t b);         // VZIP.16 q0,q0
//De-Interleave elements

int8x16x2_t vuzpq_s8(int8x16_t a, int8x16_t b);         // VUZP.8 q0,q0
int16x8x2_t vuzpq_s16(int16x8_t a, int16x8_t b);         // VUZP.16 q0,q0
int32x4x2_t vuzpq_s32(int32x4_t a, int32x4_t b);         // VUZP.32 q0,q0
uint8x16x2_t vuzpq_u8(uint8x16_t a, uint8x16_t b);         // VUZP.8 q0,q0
uint16x8x2_t vuzpq_u16(uint16x8_t a, uint16x8_t b);         // VUZP.16 q0,q0
uint32x4x2_t vuzpq_u32(uint32x4_t a, uint32x4_t b);         // VUZP.32 q0,q0
float32x4x2_t vuzpq_f32(float32x4_t a, float32x4_t b);         // VUZP.32 q0,q0
poly8x16x2_t vuzpq_p8(poly8x16_t a, poly8x16_t b);         // VUZP.8 q0,q0
poly16x8x2_t vuzpq_p16(poly16x8_t a, poly16x8_t b);         // VUZP.16 q0,q0

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
// the following macros solve the problem of the "immediate parameters requirement" for some x86 intrinsics. While for release build it is not a must,
//for debug build we need it to compile the code unless the "Intrinsic parameter must be an immediate value" error is our goal
//
#if ( ((defined _MSC_VER) && (_MSC_VER > 1600)) || defined __INTEL_COMPILER )&& defined NDEBUG     //if it is a release build, we also need it to fix the issue for VS2010 and earlier compilers.

    #if defined(USE_SSSE3)
        #define _MM_ALIGNR_EPI8 _mm_alignr_epi8
    #endif

    #define _MM_EXTRACT_EPI16  _mm_extract_epi16
    #define _MM_INSERT_EPI16 _mm_insert_epi16
    #ifdef USE_SSE4
        #define _MM_EXTRACT_EPI8  _mm_extract_epi8
        #define _MM_EXTRACT_EPI32  _mm_extract_epi32
        #define _MM_EXTRACT_PS  _mm_extract_ps

        #define _MM_INSERT_EPI8  _mm_insert_epi8
        #define _MM_INSERT_EPI32 _mm_insert_epi32
        #define _MM_INSERT_PS    _mm_insert_ps
    #ifdef  _M_X64
            #define _MM_INSERT_EPI64 _mm_insert_epi64
            #define _MM_EXTRACT_EPI64 _mm_extract_epi64
    #endif
    #endif     //SSE4
#else
    #define _NEON2SSE_COMMA ,
    #define _NEON2SSE_SWITCH16(NAME, a, b, LANE) \
            switch(LANE)         \
        {                \
        case 0:     return NAME(a b, 0); \
        case 1:     return NAME(a b, 1); \
        case 2:     return NAME(a b, 2); \
        case 3:     return NAME(a b, 3); \
        case 4:     return NAME(a b, 4); \
        case 5:     return NAME(a b, 5); \
        case 6:     return NAME(a b, 6); \
        case 7:     return NAME(a b, 7); \
        case 8:     return NAME(a b, 8); \
        case 9:     return NAME(a b, 9); \
        case 10:    return NAME(a b, 10); \
        case 11:    return NAME(a b, 11); \
        case 12:    return NAME(a b, 12); \
        case 13:    return NAME(a b, 13); \
        case 14:    return NAME(a b, 14); \
        case 15:    return NAME(a b, 15); \
        default:    return NAME(a b, 0); \
        }

    #define _NEON2SSE_SWITCH8(NAME, vec, LANE, p) \
            switch(LANE)              \
        {                          \
        case 0:  return NAME(vec p,0); \
        case 1:  return NAME(vec p,1); \
        case 2:  return NAME(vec p,2); \
        case 3:  return NAME(vec p,3); \
        case 4:  return NAME(vec p,4); \
        case 5:  return NAME(vec p,5); \
        case 6:  return NAME(vec p,6); \
        case 7:  return NAME(vec p,7); \
        default: return NAME(vec p,0); \
        }

    #define _NEON2SSE_SWITCH4(NAME, case0, case1, case2, case3, vec, LANE, p) \
            switch(LANE)              \
        {                          \
        case case0:  return NAME(vec p,case0); \
        case case1:  return NAME(vec p,case1); \
        case case2:  return NAME(vec p,case2); \
        case case3:  return NAME(vec p,case3); \
        default:     return NAME(vec p,case0); \
        }

    #if defined(USE_SSSE3)
    _NEON2SSE_INLINE __m128i _MM_ALIGNR_EPI8(__m128i a, __m128i b, int LANE)
    {
        _NEON2SSE_SWITCH16(_mm_alignr_epi8, a, _NEON2SSE_COMMA b, LANE)
    }
    #endif

    _NEON2SSE_INLINE __m128i  _MM_INSERT_EPI16(__m128i vec, int p, const int LANE)
    {
        _NEON2SSE_SWITCH8(_mm_insert_epi16, vec, LANE, _NEON2SSE_COMMA p)
    }

    _NEON2SSE_INLINE int _MM_EXTRACT_EPI16(__m128i vec, const int LANE)
    {
        _NEON2SSE_SWITCH8(_mm_extract_epi16, vec, LANE,)
    }

    #ifdef USE_SSE4
        _NEON2SSE_INLINE int _MM_EXTRACT_EPI32(__m128i vec, const int LANE)
        {
            _NEON2SSE_SWITCH4(_mm_extract_epi32, 0,1,2,3, vec, LANE,)
        }

        _NEON2SSE_INLINE int _MM_EXTRACT_PS(__m128 vec, const int LANE)
        {
            _NEON2SSE_SWITCH4(_mm_extract_ps, 0,1,2,3, vec, LANE,)
        }

        _NEON2SSE_INLINE int _MM_EXTRACT_EPI8(__m128i vec, const int LANE)
        {
            _NEON2SSE_SWITCH16(_mm_extract_epi8, vec, , LANE)
        }

        _NEON2SSE_INLINE __m128i  _MM_INSERT_EPI32(__m128i vec, int p, const int LANE)
        {
            _NEON2SSE_SWITCH4(_mm_insert_epi32, 0, 1, 2, 3, vec, LANE, _NEON2SSE_COMMA p)
        }

        _NEON2SSE_INLINE __m128i  _MM_INSERT_EPI8(__m128i vec, int p, const int LANE)
        {
            _NEON2SSE_SWITCH16(_mm_insert_epi8, vec, _NEON2SSE_COMMA p, LANE)
        }
    #ifdef  _M_X64
            _NEON2SSE_INLINE __m128i  _MM_INSERT_EPI64(__m128i vec, int p, const int LANE)
            {
                switch(LANE)
                {
                case 0:
                    return _mm_insert_epi64(vec,  p, 0);
                case 1:
                    return _mm_insert_epi64(vec,  p, 1);
                default:
                    return _mm_insert_epi64(vec,  p, 0);
                }
            }

            _NEON2SSE_INLINE int64_t _MM_EXTRACT_EPI64(__m128i val, const int LANE)
            {
                if (LANE ==0) return _mm_extract_epi64(val, 0);
                else return _mm_extract_epi64(val, 1);
            }
    #endif
        _NEON2SSE_INLINE __m128 _MM_INSERT_PS(__m128 vec, __m128 p, const int LANE)
        {
            _NEON2SSE_SWITCH4(_mm_insert_ps, 0, 16, 32, 48, vec, LANE, _NEON2SSE_COMMA p)
        }

    #endif     //USE_SSE4

#endif     //#ifdef NDEBUG

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Below are some helper functions used either for SSE4 intrinsics "emulation" for SSSE3 limited devices
// or for some specific commonly used operations implementation missing in SSE
#ifdef USE_SSE4
    #define _MM_CVTEPU8_EPI16  _mm_cvtepu8_epi16
    #define _MM_CVTEPU16_EPI32 _mm_cvtepu16_epi32
    #define _MM_CVTEPU32_EPI64  _mm_cvtepu32_epi64

    #define _MM_CVTEPI8_EPI16  _mm_cvtepi8_epi16
    #define _MM_CVTEPI16_EPI32 _mm_cvtepi16_epi32
    #define _MM_CVTEPI32_EPI64  _mm_cvtepi32_epi64

    #define _MM_MAX_EPI8  _mm_max_epi8
    #define _MM_MAX_EPI32 _mm_max_epi32
    #define _MM_MAX_EPU16 _mm_max_epu16
    #define _MM_MAX_EPU32 _mm_max_epu32

    #define _MM_MIN_EPI8  _mm_min_epi8
    #define _MM_MIN_EPI32 _mm_min_epi32
    #define _MM_MIN_EPU16 _mm_min_epu16
    #define _MM_MIN_EPU32 _mm_min_epu32

    #define _MM_BLENDV_EPI8 _mm_blendv_epi8
    #define _MM_PACKUS_EPI32 _mm_packus_epi32
    #define _MM_PACKUS1_EPI32(a) _mm_packus_epi32(a, a)

    #define _MM_MULLO_EPI32 _mm_mullo_epi32
    #define _MM_MUL_EPI32  _mm_mul_epi32
#else     //no SSE4 !!!!!!
    _NEON2SSE_INLINE __m128i _MM_CVTEPU8_EPI16(__m128i a)
    {
        __m128i zero = _mm_setzero_si128();
        return _mm_unpacklo_epi8(a, zero);
    }

    _NEON2SSE_INLINE __m128i _MM_CVTEPU16_EPI32(__m128i a)
    {
        __m128i zero = _mm_setzero_si128();
        return _mm_unpacklo_epi16(a, zero);
    }

    _NEON2SSE_INLINE __m128i _MM_CVTEPU32_EPI64(__m128i a)
    {
        __m128i zero = _mm_setzero_si128();
        return _mm_unpacklo_epi32(a, zero);
    }

    _NEON2SSE_INLINE __m128i _MM_CVTEPI8_EPI16(__m128i a)
    {
        __m128i zero = _mm_setzero_si128();
        __m128i sign = _mm_cmpgt_epi8(zero, a);
        return _mm_unpacklo_epi8(a, sign);
    }

    _NEON2SSE_INLINE __m128i _MM_CVTEPI16_EPI32(__m128i a)
    {
        __m128i zero = _mm_setzero_si128();
        __m128i sign = _mm_cmpgt_epi16(zero, a);
        return _mm_unpacklo_epi16(a, sign);
    }

    _NEON2SSE_INLINE __m128i _MM_CVTEPI32_EPI64(__m128i a)
    {
        __m128i zero = _mm_setzero_si128();
        __m128i sign = _mm_cmpgt_epi32(zero, a);
        return _mm_unpacklo_epi32(a, sign);
    }

    _NEON2SSE_INLINE int _MM_EXTRACT_EPI32(__m128i vec, const int LANE)
    {
        _NEON2SSE_ALIGN_16 int32_t tmp[4];
        _mm_store_si128((__m128i*)tmp, vec);
        return tmp[LANE];
    }

    _NEON2SSE_INLINE int _MM_EXTRACT_EPI8(__m128i vec, const int LANE)
    {
        _NEON2SSE_ALIGN_16 int8_t tmp[16];
        _mm_store_si128((__m128i*)tmp, vec);
        return (int)tmp[LANE];
    }

    _NEON2SSE_INLINE int _MM_EXTRACT_PS(__m128 vec, const int LANE)
    {
        _NEON2SSE_ALIGN_16 int32_t tmp[4];
        _mm_store_si128((__m128i*)tmp, _M128i(vec));
        return tmp[LANE];
    }

    _NEON2SSE_INLINE __m128i  _MM_INSERT_EPI32(__m128i vec, int p, const int LANE)
    {
        _NEON2SSE_ALIGN_16 int32_t pvec[4] = {0,0,0,0};
        _NEON2SSE_ALIGN_16 uint32_t mask[4] = {0xffffffff,0xffffffff,0xffffffff,0xffffffff};
        __m128i vec_masked, p_masked;
        pvec[LANE] = p;
        mask[LANE] = 0x0;
        vec_masked = _mm_and_si128 (*(__m128i*)mask,vec);         //ready for p
        p_masked = _mm_andnot_si128 (*(__m128i*)mask,*(__m128i*)pvec);         //ready for vec
        return _mm_or_si128(vec_masked, p_masked);
    }

    _NEON2SSE_INLINE __m128i  _MM_INSERT_EPI8(__m128i vec, int p, const int LANE)
    {
        _NEON2SSE_ALIGN_16 int8_t pvec[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
        _NEON2SSE_ALIGN_16 uint8_t mask[16] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
        __m128i vec_masked, p_masked;
        pvec[LANE] = (int8_t)p;
        mask[LANE] = 0x0;
        vec_masked = _mm_and_si128 (*(__m128i*)mask,vec);         //ready for p
        p_masked = _mm_andnot_si128  (*(__m128i*)mask,*(__m128i*)pvec);         //ready for vec
        return _mm_or_si128(vec_masked, p_masked);
    }

    _NEON2SSE_INLINE __m128 _MM_INSERT_PS(__m128 vec, __m128 p, const int LANE)
    {
        _NEON2SSE_ALIGN_16 int32_t mask[4] = {0xffffffff,0xffffffff,0xffffffff,0xffffffff};
        __m128 tmp, vec_masked, p_masked;
        mask[LANE >> 4] = 0x0;         //here the LANE is not actural lane, need to deal with it
        vec_masked = _mm_and_ps (*(__m128*)mask,vec);         //ready for p
        p_masked = _mm_andnot_ps (*(__m128*)mask, p);         //ready for vec
        tmp = _mm_or_ps(vec_masked, p_masked);
        return tmp;
    }

    _NEON2SSE_INLINE __m128i _MM_MAX_EPI8(__m128i a, __m128i b)
    {
        __m128i cmp, resa, resb;
        cmp = _mm_cmpgt_epi8 (a, b);
        resa = _mm_and_si128 (cmp, a);
        resb = _mm_andnot_si128 (cmp,b);
        return _mm_or_si128(resa, resb);
    }

    _NEON2SSE_INLINE __m128i _MM_MAX_EPI32(__m128i a, __m128i b)
    {
        __m128i cmp, resa, resb;
        cmp = _mm_cmpgt_epi32(a, b);
        resa = _mm_and_si128 (cmp, a);
        resb = _mm_andnot_si128 (cmp,b);
        return _mm_or_si128(resa, resb);
    }

    _NEON2SSE_INLINE __m128i _MM_MAX_EPU16(__m128i a, __m128i b)
    {
        __m128i c8000, b_s, a_s, cmp;
        c8000 = _mm_cmpeq_epi16 (a,a);         //0xffff
        c8000 = _mm_slli_epi16 (c8000, 15);         //0x8000
        b_s = _mm_sub_epi16 (b, c8000);
        a_s = _mm_sub_epi16 (a, c8000);
        cmp = _mm_cmpgt_epi16 (a_s, b_s);         //no unsigned comparison, need to go to signed
        a_s = _mm_and_si128 (cmp,a);
        b_s = _mm_andnot_si128 (cmp,b);
        return _mm_or_si128(a_s, b_s);
    }

    _NEON2SSE_INLINE __m128i _MM_MAX_EPU32(__m128i a, __m128i b)
    {
        __m128i c80000000, b_s, a_s, cmp;
        c80000000 = _mm_cmpeq_epi32 (a,a);         //0xffffffff
        c80000000 = _mm_slli_epi32 (c80000000, 31);         //0x80000000
        b_s = _mm_sub_epi32 (b, c80000000);
        a_s = _mm_sub_epi32 (a, c80000000);
        cmp = _mm_cmpgt_epi32 (a_s, b_s);         //no unsigned comparison, need to go to signed
        a_s = _mm_and_si128 (cmp,a);
        b_s = _mm_andnot_si128 (cmp,b);
        return _mm_or_si128(a_s, b_s);
    }

    _NEON2SSE_INLINE __m128i _MM_MIN_EPI8(__m128i a, __m128i b)
    {
        __m128i cmp, resa, resb;
        cmp = _mm_cmpgt_epi8 (b, a);
        resa = _mm_and_si128 (cmp, a);
        resb = _mm_andnot_si128 (cmp,b);
        return _mm_or_si128(resa, resb);
    }

    _NEON2SSE_INLINE __m128i _MM_MIN_EPI32(__m128i a, __m128i b)
    {
        __m128i cmp, resa, resb;
        cmp = _mm_cmpgt_epi32(b, a);
        resa = _mm_and_si128 (cmp, a);
        resb = _mm_andnot_si128 (cmp,b);
        return _mm_or_si128(resa, resb);
    }

    _NEON2SSE_INLINE __m128i _MM_MIN_EPU16(__m128i a, __m128i b)
    {
        __m128i c8000, b_s, a_s, cmp;
        c8000 = _mm_cmpeq_epi16 (a,a);         //0xffff
        c8000 = _mm_slli_epi16 (c8000, 15);         //0x8000
        b_s = _mm_sub_epi16 (b, c8000);
        a_s = _mm_sub_epi16 (a, c8000);
        cmp = _mm_cmpgt_epi16 (b_s, a_s);         //no unsigned comparison, need to go to signed
        a_s = _mm_and_si128 (cmp,a);
        b_s = _mm_andnot_si128 (cmp,b);
        return _mm_or_si128(a_s, b_s);
    }

    _NEON2SSE_INLINE __m128i _MM_MIN_EPU32(__m128i a, __m128i b)
    {
        __m128i c80000000, b_s, a_s, cmp;
        c80000000 = _mm_cmpeq_epi32 (a,a);         //0xffffffff
        c80000000 = _mm_slli_epi32 (c80000000, 31);         //0x80000000
        b_s = _mm_sub_epi32 (b, c80000000);
        a_s = _mm_sub_epi32 (a, c80000000);
        cmp = _mm_cmpgt_epi32 (b_s, a_s);         //no unsigned comparison, need to go to signed
        a_s = _mm_and_si128 (cmp,a);
        b_s = _mm_andnot_si128 (cmp,b);
        return _mm_or_si128(a_s, b_s);
    }

    _NEON2SSE_INLINE __m128i  _MM_BLENDV_EPI8(__m128i a, __m128i b, __m128i mask)         //this is NOT exact implementation of _mm_blendv_epi8  !!!!! - please see below
    {         //it assumes mask is either 0xff or 0  always (like in all usecases below) while for the original _mm_blendv_epi8 only MSB mask byte matters.
        __m128i a_masked, b_masked;
        b_masked = _mm_and_si128 (mask,b);         //use b if mask 0xff
        a_masked = _mm_andnot_si128 (mask,a);
        return _mm_or_si128(a_masked, b_masked);
    }

    #if defined(USE_SSSE3)
    _NEON2SSE_INLINE __m128i _MM_PACKUS_EPI32(__m128i a, __m128i b)
    {
        _NEON2SSE_ALIGN_16 int8_t mask8_32_even_odd[16] = { 0,1, 4,5, 8,9,  12,13,  2,3, 6,7,10,11,14,15};
        __m128i a16, b16, res, reshi,cmp, zero;
        zero = _mm_setzero_si128();
        a16 = _mm_shuffle_epi8 (a, *(__m128i*) mask8_32_even_odd);
        b16 = _mm_shuffle_epi8 (b, *(__m128i*) mask8_32_even_odd);
        res = _mm_unpacklo_epi64(a16, b16);         //result without saturation
        reshi = _mm_unpackhi_epi64(a16, b16);         //hi part of result used for saturation
        cmp = _mm_cmpgt_epi16(zero, reshi);         //if cmp<0 the result should be zero
        res = _mm_andnot_si128(cmp,res);         //if cmp zero - do nothing, otherwise cmp <0  and the result is 0
        cmp = _mm_cmpgt_epi16(reshi,zero);         //if cmp positive
        return _mm_or_si128(res, cmp);         //if cmp positive we are out of 16bits need to saturaate to 0xffff
    }
    #endif

    #if defined(USE_SSSE3)
    _NEON2SSE_INLINE __m128i _MM_PACKUS1_EPI32(__m128i a)
    {
        _NEON2SSE_ALIGN_16 int8_t mask8_32_even_odd[16] = { 0,1, 4,5, 8,9,  12,13,  2,3, 6,7,10,11,14,15};
        __m128i a16, res, reshi,cmp, zero;
        zero = _mm_setzero_si128();
        a16 = _mm_shuffle_epi8 (a, *(__m128i*)mask8_32_even_odd);
        reshi = _mm_unpackhi_epi64(a16, a16);         //hi part of result used for saturation
        cmp = _mm_cmpgt_epi16(zero, reshi);         //if cmp<0 the result should be zero
        res = _mm_andnot_si128(cmp, a16);         //if cmp zero - do nothing, otherwise cmp <0  and the result is 0
        cmp = _mm_cmpgt_epi16(reshi,zero);         //if cmp positive
        return _mm_or_si128(res, cmp);         //if cmp positive we are out of 16bits need to saturaate to 0xffff
    }
    #endif

    _NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(__m128i _MM_MULLO_EPI32(__m128i a, __m128i b), _NEON2SSE_REASON_SLOW_SERIAL)
    {
        _NEON2SSE_ALIGN_16 int32_t atmp[4], btmp[4], res[4];
        int64_t res64;
        int i;
        _mm_store_si128((__m128i*)atmp, a);
        _mm_store_si128((__m128i*)btmp, b);
        for (i = 0; i<4; i++) {
            res64 = atmp[i] * btmp[i];
            res[i] = (int)(res64 & 0xffffffff);
        }
        return _mm_load_si128((__m128i*)res);
    }

    #if defined(USE_SSSE3)
    _NEON2SSE_INLINE __m128i _MM_MUL_EPI32(__m128i a, __m128i b)
    {
        __m128i sign, zero,  mul_us, a_neg, b_neg, mul_us_neg;
        sign = _mm_xor_si128 (a, b);
        sign =  _mm_srai_epi32 (sign, 31);         //promote sign bit to all fields, all fff if negative and all 0 if positive
        zero = _mm_setzero_si128();
        a_neg = _mm_abs_epi32 (a);         //negate a and b
        b_neg = _mm_abs_epi32 (b);         //negate a and b
        mul_us = _mm_mul_epu32 (a_neg, b_neg);         //uses 0 and 2nd data lanes, (abs), the multiplication gives 64 bit result
        mul_us_neg = _mm_sub_epi64(zero, mul_us);
        mul_us_neg = _mm_and_si128(sign, mul_us_neg);
        mul_us = _mm_andnot_si128(sign, mul_us);
        return _mm_or_si128 (mul_us, mul_us_neg);
    }
    #endif
#endif     //SSE4

#ifndef _MM_INSERT_EPI64     //special case of SSE4 and  _M_X64
    _NEON2SSE_INLINE __m128i  _MM_INSERT_EPI64(__m128i vec, int p, const int LANE)
    {
        _NEON2SSE_ALIGN_16 uint64_t pvec[2] = {0,0};
        _NEON2SSE_ALIGN_16 uint64_t mask[2] = {0xffffffffffffffff,0xffffffffffffffff};
        __m128i vec_masked, p_masked;
        pvec[LANE] = p;
        mask[LANE] = 0x0;
        vec_masked = _mm_and_si128 (*(__m128i*)mask,vec);         //ready for p
        p_masked = _mm_andnot_si128 (*(__m128i*)mask,*(__m128i*)pvec);         //ready for vec
        return _mm_or_si128(vec_masked, p_masked);
    }
#endif
#ifndef _MM_EXTRACT_EPI64     //special case of SSE4 and  _M_X64
    _NEON2SSE_INLINE int64_t _MM_EXTRACT_EPI64(__m128i val, const int LANE)
    {
        _NEON2SSE_ALIGN_16 int64_t tmp[2];
        _mm_store_si128((__m128i*)tmp, val);
        return tmp[LANE];
    }
#endif

int32x4_t  vqd_s32(int32x4_t a);         //Doubling saturation for signed ints
_NEON2SSE_INLINE int32x4_t  vqd_s32(int32x4_t a)
{         //Overflow happens only if a and sum have the opposite signs
    __m128i c7fffffff, res, res_sat, res_xor_a;
    c7fffffff = _mm_set1_epi32(0x7fffffff);
    res = _mm_slli_epi32 (a, 1);         // res = a*2
    res_sat = _mm_srli_epi32(a, 31);
    res_sat = _mm_add_epi32(res_sat, c7fffffff);
    res_xor_a = _mm_xor_si128(res, a);
    res_xor_a = _mm_srai_epi32(res_xor_a,31);         //propagate the sigh bit, all ffff if <0 all ones otherwise
    res_sat = _mm_and_si128(res_xor_a, res_sat);
    res = _mm_andnot_si128(res_xor_a, res);
    return _mm_or_si128(res, res_sat);
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//*************************************************************************
//*************************************************************************
//*****************  Functions redefinition\implementatin starts here *****
//*************************************************************************
//*************************************************************************
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/*If the unified intrinsics solutions is necessary please define your SSE intrinsics wrap here like in the following sample:
#ifdef ARM
#define vector_addq_s32 _mm_add_epi32
#else //if we have IA
#endif

********************************************************************************************
Functions below are organised in the following way:

Each NEON intrinsic function has one of the following options:
1.  its x86 full equivalent SSE intrinsic - in this case x86 version just follows the NEON one under the corresponding #define statement
2.  x86 implementation using more than one x86 intrinsics. In this case it is shaped as inlined C function with return statement
3.  the reference to the NEON function returning the same result and implemented in x86 as above. In this case it is shaped as matching NEON function definition
4.  for about 5% of functions due to the corresponding x86 SIMD unavailability or inefficiency in terms of performance
the serial implementation is provided along with the corresponding compiler warnin//these functions are on your app critical path
- please consider such functions removal from your code.
*/

//***********************************************************************
//************************      Vector add   *****************************
//***********************************************************************

int8x16_t   vaddq_s8(int8x16_t a, int8x16_t b);         // VADD.I8 q0,q0,q0
#define vaddq_s8 _mm_add_epi8

int16x8_t   vaddq_s16(int16x8_t a, int16x8_t b);         // VADD.I16 q0,q0,q0
#define vaddq_s16 _mm_add_epi16

int32x4_t   vaddq_s32(int32x4_t a, int32x4_t b);         // VADD.I32 q0,q0,q0
#define vaddq_s32 _mm_add_epi32

int64x2_t   vaddq_s64(int64x2_t a, int64x2_t b);         // VADD.I64 q0,q0,q0
#define vaddq_s64 _mm_add_epi64

float32x4_t vaddq_f32(float32x4_t a, float32x4_t b);         // VADD.F32 q0,q0,q0
#define vaddq_f32 _mm_add_ps

uint8x16_t   vaddq_u8(uint8x16_t a, uint8x16_t b);         // VADD.I8 q0,q0,q0
#define vaddq_u8 _mm_add_epi8

uint16x8_t   vaddq_u16(uint16x8_t a, uint16x8_t b);         // VADD.I16 q0,q0,q0
#define vaddq_u16 _mm_add_epi16

uint32x4_t   vaddq_u32(uint32x4_t a, uint32x4_t b);         // VADD.I32 q0,q0,q0
#define vaddq_u32 _mm_add_epi32

uint64x2_t   vaddq_u64(uint64x2_t a, uint64x2_t b);         // VADD.I64 q0,q0,q0
#define vaddq_u64 _mm_add_epi64

//**************************** Vector long add *****************************:
//***********************************************************************
//Va, Vb have equal lane sizes, result is a 128 bit vector of lanes that are twice the width.

//***************   Vector wide add: vaddw_<type>. Vr[i]:=Va[i]+Vb[i] ******************
//*************** *********************************************************************

//******************************Vector halving add: vhadd -> Vr[i]:=(Va[i]+Vb[i])>>1 ,  result truncated *******************************
//*************************************************************************************************************************

int8x16_t vhaddq_s8(int8x16_t a, int8x16_t b);         // VHADD.S8 q0,q0,q0
_NEON2SSE_INLINE int8x16_t vhaddq_s8(int8x16_t a, int8x16_t b)
{         //need to avoid internal overflow, will use the (x&y)+((x^y)>>1).
    __m128i tmp1, tmp2;
    tmp1 = _mm_and_si128(a,b);
    tmp2 = _mm_xor_si128(a,b);
    tmp2 = vshrq_n_s8(tmp2,1);
    return _mm_add_epi8(tmp1,tmp2);
}

int16x8_t vhaddq_s16(int16x8_t a, int16x8_t b);         // VHADD.S1 6 q0,q0,q0
_NEON2SSE_INLINE int16x8_t vhaddq_s16(int16x8_t a, int16x8_t b)
{         //need to avoid internal overflow, will use the (x&y)+((x^y)>>1).
    __m128i tmp1, tmp2;
    tmp1 = _mm_and_si128(a,b);
    tmp2 = _mm_xor_si128(a,b);
    tmp2 = _mm_srai_epi16(tmp2,1);
    return _mm_add_epi16(tmp1,tmp2);
}

int32x4_t vhaddq_s32(int32x4_t a, int32x4_t b);         // VHADD.S32 q0,q0,q0
_NEON2SSE_INLINE int32x4_t vhaddq_s32(int32x4_t a, int32x4_t b)         // VHADD.S32 q0,q0,q0
{         //need to avoid internal overflow, will use the (x&y)+((x^y)>>1).
    __m128i tmp1, tmp2;
    tmp1 = _mm_and_si128(a,b);
    tmp2 = _mm_xor_si128(a,b);
    tmp2 = _mm_srai_epi32(tmp2,1);
    return _mm_add_epi32(tmp1,tmp2);
}

uint8x16_t vhaddq_u8(uint8x16_t a, uint8x16_t b);         // VHADD.U8 q0,q0,q0
_NEON2SSE_INLINE uint8x16_t vhaddq_u8(uint8x16_t a, uint8x16_t b)         // VHADD.U8 q0,q0,q0
{
    __m128i c1, sum, res;
    c1 = _mm_set1_epi8(1);
    sum = _mm_avg_epu8(a, b);         //result is rounded, need to compensate it
    res = _mm_xor_si128(a, b);         //for rounding compensation
    res = _mm_and_si128(res,c1);         //for rounding compensation
    return _mm_sub_epi8 (sum, res);         //actual rounding compensation
}

uint16x8_t vhaddq_u16(uint16x8_t a, uint16x8_t b);         // VHADD.s16 q0,q0,q0
_NEON2SSE_INLINE uint16x8_t vhaddq_u16(uint16x8_t a, uint16x8_t b)         // VHADD.s16 q0,q0,q0
{
    __m128i sum, res;
    sum = _mm_avg_epu16(a, b);         //result is rounded, need to compensate it
    res = _mm_xor_si128(a, b);         //for rounding compensation
    res = _mm_slli_epi16 (res,15);         //shift left  then back right to
    res = _mm_srli_epi16 (res,15);         //get 1 or zero
    return _mm_sub_epi16 (sum, res);         //actual rounding compensation
}

uint32x4_t vhaddq_u32(uint32x4_t a, uint32x4_t b);         // VHADD.U32 q0,q0,q0
_NEON2SSE_INLINE uint32x4_t vhaddq_u32(uint32x4_t a, uint32x4_t b)         // VHADD.U32 q0,q0,q0
{         //need to avoid internal overflow, will use the (x&y)+((x^y)>>1).
    __m128i tmp1, tmp2;
    tmp1 = _mm_and_si128(a,b);
    tmp2 = _mm_xor_si128(a,b);
    tmp2 = _mm_srli_epi32(tmp2,1);
    return _mm_add_epi32(tmp1,tmp2);
}

//************************Vector rounding halving add: vrhadd{q}_<type>. Vr[i]:=(Va[i]+Vb[i]+1)>>1   ***************************
//*****************************************************************************************************************************

//SSE, result rounding!!!

//SSE, result rounding!!!

int8x16_t  vrhaddq_s8(int8x16_t a, int8x16_t b);         // VRHADD.S8 q0,q0,q0
_NEON2SSE_INLINE int8x16_t  vrhaddq_s8(int8x16_t a, int8x16_t b)         // VRHADD.S8 q0,q0,q0
{         //no signed average in x86 SIMD, go to unsigned
    __m128i c128, au, bu, sum;
    c128 = _mm_set1_epi8(128);
    au = _mm_add_epi8(a, c128);
    bu = _mm_add_epi8(b, c128);
    sum = _mm_avg_epu8(au, bu);
    return _mm_sub_epi8 (sum, c128);
}

int16x8_t  vrhaddq_s16(int16x8_t a, int16x8_t b);         // VRHADD.S16 q0,q0,q0
_NEON2SSE_INLINE int16x8_t  vrhaddq_s16(int16x8_t a, int16x8_t b)         // VRHADD.S16 q0,q0,q0
{         //no signed average in x86 SIMD, go to unsigned
    __m128i cx8000, au, bu, sum;
    cx8000 = _mm_set1_epi16(0x8000);
    au = _mm_add_epi16(a, cx8000);
    bu = _mm_add_epi16(b, cx8000);
    sum = _mm_avg_epu16(au, bu);
    return _mm_sub_epi16 (sum, cx8000);
}

int32x4_t  vrhaddq_s32(int32x4_t a, int32x4_t b);         // VRHADD.S32 q0,q0,q0
_NEON2SSE_INLINE int32x4_t  vrhaddq_s32(int32x4_t a, int32x4_t b)
{         //need to avoid overflow
    __m128i a2, b2, res, sum;
    a2 = _mm_srai_epi32(a,1);         //a2=a/2;
    b2 = _mm_srai_epi32(b,1);         // b2=b/2;
    res = _mm_or_si128(a,b);         //for rounding
    res = _mm_slli_epi32 (res,31);         //shift left  then back right to
    res = _mm_srli_epi32 (res,31);         //get 1 or zero
    sum = _mm_add_epi32(a2,b2);
    return _mm_add_epi32(sum,res);
}

uint8x16_t   vrhaddq_u8(uint8x16_t a, uint8x16_t b);         // VRHADD.U8 q0,q0,q0
#define vrhaddq_u8 _mm_avg_epu8         //SSE2, results rounded

uint16x8_t   vrhaddq_u16(uint16x8_t a, uint16x8_t b);         // VRHADD.s16 q0,q0,q0
#define vrhaddq_u16 _mm_avg_epu16         //SSE2, results rounded

uint32x4_t vrhaddq_u32(uint32x4_t a, uint32x4_t b);         // VRHADD.U32 q0,q0,q0
_NEON2SSE_INLINE uint32x4_t vrhaddq_u32(uint32x4_t a, uint32x4_t b)         // VRHADD.U32 q0,q0,q0
{         //need to avoid overflow
    __m128i a2, b2, res, sum;
    a2 = _mm_srli_epi32(a,1);         //a2=a/2;
    b2 = _mm_srli_epi32(b,1);         // b2=b/2;
    res = _mm_or_si128(a,b);         //for rounding
    res = _mm_slli_epi32 (res,31);         //shift left  then back right to
    res = _mm_srli_epi32 (res,31);         //get 1 or zero
    sum = _mm_add_epi32(a2,b2);
    return _mm_add_epi32(sum,res);
}

//****************** VQADD: Vector saturating add ************************
//************************************************************************

int8x16_t   vqaddq_s8(int8x16_t a, int8x16_t b);         // VQADD.S8 q0,q0,q0
#define vqaddq_s8 _mm_adds_epi8

int16x8_t   vqaddq_s16(int16x8_t a, int16x8_t b);         // VQADD.S16 q0,q0,q0
#define vqaddq_s16 _mm_adds_epi16

int32x4_t  vqaddq_s32(int32x4_t a, int32x4_t b);         // VQADD.S32 q0,q0,q0
_NEON2SSE_INLINE int32x4_t  vqaddq_s32(int32x4_t a, int32x4_t b)
{         //no corresponding x86 SIMD soulution, special tricks are necessary. Overflow happens only if a and b have the same sign and sum has the opposite sign
    __m128i c7fffffff, res, res_sat, res_xor_a, b_xor_a_;
    c7fffffff = _mm_set1_epi32(0x7fffffff);
    res = _mm_add_epi32(a, b);
    res_sat = _mm_srli_epi32(a, 31);
    res_sat = _mm_add_epi32(res_sat, c7fffffff);
    res_xor_a = _mm_xor_si128(res, a);
    b_xor_a_ = _mm_xor_si128(b, a);
    res_xor_a = _mm_andnot_si128(b_xor_a_, res_xor_a);
    res_xor_a = _mm_srai_epi32(res_xor_a,31);         //propagate the sigh bit, all ffff if <0 all ones otherwise
    res_sat = _mm_and_si128(res_xor_a, res_sat);
    res = _mm_andnot_si128(res_xor_a, res);
    return _mm_or_si128(res, res_sat);
}

int64x2_t  vqaddq_s64(int64x2_t a, int64x2_t b);         // VQADD.S64 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int64x2_t vqaddq_s64(int64x2_t a, int64x2_t b), _NEON2SSE_REASON_SLOW_SERIAL)
{
    _NEON2SSE_ALIGN_16 uint64_t atmp[2], btmp[2], res[2];
    _mm_store_si128((__m128i*)atmp, a);
    _mm_store_si128((__m128i*)btmp, b);
    res[0] = atmp[0] + btmp[0];
    res[1] = atmp[1] + btmp[1];

    atmp[0] = (atmp[0] >> 63) + (~_SIGNBIT64);
    atmp[1] = (atmp[1] >> 63) + (~_SIGNBIT64);

    if ((int64_t)((btmp[0] ^ atmp[0]) | ~(res[0] ^ btmp[0]))>=0) {
        res[0] = atmp[0];
    }
    if ((int64_t)((btmp[1] ^ atmp[1]) | ~(res[1] ^ btmp[1]))>=0) {
        res[1] = atmp[1];
    }
    return _mm_load_si128((__m128i*)res);
}

uint8x16_t   vqaddq_u8(uint8x16_t a, uint8x16_t b);         // VQADD.U8 q0,q0,q0
#define vqaddq_u8 _mm_adds_epu8

uint16x8_t   vqaddq_u16(uint16x8_t a, uint16x8_t b);         // VQADD.s16 q0,q0,q0
#define vqaddq_u16 _mm_adds_epu16

uint32x4_t vqaddq_u32(uint32x4_t a, uint32x4_t b);         // VQADD.U32 q0,q0,q0
_NEON2SSE_INLINE uint32x4_t vqaddq_u32(uint32x4_t a, uint32x4_t b)
{
    __m128i c80000000, cmp, subsum, suba, sum;
    c80000000 = _mm_set1_epi32 (0x80000000);
    sum = _mm_add_epi32 (a, b);
    subsum = _mm_sub_epi32 (sum, c80000000);
    suba = _mm_sub_epi32 (a, c80000000);
    cmp = _mm_cmpgt_epi32 ( suba, subsum);         //no unsigned comparison, need to go to signed
    return _mm_or_si128 (sum, cmp);         //saturation
}

uint64x2_t vqaddq_u64(uint64x2_t a, uint64x2_t b);         // VQADD.U64 q0,q0,q0
#ifdef USE_SSE4
    _NEON2SSE_INLINE uint64x2_t vqaddq_u64(uint64x2_t a, uint64x2_t b)
    {
        __m128i c80000000, sum, cmp, suba, subsum;
        c80000000 = _mm_set_epi32 (0x80000000, 0x0, 0x80000000, 0x0);
        sum = _mm_add_epi64 (a, b);
        subsum = _mm_sub_epi64 (sum, c80000000);
        suba = _mm_sub_epi64 (a, c80000000);
        cmp = _mm_cmpgt_epi64 ( suba, subsum);         //no unsigned comparison, need to go to signed, SSE4.2!!!
        return _mm_or_si128 (sum, cmp);         //saturation
    }
#else
    _NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint64x2_t vqaddq_u64(uint64x2_t a, uint64x2_t b), _NEON2SSE_REASON_SLOW_SERIAL)
    {
        _NEON2SSE_ALIGN_16 uint64_t atmp[2], btmp[2], res[2];
        _mm_store_si128((__m128i*)atmp, a);
        _mm_store_si128((__m128i*)btmp, b);
        res[0] = atmp[0] + btmp[0];
        res[1] = atmp[1] + btmp[1];
        if (res[0] < atmp[0]) res[0] = ~(uint64_t)0;
        if (res[1] < atmp[1]) res[1] = ~(uint64_t)0;
        return _mm_load_si128((__m128i*)(res));
    }
#endif

//******************* Vector add high half (truncated)  ******************
//************************************************************************

//*********** Vector rounding add high half: vraddhn_<type> ******************.
//***************************************************************************

//**********************************************************************************
//*********             Multiplication            *************************************
//**************************************************************************************

//Vector multiply: vmul -> Vr[i] := Va[i] * Vb[i]
//As we don't go to wider result functions are equal to "multiply low" in x86

#if defined(USE_SSSE3)
int8x16_t vmulq_s8(int8x16_t a, int8x16_t b);         // VMUL.I8 q0,q0,q0
_NEON2SSE_INLINE int8x16_t vmulq_s8(int8x16_t a, int8x16_t b)         // VMUL.I8 q0,q0,q0
{         // no 8 bit simd multiply, need to go to 16 bits
      //solution may be not optimal
    __m128i a16, b16, r16_1, r16_2;
    _NEON2SSE_ALIGN_16 int8_t mask8_16_even_odd[16] = { 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15 };
    a16 = _MM_CVTEPI8_EPI16 (a);         // SSE 4.1
    b16 = _MM_CVTEPI8_EPI16 (b);         // SSE 4.1
    r16_1 = _mm_mullo_epi16 (a16, b16);
    //swap hi and low part of a and b to process the remaining data
    a16 = _mm_shuffle_epi32 (a, _SWAP_HI_LOW32);
    b16 = _mm_shuffle_epi32 (b, _SWAP_HI_LOW32);
    a16 = _MM_CVTEPI8_EPI16 (a16);         // SSE 4.1
    b16 = _MM_CVTEPI8_EPI16 (b16);         // SSE 4.1  __m128i r16_2

    r16_2 = _mm_mullo_epi16 (a16, b16);
    r16_1 = _mm_shuffle_epi8 (r16_1, *(__m128i*)mask8_16_even_odd);         //return to 8 bit
    r16_2 = _mm_shuffle_epi8 (r16_2, *(__m128i*)mask8_16_even_odd);         //return to 8 bit

    return _mm_unpacklo_epi64(r16_1,  r16_2);
}
#endif

int16x8_t   vmulq_s16(int16x8_t a, int16x8_t b);         // VMUL.I16 q0,q0,q0
#define vmulq_s16 _mm_mullo_epi16

int32x4_t   vmulq_s32(int32x4_t a, int32x4_t b);         // VMUL.I32 q0,q0,q0
#define vmulq_s32 _MM_MULLO_EPI32         //SSE4.1

float32x4_t vmulq_f32(float32x4_t a, float32x4_t b);         // VMUL.F32 q0,q0,q0
#define vmulq_f32 _mm_mul_ps

uint8x16_t vmulq_u8(uint8x16_t a, uint8x16_t b);         // VMUL.I8 q0,q0,q0
_NEON2SSE_INLINE uint8x16_t vmulq_u8(uint8x16_t a, uint8x16_t b)         // VMUL.I8 q0,q0,q0
{         // no 8 bit simd multiply, need to go to 16 bits
      //solution may be not optimal
    __m128i maskff, a16, b16, r16_1, r16_2;
    maskff = _mm_set1_epi16(0xff);
    a16 = _MM_CVTEPU8_EPI16 (a);         // SSE 4.1
    b16 = _MM_CVTEPU8_EPI16 (b);         // SSE 4.1
    r16_1 = _mm_mullo_epi16 (a16, b16);
    r16_1 = _mm_and_si128(r16_1, maskff);         //to avoid saturation
    //swap hi and low part of a and b to process the remaining data
    a16 = _mm_shuffle_epi32 (a, _SWAP_HI_LOW32);
    b16 = _mm_shuffle_epi32 (b, _SWAP_HI_LOW32);
    a16 = _MM_CVTEPI8_EPI16 (a16);         // SSE 4.1
    b16 = _MM_CVTEPI8_EPI16 (b16);         // SSE 4.1

    r16_2 = _mm_mullo_epi16 (a16, b16);
    r16_2 = _mm_and_si128(r16_2, maskff);         //to avoid saturation
    return _mm_packus_epi16 (r16_1,  r16_2);
}

uint16x8_t   vmulq_u16(uint16x8_t a, uint16x8_t b);         // VMUL.I16 q0,q0,q0
#define vmulq_u16 _mm_mullo_epi16

uint32x4_t   vmulq_u32(uint32x4_t a, uint32x4_t b);         // VMUL.I32 q0,q0,q0
#define vmulq_u32 _MM_MULLO_EPI32         //SSE4.1

poly8x16_t vmulq_p8(poly8x16_t a, poly8x16_t b);         // VMUL.P8 q0,q0,q0
_NEON2SSE_INLINE poly8x16_t vmulq_p8(poly8x16_t a, poly8x16_t b)
{         //may be optimized
    __m128i c1, res, tmp, bmasked;
    int i;
    c1 = _mm_cmpeq_epi8 (a,a);         //all ones 0xff....
    c1 = vshrq_n_u8(c1,7);         //0x1
    bmasked = _mm_and_si128(b, c1);         //0x1
    res = vmulq_u8(a, bmasked);
    for(i = 1; i<8; i++) {
        c1 = _mm_slli_epi16(c1,1);         //shift mask left by 1, 16 bit shift is OK here
        bmasked = _mm_and_si128(b, c1);         //0x1
        tmp = vmulq_u8(a, bmasked);
        res = _mm_xor_si128(res, tmp);
    }
    return res;
}

//************************* Vector long multiply ***********************************
//****************************************************************************

//****************Vector saturating doubling long multiply **************************
//*****************************************************************

//********************* Vector multiply accumulate: vmla -> Vr[i] := Va[i] + Vb[i] * Vc[i]  ************************
//******************************************************************************************

#if defined(USE_SSSE3)
int8x16_t vmlaq_s8(int8x16_t a, int8x16_t b, int8x16_t c);         // VMLA.I8 q0,q0,q0
_NEON2SSE_INLINE int8x16_t vmlaq_s8(int8x16_t a, int8x16_t b, int8x16_t c)         // VMLA.I8 q0,q0,q0
{         //solution may be not optimal
      // no 8 bit simd multiply, need to go to 16 bits
    __m128i b16, c16, r16_1, a_2,r16_2;
    _NEON2SSE_ALIGN_16 int8_t mask8_16_even_odd[16] = { 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15 };
    b16 = _MM_CVTEPI8_EPI16 (b);         // SSE 4.1
    c16 = _MM_CVTEPI8_EPI16 (c);         // SSE 4.1
    r16_1 = _mm_mullo_epi16 (b16, c16);
    r16_1 = _mm_shuffle_epi8 (r16_1, *(__m128i*) mask8_16_even_odd);         //return to 8 bits
    r16_1 = _mm_add_epi8 (r16_1, a);
    //swap hi and low part of a, b and c to process the remaining data
    a_2 = _mm_shuffle_epi32 (a, _SWAP_HI_LOW32);
    b16 = _mm_shuffle_epi32 (b, _SWAP_HI_LOW32);
    c16 = _mm_shuffle_epi32 (c, _SWAP_HI_LOW32);
    b16 = _MM_CVTEPI8_EPI16 (b16);         // SSE 4.1
    c16 = _MM_CVTEPI8_EPI16 (c16);         // SSE 4.1

    r16_2 = _mm_mullo_epi16 (b16, c16);
    r16_2 = _mm_shuffle_epi8 (r16_2, *(__m128i*) mask8_16_even_odd);
    r16_2 = _mm_add_epi8(r16_2, a_2);
    return _mm_unpacklo_epi64(r16_1,r16_2);
}
#endif

int16x8_t vmlaq_s16(int16x8_t a, int16x8_t b, int16x8_t c);         // VMLA.I16 q0,q0,q0
_NEON2SSE_INLINE int16x8_t vmlaq_s16(int16x8_t a, int16x8_t b, int16x8_t c)         // VMLA.I16 q0,q0,q0
{
    __m128i res;
    res = _mm_mullo_epi16 (c, b);
    return _mm_add_epi16 (res, a);
}

int32x4_t vmlaq_s32(int32x4_t a, int32x4_t b, int32x4_t c);         // VMLA.I32 q0,q0,q0
_NEON2SSE_INLINE int32x4_t vmlaq_s32(int32x4_t a, int32x4_t b, int32x4_t c)         // VMLA.I32 q0,q0,q0
{
    __m128i res;
    res = _MM_MULLO_EPI32 (c,  b);         //SSE4.1
    return _mm_add_epi32 (res, a);
}

float32x4_t vmlaq_f32(float32x4_t a, float32x4_t b, float32x4_t c);         // VMLA.F32 q0,q0,q0
_NEON2SSE_INLINE float32x4_t vmlaq_f32(float32x4_t a, float32x4_t b, float32x4_t c)         // VMLA.F32 q0,q0,q0
{         //fma is coming soon, but right now:
    __m128 res;
    res = _mm_mul_ps (c, b);
    return _mm_add_ps (a, res);
}

#if defined(USE_SSSE3)
uint8x16_t vmlaq_u8(uint8x16_t a, uint8x16_t b, uint8x16_t c);         // VMLA.I8 q0,q0,q0
_NEON2SSE_INLINE uint8x16_t vmlaq_u8(uint8x16_t a, uint8x16_t b, uint8x16_t c)         // VMLA.I8 q0,q0,q0
{         //solution may be not optimal
      // no 8 bit simd multiply, need to go to 16 bits
    __m128i b16, c16, r16_1, a_2, r16_2;
    _NEON2SSE_ALIGN_16 int8_t mask8_16_even_odd[16] = { 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15 };
    b16 = _MM_CVTEPU8_EPI16 (b);         // SSE 4.1
    c16 = _MM_CVTEPU8_EPI16 (c);         // SSE 4.1
    r16_1 = _mm_mullo_epi16 (b16, c16);
    r16_1 = _mm_shuffle_epi8 (r16_1, *(__m128i*) mask8_16_even_odd);         //return to 8 bits
    r16_1 = _mm_add_epi8 (r16_1, a);
    //swap hi and low part of a, b and c to process the remaining data
    a_2 = _mm_shuffle_epi32 (a, _SWAP_HI_LOW32);
    b16 = _mm_shuffle_epi32 (b, _SWAP_HI_LOW32);
    c16 = _mm_shuffle_epi32 (c, _SWAP_HI_LOW32);
    b16 = _MM_CVTEPU8_EPI16 (b16);         // SSE 4.1
    c16 = _MM_CVTEPU8_EPI16 (c16);         // SSE 4.1

    r16_2 = _mm_mullo_epi16 (b16, c16);
    r16_2 = _mm_shuffle_epi8 (r16_2, *(__m128i*) mask8_16_even_odd);
    r16_2 = _mm_add_epi8(r16_2, a_2);
    return _mm_unpacklo_epi64(r16_1,r16_2);
}
#endif

uint16x8_t vmlaq_u16(uint16x8_t a, uint16x8_t b, uint16x8_t c);         // VMLA.I16 q0,q0,q0
#define vmlaq_u16 vmlaq_s16

uint32x4_t vmlaq_u32(uint32x4_t a, uint32x4_t b, uint32x4_t c);         // VMLA.I32 q0,q0,q0
#define vmlaq_u32 vmlaq_s32

//**********************  Vector widening multiply accumulate (long multiply accumulate):
//                          vmla -> Vr[i] := Va[i] + Vb[i] * Vc[i]  **************
//********************************************************************************************

//******************** Vector multiply subtract: vmls -> Vr[i] := Va[i] - Vb[i] * Vc[i] ***************************************
//********************************************************************************************

#if defined(USE_SSSE3)
int8x16_t vmlsq_s8(int8x16_t a, int8x16_t b, int8x16_t c);         // VMLS.I8 q0,q0,q0
_NEON2SSE_INLINE int8x16_t vmlsq_s8(int8x16_t a, int8x16_t b, int8x16_t c)         // VMLS.I8 q0,q0,q0
{         //solution may be not optimal
      // no 8 bit simd multiply, need to go to 16 bits
    __m128i b16, c16, r16_1, a_2, r16_2;
    _NEON2SSE_ALIGN_16 int8_t mask8_16_even_odd[16] = { 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15 };
    b16 = _MM_CVTEPI8_EPI16 (b);         // SSE 4.1
    c16 = _MM_CVTEPI8_EPI16 (c);         // SSE 4.1
    r16_1 = _mm_mullo_epi16 (b16, c16);
    r16_1 = _mm_shuffle_epi8 (r16_1, *(__m128i*) mask8_16_even_odd);
    r16_1 = _mm_sub_epi8 (a, r16_1);
    //swap hi and low part of a, b, c to process the remaining data
    a_2 = _mm_shuffle_epi32 (a, _SWAP_HI_LOW32);
    b16 = _mm_shuffle_epi32 (b, _SWAP_HI_LOW32);
    c16 = _mm_shuffle_epi32 (c, _SWAP_HI_LOW32);
    b16 = _MM_CVTEPI8_EPI16 (b16);         // SSE 4.1
    c16 = _MM_CVTEPI8_EPI16 (c16);         // SSE 4.1

    r16_2 = _mm_mullo_epi16 (b16, c16);
    r16_2 = _mm_shuffle_epi8 (r16_2, *(__m128i*) mask8_16_even_odd);
    r16_2 = _mm_sub_epi8 (a_2, r16_2);
    return _mm_unpacklo_epi64(r16_1,r16_2);
}
#endif

int16x8_t vmlsq_s16(int16x8_t a, int16x8_t b, int16x8_t c);         // VMLS.I16 q0,q0,q0
_NEON2SSE_INLINE int16x8_t vmlsq_s16(int16x8_t a, int16x8_t b, int16x8_t c)         // VMLS.I16 q0,q0,q0
{
    __m128i res;
    res = _mm_mullo_epi16 (c, b);
    return _mm_sub_epi16 (a, res);
}

int32x4_t vmlsq_s32(int32x4_t a, int32x4_t b, int32x4_t c);         // VMLS.I32 q0,q0,q0
_NEON2SSE_INLINE int32x4_t vmlsq_s32(int32x4_t a, int32x4_t b, int32x4_t c)         // VMLS.I32 q0,q0,q0
{
    __m128i res;
    res = _MM_MULLO_EPI32 (c, b);         //SSE4.1
    return _mm_sub_epi32 (a, res);
}

float32x4_t vmlsq_f32(float32x4_t a, float32x4_t b, float32x4_t c);         // VMLS.F32 q0,q0,q0
_NEON2SSE_INLINE float32x4_t vmlsq_f32(float32x4_t a, float32x4_t b, float32x4_t c)         // VMLS.F32 q0,q0,q0
{
    __m128 res;
    res = _mm_mul_ps (c, b);
    return _mm_sub_ps (a, res);
}

#if defined(USE_SSSE3)
uint8x16_t vmlsq_u8(uint8x16_t a, uint8x16_t b, uint8x16_t c);         // VMLS.I8 q0,q0,q0
_NEON2SSE_INLINE uint8x16_t vmlsq_u8(uint8x16_t a, uint8x16_t b, uint8x16_t c)         // VMLS.I8 q0,q0,q0
{         //solution may be not optimal
      // no 8 bit simd multiply, need to go to 16 bits
    __m128i b16, c16, r16_1, a_2, r16_2;
    _NEON2SSE_ALIGN_16 int8_t mask8_16_even_odd[16] = { 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15 };
    b16 = _MM_CVTEPU8_EPI16 (b);         // SSE 4.1
    c16 = _MM_CVTEPU8_EPI16 (c);         // SSE 4.1
    r16_1 = _mm_mullo_epi16 (b16, c16);
    r16_1 = _mm_shuffle_epi8 (r16_1, *(__m128i*) mask8_16_even_odd);         //return to 8 bits
    r16_1 = _mm_sub_epi8 (a, r16_1);
    //swap hi and low part of a, b and c to process the remaining data
    a_2 = _mm_shuffle_epi32 (a, _SWAP_HI_LOW32);
    b16 = _mm_shuffle_epi32 (b, _SWAP_HI_LOW32);
    c16 = _mm_shuffle_epi32 (c, _SWAP_HI_LOW32);
    b16 = _MM_CVTEPU8_EPI16 (b16);         // SSE 4.1
    c16 = _MM_CVTEPU8_EPI16 (c16);         // SSE 4.1

    r16_2 = _mm_mullo_epi16 (b16, c16);
    r16_2 = _mm_shuffle_epi8 (r16_2, *(__m128i*) mask8_16_even_odd);
    r16_2 = _mm_sub_epi8(a_2, r16_2);
    return _mm_unpacklo_epi64(r16_1,r16_2);
}
#endif

uint16x8_t vmlsq_u16(uint16x8_t a, uint16x8_t b, uint16x8_t c);         // VMLS.I16 q0,q0,q0
#define vmlsq_u16 vmlsq_s16

uint32x4_t vmlsq_u32(uint32x4_t a, uint32x4_t b, uint32x4_t c);         // VMLS.I32 q0,q0,q0
#define vmlsq_u32 vmlsq_s32

//******************** Vector multiply subtract long (widening multiply subtract) ************************************
//*************************************************************************************************************

//******  Vector saturating doubling multiply high **********************
//*************************************************************************
//For some ARM implementations if the multiply high result is all 0xffffffff then it is not doubled. We do the same here

int16x8_t vqdmulhq_s16(int16x8_t a, int16x8_t b);         // VQDMULH.S16 q0,q0,q0
_NEON2SSE_INLINE int16x8_t vqdmulhq_s16(int16x8_t a, int16x8_t b)         // VQDMULH.S16 q0,q0,q0
{
    __m128i res_sat, cffff, mask, res;
    res = _mm_mulhi_epi16 (a, b);
    cffff = _mm_cmpeq_epi16(res,res);         //0xffff
    mask = _mm_cmpeq_epi16(res, cffff);         //if ffff need to saturate
    res_sat = _mm_adds_epi16(res, res);         //res *= 2 and saturate
    return _mm_or_si128(mask, res_sat);
}

#if defined(USE_SSSE3)
int32x4_t vqdmulhq_s32(int32x4_t a, int32x4_t b);         // VQDMULH.S32 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int32x4_t vqdmulhq_s32(int32x4_t a, int32x4_t b), _NEON2SSE_REASON_SLOW_UNEFFECTIVE)
{         // no multiply high 32 bit SIMD in IA32, may be not optimal compared with a serial solution for the SSSE3 target
    __m128i ab, ba, res_sat, cffffffff, mask, mul, mul1;
    ab = _mm_unpacklo_epi32 (a, b);         //a0, b0, a1,b1
    ba = _mm_unpacklo_epi32 (b, a);         //b0, a0, b1,a1
    mul = _MM_MUL_EPI32(ab, ba);         //uses 1rst and 3rd data lanes, the multiplication gives 64 bit result
    ab = _mm_unpackhi_epi32 (a, b);         //a2, b2, a3,b3
    ba = _mm_unpackhi_epi32 (b, a);         //b2, a2, b3,a3
    mul1 = _MM_MUL_EPI32(ab, ba);         //uses 1rst and 3rd data lanes, the multiplication gives 64 bit result
    mul = _mm_shuffle_epi32 (mul, 1 | (3 << 2) | (0 << 4) | (2 << 6));         //shuffle the data to get 2 32-bits
    mul1 = _mm_shuffle_epi32 (mul1, 1 | (3 << 2) | (0 << 4) | (2 << 6));         //shuffle the data to get 2 32-bits
    mul = _mm_unpacklo_epi64(mul, mul1);
    cffffffff = _mm_cmpeq_epi32(mul,mul);         //0xffffffff
    mask = _mm_cmpeq_epi32(mul, cffffffff);         //if ffffffff need to saturate
    res_sat = vqd_s32(mul);
    return _mm_or_si128(mask, res_sat);
}
#endif

//********* Vector saturating rounding doubling multiply high ****************
//****************************************************************************
//If use _mm_mulhrs_xx functions  the result may differ from NEON one a little  due to different rounding rules and order

#if defined(USE_SSSE3)
int16x8_t vqrdmulhq_s16(int16x8_t a, int16x8_t b);         // VQRDMULH.S16 q0,q0,q0
_NEON2SSE_INLINE int16x8_t vqrdmulhq_s16(int16x8_t a, int16x8_t b)         // VQRDMULH.S16 q0,q0,q0
{
    __m128i res_sat, cffff, mask, res;
    res = _mm_mulhrs_epi16 (a, b);
    cffff = _mm_cmpeq_epi16(res,res);         //0xffff
    mask = _mm_cmpeq_epi16(res, cffff);         //if ffff need to saturate
    res_sat = _mm_adds_epi16(res, res);         //res *= 2 and saturate
    return _mm_or_si128(mask, res_sat);
}
#endif

#if defined(USE_SSSE3)
int32x4_t vqrdmulhq_s32(int32x4_t a, int32x4_t b);         // VQRDMULH.S32 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int32x4_t vqrdmulhq_s32(int32x4_t a, int32x4_t b), _NEON2SSE_REASON_SLOW_UNEFFECTIVE)
{         // no multiply high 32 bit SIMD in IA32, may be not optimal compared with a serial solution for the SSSE3 target
    __m128i ab, ba, res_sat, cffffffff, mask, mul, mul1, mask1;
    ab = _mm_unpacklo_epi32 (a, b);         //a0, b0, a1,b1
    ba = _mm_unpacklo_epi32 (b, a);         //b0, a0, b1,a1
    mul = _MM_MUL_EPI32(ab, ba);         //uses 1rst and 3rd data lanes, the multiplication gives 64 bit result
    ab = _mm_unpackhi_epi32 (a, b);         //a2, b2, a3,b3
    ba = _mm_unpackhi_epi32 (b, a);         //b2, a2, b3,a3
    mul1 = _MM_MUL_EPI32(ab, ba);         //uses 1rst and 3rd data lanes, the multiplication gives 64 bit result
    mul = _mm_shuffle_epi32 (mul, 1 | (3 << 2) | (0 << 4) | (2 << 6));         //shuffle the data to get 2 32-bits
    mul1 = _mm_shuffle_epi32 (mul1, 1 | (3 << 2) | (0 << 4) | (2 << 6));         //shuffle the data to get 2 32-bits
    mul = _mm_unpacklo_epi64(mul, mul1);
    cffffffff = _mm_cmpeq_epi32(mul,mul);         //0xffffffff
    mask1 = _mm_slli_epi32(mul, 17);         //shift left then back right to
    mask1 = _mm_srli_epi32(mul,31);         //get  15-th bit 1 or zero
    mul = _mm_add_epi32 (mul, mask1);         //actual rounding
    mask = _mm_cmpeq_epi32(mul, cffffffff);         //if ffffffff need to saturate
    res_sat = vqd_s32(mul);
    return _mm_or_si128(mask, res_sat);
}
#endif

//*************Vector widening saturating doubling multiply accumulate (long saturating doubling multiply accumulate) *****
//*************************************************************************************************************************

//************************************************************************************
//******************  Vector subtract ***********************************************
//************************************************************************************

int8x16_t   vsubq_s8(int8x16_t a, int8x16_t b);         // VSUB.I8 q0,q0,q0
#define vsubq_s8 _mm_sub_epi8

int16x8_t   vsubq_s16(int16x8_t a, int16x8_t b);         // VSUB.I16 q0,q0,q0
#define vsubq_s16 _mm_sub_epi16

int32x4_t   vsubq_s32(int32x4_t a, int32x4_t b);         // VSUB.I32 q0,q0,q0
#define vsubq_s32 _mm_sub_epi32

int64x2_t   vsubq_s64(int64x2_t a, int64x2_t b);         // VSUB.I64 q0,q0,q0
#define vsubq_s64 _mm_sub_epi64

float32x4_t vsubq_f32(float32x4_t a, float32x4_t b);         // VSUB.F32 q0,q0,q0
#define vsubq_f32 _mm_sub_ps

uint8x16_t   vsubq_u8(uint8x16_t a, uint8x16_t b);         // VSUB.I8 q0,q0,q0
#define vsubq_u8 _mm_sub_epi8

uint16x8_t   vsubq_u16(uint16x8_t a, uint16x8_t b);         // VSUB.I16 q0,q0,q0
#define vsubq_u16 _mm_sub_epi16

uint32x4_t   vsubq_u32(uint32x4_t a, uint32x4_t b);         // VSUB.I32 q0,q0,q0
#define vsubq_u32 _mm_sub_epi32

uint64x2_t   vsubq_u64(uint64x2_t a, uint64x2_t b);         // VSUB.I64 q0,q0,q0
#define vsubq_u64 _mm_sub_epi64

//***************Vector long subtract: vsub -> Vr[i]:=Va[i]-Vb[i] ******************
//***********************************************************************************
//Va, Vb have equal lane sizes, result is a 128 bit vector of lanes that are twice the width.

//***************** Vector wide subtract: vsub -> Vr[i]:=Va[i]-Vb[i] **********************************
//*****************************************************************************************************

//************************Vector saturating subtract *********************************
//*************************************************************************************

int8x16_t   vqsubq_s8(int8x16_t a, int8x16_t b);         // VQSUB.S8 q0,q0,q0
#define vqsubq_s8 _mm_subs_epi8

int16x8_t   vqsubq_s16(int16x8_t a, int16x8_t b);         // VQSUB.S16 q0,q0,q0
#define vqsubq_s16 _mm_subs_epi16

int32x4_t vqsubq_s32(int32x4_t a, int32x4_t b);         // VQSUB.S32 q0,q0,q0
_NEON2SSE_INLINE int32x4_t vqsubq_s32(int32x4_t a, int32x4_t b)
{         //no corresponding x86 SIMD soulution, special tricks are necessary. The overflow is possible only if a and b have opposite signs and sub has opposite sign to a
    __m128i c7fffffff, res, res_sat, res_xor_a, b_xor_a;
    c7fffffff = _mm_set1_epi32(0x7fffffff);
    res = _mm_sub_epi32(a, b);
    res_sat = _mm_srli_epi32(a, 31);
    res_sat = _mm_add_epi32(res_sat, c7fffffff);
    res_xor_a = _mm_xor_si128(res, a);
    b_xor_a = _mm_xor_si128(b, a);
    res_xor_a = _mm_and_si128(b_xor_a, res_xor_a);
    res_xor_a = _mm_srai_epi32(res_xor_a,31);         //propagate the sigh bit, all ffff if <0 all ones otherwise
    res_sat = _mm_and_si128(res_xor_a, res_sat);
    res = _mm_andnot_si128(res_xor_a, res);
    return _mm_or_si128(res, res_sat);
}

int64x2_t vqsubq_s64(int64x2_t a, int64x2_t b);         // VQSUB.S64 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int64x2_t vqsubq_s64(int64x2_t a, int64x2_t b), _NEON2SSE_REASON_SLOW_SERIAL)         //no optimal SIMD soulution
{
    _NEON2SSE_ALIGN_16 int64_t atmp[2], btmp[2];
    _NEON2SSE_ALIGN_16 uint64_t res[2];
    _mm_store_si128((__m128i*)atmp, a);
    _mm_store_si128((__m128i*)btmp, b);
    res[0] = atmp[0] - btmp[0];
    res[1] = atmp[1] - btmp[1];
    if (((res[0] ^ atmp[0]) & _SIGNBIT64) && ((atmp[0] ^ btmp[0]) & _SIGNBIT64)) {
        res[0] = (atmp[0] >> 63) ^ ~_SIGNBIT64;
    }
    if (((res[1] ^ atmp[1]) & _SIGNBIT64) && ((atmp[1] ^ btmp[1]) & _SIGNBIT64)) {
        res[1] = (atmp[1] >> 63) ^ ~_SIGNBIT64;
    }
    return _mm_load_si128((__m128i*)res);
}

uint8x16_t   vqsubq_u8(uint8x16_t a, uint8x16_t b);         // VQSUB.U8 q0,q0,q0
#define vqsubq_u8 _mm_subs_epu8

uint16x8_t   vqsubq_u16(uint16x8_t a, uint16x8_t b);         // VQSUB.s16 q0,q0,q0
#define vqsubq_u16 _mm_subs_epu16

uint32x4_t vqsubq_u32(uint32x4_t a, uint32x4_t b);         // VQSUB.U32 q0,q0,q0
_NEON2SSE_INLINE uint32x4_t vqsubq_u32(uint32x4_t a, uint32x4_t b)         // VQSUB.U32 q0,q0,q0
{
    __m128i min, mask, sub;
    min = _MM_MIN_EPU32(a, b);         //SSE4.1
    mask = _mm_cmpeq_epi32 (min,  b);
    sub = _mm_sub_epi32 (a, b);
    return _mm_and_si128 ( sub, mask);
}

_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint64x2_t vqsubq_u64(uint64x2_t a, uint64x2_t b), _NEON2SSE_REASON_SLOW_SERIAL);         // VQSUB.U64 q0,q0,q0
#ifdef USE_SSE4
    _NEON2SSE_INLINE uint64x2_t vqsubq_u64(uint64x2_t a, uint64x2_t b)
    {
        __m128i c80000000, subb, suba, cmp, sub;
        c80000000 = _mm_set_epi32 (0x80000000, 0x0, 0x80000000, 0x0);
        sub  = _mm_sub_epi64 (a, b);
        suba = _mm_sub_epi64 (a, c80000000);
        subb = _mm_sub_epi64 (b, c80000000);
        cmp = _mm_cmpgt_epi64 ( suba, subb);         //no unsigned comparison, need to go to signed, SSE4.2!!!
        return _mm_and_si128 (sub, cmp);         //saturation
    }
#else
    _NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint64x2_t vqsubq_u64(uint64x2_t a, uint64x2_t b), _NEON2SSE_REASON_SLOW_SERIAL)
    {
        _NEON2SSE_ALIGN_16 uint64_t atmp[2], btmp[2], res[2];
        _mm_store_si128((__m128i*)atmp, a);
        _mm_store_si128((__m128i*)btmp, b);
        res[0] = (atmp[0] > btmp[0]) ? atmp[0] -  btmp[0] : 0;
        res[1] = (atmp[1] > btmp[1]) ? atmp[1] -  btmp[1] : 0;
        return _mm_load_si128((__m128i*)(res));
    }
#endif

//**********Vector halving subtract Vr[i]:=(Va[i]-Vb[i])>>1  ******************************************************
//****************************************************************

int8x16_t vhsubq_s8(int8x16_t a, int8x16_t b);         // VHSUB.S8 q0,q0,q0
_NEON2SSE_INLINE int8x16_t vhsubq_s8(int8x16_t a, int8x16_t b)         // VHSUB.S8 q0,q0,q0
{         // //need to deal with the possibility of internal overflow
    __m128i c128, au,bu;
    c128 = _mm_set1_epi8 (128);
    au = _mm_add_epi8( a, c128);
    bu = _mm_add_epi8( b, c128);
    return vhsubq_u8(au,bu);
}

int16x8_t vhsubq_s16(int16x8_t a, int16x8_t b);         // VHSUB.S16 q0,q0,q0
_NEON2SSE_INLINE int16x8_t vhsubq_s16(int16x8_t a, int16x8_t b)         // VHSUB.S16 q0,q0,q0
{         //need to deal with the possibility of internal overflow
    __m128i c8000, au,bu;
    c8000 = _mm_set1_epi16(0x8000);
    au = _mm_add_epi16( a, c8000);
    bu = _mm_add_epi16( b, c8000);
    return vhsubq_u16(au,bu);
}

int32x4_t vhsubq_s32(int32x4_t a, int32x4_t b);         // VHSUB.S32 q0,q0,q0
_NEON2SSE_INLINE int32x4_t vhsubq_s32(int32x4_t a, int32x4_t b)         // VHSUB.S32 q0,q0,q0
{//need to deal with the possibility of internal overflow
    __m128i a2, b2,r, b_1;
    a2 = _mm_srai_epi32 (a,1);
    b2 = _mm_srai_epi32 (b,1);
    r = _mm_sub_epi32 (a2, b2);
    b_1 = _mm_andnot_si128(a, b); //!a and b
    b_1 = _mm_slli_epi32 (b_1,31);
    b_1 = _mm_srli_epi32 (b_1,31); //0 or 1, last b bit
    return _mm_sub_epi32(r,b_1);
}

uint8x16_t vhsubq_u8(uint8x16_t a, uint8x16_t b);         // VHSUB.U8 q0,q0,q0
_NEON2SSE_INLINE uint8x16_t vhsubq_u8(uint8x16_t a, uint8x16_t b)         // VHSUB.U8 q0,q0,q0
{
    __m128i avg;
    avg = _mm_avg_epu8 (a, b);
    return _mm_sub_epi8(a, avg);
}

uint16x8_t vhsubq_u16(uint16x8_t a, uint16x8_t b);         // VHSUB.s16 q0,q0,q0
_NEON2SSE_INLINE uint16x8_t vhsubq_u16(uint16x8_t a, uint16x8_t b)         // VHSUB.s16 q0,q0,q0
{
    __m128i avg;
    avg = _mm_avg_epu16 (a, b);
    return _mm_sub_epi16(a, avg);
}

uint32x4_t vhsubq_u32(uint32x4_t a, uint32x4_t b);         // VHSUB.U32 q0,q0,q0
_NEON2SSE_INLINE uint32x4_t vhsubq_u32(uint32x4_t a, uint32x4_t b)         // VHSUB.U32 q0,q0,q0
{//need to deal with the possibility of internal overflow
    __m128i a2, b2,r, b_1;
    a2 = _mm_srli_epi32 (a,1);
    b2 = _mm_srli_epi32 (b,1);
    r = _mm_sub_epi32 (a2, b2);
    b_1 = _mm_andnot_si128(a, b); //!a and b
    b_1 = _mm_slli_epi32 (b_1,31);
    b_1 = _mm_srli_epi32 (b_1,31); //0 or 1, last b bit
    return _mm_sub_epi32(r,b_1);
}

//******* Vector subtract high half (truncated) ** ************
//************************************************************

//************ Vector rounding subtract high half *********************
//*********************************************************************

//*********** Vector saturating doubling multiply subtract long ********************
//************************************************************************************

//******************  COMPARISON ***************************************
//******************* Vector compare equal *************************************
//****************************************************************************

uint8x16_t   vceqq_s8(int8x16_t a, int8x16_t b);         // VCEQ.I8 q0, q0, q0
#define vceqq_s8 _mm_cmpeq_epi8

uint16x8_t   vceqq_s16(int16x8_t a, int16x8_t b);         // VCEQ.I16 q0, q0, q0
#define vceqq_s16 _mm_cmpeq_epi16

uint32x4_t   vceqq_s32(int32x4_t a, int32x4_t b);         // VCEQ.I32 q0, q0, q0
#define vceqq_s32 _mm_cmpeq_epi32

uint32x4_t vceqq_f32(float32x4_t a, float32x4_t b);         // VCEQ.F32 q0, q0, q0
_NEON2SSE_INLINE uint32x4_t vceqq_f32(float32x4_t a, float32x4_t b)
{
    __m128 res;
    res = _mm_cmpeq_ps(a,b);
    return *(__m128i*)&res;
}

uint8x16_t   vceqq_u8(uint8x16_t a, uint8x16_t b);         // VCEQ.I8 q0, q0, q0
#define vceqq_u8 _mm_cmpeq_epi8

uint16x8_t   vceqq_u16(uint16x8_t a, uint16x8_t b);         // VCEQ.I16 q0, q0, q0
#define vceqq_u16 _mm_cmpeq_epi16

uint32x4_t   vceqq_u32(uint32x4_t a, uint32x4_t b);         // VCEQ.I32 q0, q0, q0
#define vceqq_u32 _mm_cmpeq_epi32

uint8x16_t   vceqq_p8(poly8x16_t a, poly8x16_t b);         // VCEQ.I8 q0, q0, q0
#define vceqq_p8 _mm_cmpeq_epi8

//******************Vector compare greater-than or equal*************************
//*******************************************************************************
//in IA SIMD no greater-than-or-equal comparison for integers,
// there is greater-than available only, so we need the following tricks

uint8x16_t vcgeq_s8(int8x16_t a, int8x16_t b);         // VCGE.S8 q0, q0, q0
_NEON2SSE_INLINE uint8x16_t vcgeq_s8(int8x16_t a, int8x16_t b)         // VCGE.S8 q0, q0, q0
{
    __m128i m1, m2;
    m1 = _mm_cmpgt_epi8 ( a, b);
    m2 = _mm_cmpeq_epi8 ( a, b);
    return _mm_or_si128  ( m1, m2);
}

uint16x8_t vcgeq_s16(int16x8_t a, int16x8_t b);         // VCGE.S16 q0, q0, q0
_NEON2SSE_INLINE uint16x8_t vcgeq_s16(int16x8_t a, int16x8_t b)         // VCGE.S16 q0, q0, q0
{
    __m128i m1, m2;
    m1 = _mm_cmpgt_epi16 ( a, b);
    m2 = _mm_cmpeq_epi16 ( a, b);
    return _mm_or_si128   ( m1,m2);
}

uint32x4_t vcgeq_s32(int32x4_t a, int32x4_t b);         // VCGE.S32 q0, q0, q0
_NEON2SSE_INLINE uint32x4_t vcgeq_s32(int32x4_t a, int32x4_t b)         // VCGE.S32 q0, q0, q0
{
    __m128i m1, m2;
    m1 = _mm_cmpgt_epi32 (a, b);
    m2 = _mm_cmpeq_epi32 (a, b);
    return _mm_or_si128   (m1, m2);
}

uint32x4_t vcgeq_f32(float32x4_t a, float32x4_t b);         // VCGE.F32 q0, q0, q0
_NEON2SSE_INLINE uint32x4_t vcgeq_f32(float32x4_t a, float32x4_t b)
{
    __m128 res;
    res = _mm_cmpge_ps(a,b);         //use only 2 first entries
    return *(__m128i*)&res;
}

uint8x16_t vcgeq_u8(uint8x16_t a, uint8x16_t b);         // VCGE.U8 q0, q0, q0
_NEON2SSE_INLINE uint8x16_t vcgeq_u8(uint8x16_t a, uint8x16_t b)         // VCGE.U8 q0, q0, q0
{         //no unsigned chars comparison, only signed available,so need the trick
    #ifdef USE_SSE4
        __m128i cmp;
        cmp = _mm_max_epu8(a, b);
        return _mm_cmpeq_epi8(cmp, a);         //a>=b
    #else
        __m128i c128, as, bs, m1, m2;
        c128 = _mm_set1_epi8 (128);
        as = _mm_sub_epi8( a, c128);
        bs = _mm_sub_epi8( b, c128);
        m1 = _mm_cmpgt_epi8( as, bs);
        m2 = _mm_cmpeq_epi8 (as, bs);
        return _mm_or_si128 ( m1,  m2);
    #endif
}

uint16x8_t vcgeq_u16(uint16x8_t a, uint16x8_t b);         // VCGE.s16 q0, q0, q0
_NEON2SSE_INLINE uint16x8_t vcgeq_u16(uint16x8_t a, uint16x8_t b)         // VCGE.s16 q0, q0, q0
{         //no unsigned shorts comparison, only signed available,so need the trick
    #ifdef USE_SSE4
        __m128i cmp;
        cmp = _mm_max_epu16(a, b);
        return _mm_cmpeq_epi16(cmp, a);         //a>=b
    #else
        __m128i c8000, as, bs, m1, m2;
        c8000 = _mm_set1_epi16 (0x8000);
        as = _mm_sub_epi16(a,c8000);
        bs = _mm_sub_epi16(b,c8000);
        m1 = _mm_cmpgt_epi16(as, bs);
        m2 = _mm_cmpeq_epi16 (as, bs);
        return _mm_or_si128 ( m1, m2);
    #endif
}

uint32x4_t vcgeq_u32(uint32x4_t a, uint32x4_t b);         // VCGE.U32 q0, q0, q0
_NEON2SSE_INLINE uint32x4_t vcgeq_u32(uint32x4_t a, uint32x4_t b)         // VCGE.U32 q0, q0, q0
{         //no unsigned ints comparison, only signed available,so need the trick
    #ifdef USE_SSE4
        __m128i cmp;
        cmp = _mm_max_epu32(a, b);
        return _mm_cmpeq_epi32(cmp, a);         //a>=b
    #else
        //serial solution may be faster
        __m128i c80000000, as, bs, m1, m2;
        c80000000 = _mm_set1_epi32 (0x80000000);
        as = _mm_sub_epi32(a,c80000000);
        bs = _mm_sub_epi32(b,c80000000);
        m1 = _mm_cmpgt_epi32 (as, bs);
        m2 = _mm_cmpeq_epi32 (as, bs);
        return _mm_or_si128 ( m1,  m2);
    #endif
}

//**********************Vector compare less-than or equal******************************
//***************************************************************************************
//in IA SIMD no less-than-or-equal comparison for integers present, so we need the tricks

uint8x16_t vcleq_s8(int8x16_t a, int8x16_t b);         // VCGE.S8 q0, q0, q0
_NEON2SSE_INLINE uint8x16_t vcleq_s8(int8x16_t a, int8x16_t b)         // VCGE.S8 q0, q0, q0
{
    __m128i c1, res;
    c1 = _mm_cmpeq_epi8 (a,a);         //all ones 0xff....
    res = _mm_cmpgt_epi8 ( a,  b);
    return _mm_andnot_si128 (res, c1);         //inverse the cmpgt result, get less-than-or-equal
}

uint16x8_t vcleq_s16(int16x8_t a, int16x8_t b);         // VCGE.S16 q0, q0, q0
_NEON2SSE_INLINE uint16x8_t vcleq_s16(int16x8_t a, int16x8_t b)         // VCGE.S16 q0, q0, q0
{
    __m128i c1, res;
    c1 = _mm_cmpeq_epi16 (a,a);         //all ones 0xff....
    res = _mm_cmpgt_epi16 ( a,  b);
    return _mm_andnot_si128 (res, c1);
}

uint32x4_t vcleq_s32(int32x4_t a, int32x4_t b);         // VCGE.S32 q0, q0, q0
_NEON2SSE_INLINE uint32x4_t vcleq_s32(int32x4_t a, int32x4_t b)         // VCGE.S32 q0, q0, q0
{
    __m128i c1, res;
    c1 = _mm_cmpeq_epi32 (a,a);         //all ones 0xff....
    res = _mm_cmpgt_epi32 ( a,  b);
    return _mm_andnot_si128 (res, c1);
}

uint32x4_t vcleq_f32(float32x4_t a, float32x4_t b);         // VCGE.F32 q0, q0, q0
_NEON2SSE_INLINE uint32x4_t vcleq_f32(float32x4_t a, float32x4_t b)
{
    __m128 res;
    res = _mm_cmple_ps(a,b);
    return *(__m128i*)&res;
}

uint8x16_t vcleq_u8(uint8x16_t a, uint8x16_t b);         // VCGE.U8 q0, q0, q0
#ifdef USE_SSE4
    _NEON2SSE_INLINE uint8x16_t vcleq_u8(uint8x16_t a, uint8x16_t b)         // VCGE.U8 q0, q0, q0
    {         //no unsigned chars comparison in SSE, only signed available,so need the trick

        __m128i cmp;
        cmp = _mm_min_epu8(a, b);
        return _mm_cmpeq_epi8(cmp, a);         //a<=b
    }
#else
    #define vcleq_u8(a,b) vcgeq_u8(b,a)
#endif

uint16x8_t vcleq_u16(uint16x8_t a, uint16x8_t b);         // VCGE.s16 q0, q0, q0
#ifdef USE_SSE4
    _NEON2SSE_INLINE uint16x8_t vcleq_u16(uint16x8_t a, uint16x8_t b)         // VCGE.s16 q0, q0, q0
    {         //no unsigned shorts comparison in SSE, only signed available,so need the trick
        __m128i cmp;
        cmp = _mm_min_epu16(a, b);
        return _mm_cmpeq_epi16(cmp, a);         //a<=b
    }
#else
    #define vcleq_u16(a,b) vcgeq_u16(b,a)
#endif

uint32x4_t vcleq_u32(uint32x4_t a, uint32x4_t b);         // VCGE.U32 q0, q0, q0
#ifdef USE_SSE4
    _NEON2SSE_INLINE uint32x4_t vcleq_u32(uint32x4_t a, uint32x4_t b)         // VCGE.U32 q0, q0, q0
    {         //no unsigned chars comparison in SSE, only signed available,so need the trick
        __m128i cmp;
        cmp = _mm_min_epu32(a, b);
        return _mm_cmpeq_epi32(cmp, a);         //a<=b
    }
#else
//solution may be not optimal compared with the serial one
    #define vcleq_u32(a,b) vcgeq_u32(b,a)
#endif

//****** Vector compare greater-than ******************************************
//**************************************************************************

uint8x16_t   vcgtq_s8(int8x16_t a, int8x16_t b);         // VCGT.S8 q0, q0, q0
#define vcgtq_s8 _mm_cmpgt_epi8

uint16x8_t   vcgtq_s16(int16x8_t a, int16x8_t b);         // VCGT.S16 q0, q0, q0
#define vcgtq_s16 _mm_cmpgt_epi16

uint32x4_t   vcgtq_s32(int32x4_t a, int32x4_t b);         // VCGT.S32 q0, q0, q0
#define vcgtq_s32 _mm_cmpgt_epi32

uint32x4_t vcgtq_f32(float32x4_t a, float32x4_t b);         // VCGT.F32 q0, q0, q0
_NEON2SSE_INLINE uint32x4_t vcgtq_f32(float32x4_t a, float32x4_t b)
{
    __m128 res;
    res = _mm_cmpgt_ps(a,b);         //use only 2 first entries
    return *(__m128i*)&res;
}

uint8x16_t vcgtq_u8(uint8x16_t a, uint8x16_t b);         // VCGT.U8 q0, q0, q0
_NEON2SSE_INLINE uint8x16_t vcgtq_u8(uint8x16_t a, uint8x16_t b)         // VCGT.U8 q0, q0, q0
{         //no unsigned chars comparison, only signed available,so need the trick
    __m128i c128, as, bs;
    c128 = _mm_set1_epi8 (128);
    as = _mm_sub_epi8(a,c128);
    bs = _mm_sub_epi8(b,c128);
    return _mm_cmpgt_epi8 (as, bs);
}

uint16x8_t vcgtq_u16(uint16x8_t a, uint16x8_t b);         // VCGT.s16 q0, q0, q0
_NEON2SSE_INLINE uint16x8_t vcgtq_u16(uint16x8_t a, uint16x8_t b)         // VCGT.s16 q0, q0, q0
{         //no unsigned short comparison, only signed available,so need the trick
    __m128i c8000, as, bs;
    c8000 = _mm_set1_epi16 (0x8000);
    as = _mm_sub_epi16(a,c8000);
    bs = _mm_sub_epi16(b,c8000);
    return _mm_cmpgt_epi16 ( as, bs);
}

uint32x4_t vcgtq_u32(uint32x4_t a, uint32x4_t b);         // VCGT.U32 q0, q0, q0
_NEON2SSE_INLINE uint32x4_t vcgtq_u32(uint32x4_t a, uint32x4_t b)         // VCGT.U32 q0, q0, q0
{         //no unsigned int comparison, only signed available,so need the trick
    __m128i c80000000, as, bs;
    c80000000 = _mm_set1_epi32 (0x80000000);
    as = _mm_sub_epi32(a,c80000000);
    bs = _mm_sub_epi32(b,c80000000);
    return _mm_cmpgt_epi32 ( as, bs);
}

//********************* Vector compare less-than **************************
//*************************************************************************

uint8x16_t   vcltq_s8(int8x16_t a, int8x16_t b);         // VCGT.S8 q0, q0, q0
#define vcltq_s8(a,b) vcgtq_s8(b, a)         //swap the arguments!!

uint16x8_t   vcltq_s16(int16x8_t a, int16x8_t b);         // VCGT.S16 q0, q0, q0
#define vcltq_s16(a,b) vcgtq_s16(b, a)         //swap the arguments!!

uint32x4_t   vcltq_s32(int32x4_t a, int32x4_t b);         // VCGT.S32 q0, q0, q0
#define vcltq_s32(a,b) vcgtq_s32(b, a)         //swap the arguments!!

uint32x4_t vcltq_f32(float32x4_t a, float32x4_t b);         // VCGT.F32 q0, q0, q0
#define vcltq_f32(a,b) vcgtq_f32(b, a)         //swap the arguments!!

uint8x16_t vcltq_u8(uint8x16_t a, uint8x16_t b);         // VCGT.U8 q0, q0, q0
#define vcltq_u8(a,b) vcgtq_u8(b, a)         //swap the arguments!!

uint16x8_t vcltq_u16(uint16x8_t a, uint16x8_t b);         // VCGT.s16 q0, q0, q0
#define vcltq_u16(a,b) vcgtq_u16(b, a)         //swap the arguments!!

uint32x4_t vcltq_u32(uint32x4_t a, uint32x4_t b);         // VCGT.U32 q0, q0, q0
#define vcltq_u32(a,b) vcgtq_u32(b, a)         //swap the arguments!!

//*****************Vector compare absolute greater-than or equal ************
//***************************************************************************

uint32x4_t vcageq_f32(float32x4_t a, float32x4_t b);         // VACGE.F32 q0, q0, q0
_NEON2SSE_INLINE uint32x4_t vcageq_f32(float32x4_t a, float32x4_t b)         // VACGE.F32 q0, q0, q0
{
    __m128i c7fffffff;
    __m128 a0, b0;
    c7fffffff = _mm_set1_epi32 (0x7fffffff);
    a0 = _mm_and_ps (a, *(__m128*)&c7fffffff);
    b0 = _mm_and_ps (b, *(__m128*)&c7fffffff);
    a0 = _mm_cmpge_ps ( a0, b0);
    return (*(__m128i*)&a0);
}

//********Vector compare absolute less-than or equal ******************
//********************************************************************

uint32x4_t vcaleq_f32(float32x4_t a, float32x4_t b);         // VACGE.F32 q0, q0, q0
_NEON2SSE_INLINE uint32x4_t vcaleq_f32(float32x4_t a, float32x4_t b)         // VACGE.F32 q0, q0, q0
{
    __m128i c7fffffff;
    __m128 a0, b0;
    c7fffffff = _mm_set1_epi32 (0x7fffffff);
    a0 = _mm_and_ps (a, *(__m128*)&c7fffffff);
    b0 = _mm_and_ps (b, *(__m128*)&c7fffffff);
    a0 = _mm_cmple_ps (a0, b0);
    return (*(__m128i*)&a0);
}

//********  Vector compare absolute greater-than    ******************
//******************************************************************

uint32x4_t vcagtq_f32(float32x4_t a, float32x4_t b);         // VACGT.F32 q0, q0, q0
_NEON2SSE_INLINE uint32x4_t vcagtq_f32(float32x4_t a, float32x4_t b)         // VACGT.F32 q0, q0, q0
{
    __m128i c7fffffff;
    __m128 a0, b0;
    c7fffffff = _mm_set1_epi32 (0x7fffffff);
    a0 = _mm_and_ps (a, *(__m128*)&c7fffffff);
    b0 = _mm_and_ps (b, *(__m128*)&c7fffffff);
    a0 = _mm_cmpgt_ps (a0, b0);
    return (*(__m128i*)&a0);
}

//***************Vector compare absolute less-than  ***********************
//*************************************************************************

uint32x4_t vcaltq_f32(float32x4_t a, float32x4_t b);         // VACGT.F32 q0, q0, q0
_NEON2SSE_INLINE uint32x4_t vcaltq_f32(float32x4_t a, float32x4_t b)         // VACGT.F32 q0, q0, q0
{
    __m128i c7fffffff;
    __m128 a0, b0;
    c7fffffff = _mm_set1_epi32 (0x7fffffff);
    a0 = _mm_and_ps (a, *(__m128*)&c7fffffff);
    b0 = _mm_and_ps (b, *(__m128*)&c7fffffff);
    a0 = _mm_cmplt_ps (a0, b0);
    return (*(__m128i*)&a0);

}

//*************************Vector test bits************************************
//*****************************************************************************
/*VTST (Vector Test Bits) takes each element in a vector, and bitwise logical ANDs them
with the corresponding element of a second vector. If the result is not zero, the
corresponding element in the destination vector is set to all ones. Otherwise, it is set to
all zeros. */

uint8x16_t vtstq_s8(int8x16_t a, int8x16_t b);         // VTST.8 q0, q0, q0
_NEON2SSE_INLINE uint8x16_t vtstq_s8(int8x16_t a, int8x16_t b)         // VTST.8 q0, q0, q0
{
    __m128i zero, one, res;
    zero = _mm_setzero_si128 ();
    one = _mm_cmpeq_epi8(zero,zero);         //0xfff..ffff
    res = _mm_and_si128 (a, b);
    res =  _mm_cmpeq_epi8 (res, zero);
    return _mm_xor_si128(res, one);         //invert result
}

uint16x8_t vtstq_s16(int16x8_t a, int16x8_t b);         // VTST.16 q0, q0, q0
_NEON2SSE_INLINE uint16x8_t vtstq_s16(int16x8_t a, int16x8_t b)         // VTST.16 q0, q0, q0
{
    __m128i zero, one, res;
    zero = _mm_setzero_si128 ();
    one = _mm_cmpeq_epi8(zero,zero);         //0xfff..ffff
    res = _mm_and_si128 (a, b);
    res =  _mm_cmpeq_epi16 (res, zero);
    return _mm_xor_si128(res, one);         //invert result
}

uint32x4_t vtstq_s32(int32x4_t a, int32x4_t b);         // VTST.32 q0, q0, q0
_NEON2SSE_INLINE uint32x4_t vtstq_s32(int32x4_t a, int32x4_t b)         // VTST.32 q0, q0, q0
{
    __m128i zero, one, res;
    zero = _mm_setzero_si128 ();
    one = _mm_cmpeq_epi8(zero,zero);         //0xfff..ffff
    res = _mm_and_si128 (a, b);
    res =  _mm_cmpeq_epi32 (res, zero);
    return _mm_xor_si128(res, one);         //invert result
}

uint8x16_t vtstq_u8(uint8x16_t a, uint8x16_t b);         // VTST.8 q0, q0, q0
#define vtstq_u8 vtstq_s8

uint16x8_t vtstq_u16(uint16x8_t a, uint16x8_t b);         // VTST.16 q0, q0, q0
#define vtstq_u16 vtstq_s16

uint32x4_t vtstq_u32(uint32x4_t a, uint32x4_t b);         // VTST.32 q0, q0, q0
#define vtstq_u32 vtstq_s32

uint8x16_t vtstq_p8(poly8x16_t a, poly8x16_t b);         // VTST.8 q0, q0, q0
#define vtstq_p8 vtstq_u8

//****************** Absolute difference ********************
//*** Absolute difference between the arguments: Vr[i] = | Va[i] - Vb[i] |*****
//************************************************************
#if defined(USE_SSSE3)

#endif

#if defined(USE_SSSE3)
int8x16_t vabdq_s8(int8x16_t a, int8x16_t b);         // VABD.S8 q0,q0,q0
_NEON2SSE_INLINE int8x16_t vabdq_s8(int8x16_t a, int8x16_t b)         // VABD.S8 q0,q0,q0
{
    __m128i res;
    res = _mm_sub_epi8 (a, b);
    return _mm_abs_epi8 (res);
}
#endif

#if defined(USE_SSSE3)
int16x8_t vabdq_s16(int16x8_t a, int16x8_t b);         // VABD.S16 q0,q0,q0
_NEON2SSE_INLINE int16x8_t vabdq_s16(int16x8_t a, int16x8_t b)         // VABD.S16 q0,q0,q0
{
    __m128i res;
    res = _mm_sub_epi16 (a,b);
    return _mm_abs_epi16 (res);
}
#endif

#if defined(USE_SSSE3)
int32x4_t vabdq_s32(int32x4_t a, int32x4_t b);         // VABD.S32 q0,q0,q0
_NEON2SSE_INLINE int32x4_t vabdq_s32(int32x4_t a, int32x4_t b)         // VABD.S32 q0,q0,q0
{
    __m128i res;
    res = _mm_sub_epi32 (a,b);
    return _mm_abs_epi32 (res);
}
#endif

uint8x16_t vabdq_u8(uint8x16_t a, uint8x16_t b);         // VABD.U8 q0,q0,q0
_NEON2SSE_INLINE uint8x16_t vabdq_u8(uint8x16_t a, uint8x16_t b)         //no abs for unsigned
{
    __m128i cmp, difab, difba;
    cmp = vcgtq_u8(a,b);
    difab = _mm_sub_epi8(a,b);
    difba = _mm_sub_epi8 (b,a);
    difab = _mm_and_si128(cmp, difab);
    difba = _mm_andnot_si128(cmp, difba);
    return _mm_or_si128(difab, difba);
}

uint16x8_t vabdq_u16(uint16x8_t a, uint16x8_t b);         // VABD.s16 q0,q0,q0
_NEON2SSE_INLINE uint16x8_t vabdq_u16(uint16x8_t a, uint16x8_t b)
{
    __m128i cmp, difab, difba;
    cmp = vcgtq_u16(a,b);
    difab = _mm_sub_epi16(a,b);
    difba = _mm_sub_epi16 (b,a);
    difab = _mm_and_si128(cmp, difab);
    difba = _mm_andnot_si128(cmp, difba);
    return _mm_or_si128(difab, difba);
}

uint32x4_t vabdq_u32(uint32x4_t a, uint32x4_t b);         // VABD.U32 q0,q0,q0
_NEON2SSE_INLINE uint32x4_t vabdq_u32(uint32x4_t a, uint32x4_t b)
{
    __m128i cmp, difab, difba;
    cmp = vcgtq_u32(a,b);
    difab = _mm_sub_epi32(a,b);
    difba = _mm_sub_epi32 (b,a);
    difab = _mm_and_si128(cmp, difab);
    difba = _mm_andnot_si128(cmp, difba);
    return _mm_or_si128(difab, difba);
}

float32x4_t vabdq_f32(float32x4_t a, float32x4_t b);         // VABD.F32 q0,q0,q0
_NEON2SSE_INLINE float32x4_t vabdq_f32(float32x4_t a, float32x4_t b)         // VABD.F32 q0,q0,q0
{
    __m128i c1;
    __m128 res;
    c1 =  _mm_set1_epi32(0x7fffffff);
    res = _mm_sub_ps (a, b);
    return _mm_and_ps (res, *(__m128*)&c1);
}

//************  Absolute difference - long **************************
//********************************************************************

//**********Absolute difference and accumulate: Vr[i] = Va[i] + | Vb[i] - Vc[i] | *************
//*********************************************************************************************

#if defined(USE_SSSE3)
int8x16_t vabaq_s8(int8x16_t a, int8x16_t b, int8x16_t c);         // VABA.S8 q0,q0,q0
_NEON2SSE_INLINE int8x16_t vabaq_s8(int8x16_t a, int8x16_t b, int8x16_t c)         // VABA.S8 q0,q0,q0
{
    int8x16_t sub;
    sub = vabdq_s8(b, c);
    return vaddq_s8( a, sub);
}
#endif

#if defined(USE_SSSE3)
int16x8_t vabaq_s16(int16x8_t a, int16x8_t b, int16x8_t c);         // VABA.S16 q0,q0,q0
_NEON2SSE_INLINE int16x8_t vabaq_s16(int16x8_t a, int16x8_t b, int16x8_t c)         // VABA.S16 q0,q0,q0
{
    int16x8_t sub;
    sub = vabdq_s16(b, c);
    return vaddq_s16( a, sub);
}
#endif

#if defined(USE_SSSE3)
int32x4_t vabaq_s32(int32x4_t a, int32x4_t b, int32x4_t c);         // VABA.S32 q0,q0,q0
_NEON2SSE_INLINE int32x4_t vabaq_s32(int32x4_t a, int32x4_t b, int32x4_t c)         // VABA.S32 q0,q0,q0
{
    int32x4_t sub;
    sub = vabdq_s32(b, c);
    return vaddq_s32( a, sub);
}
#endif

uint8x16_t vabaq_u8(uint8x16_t a, uint8x16_t b, uint8x16_t c);         // VABA.U8 q0,q0,q0
_NEON2SSE_INLINE uint8x16_t vabaq_u8(uint8x16_t a, uint8x16_t b, uint8x16_t c)
{
    uint8x16_t sub;
    sub = vabdq_u8(b, c);
    return vaddq_u8( a, sub);
}

uint16x8_t vabaq_u16(uint16x8_t a, uint16x8_t b, uint16x8_t c);         // VABA.s16 q0,q0,q0
_NEON2SSE_INLINE uint16x8_t vabaq_u16(uint16x8_t a, uint16x8_t b, uint16x8_t c)
{
    uint16x8_t sub;
    sub = vabdq_u16(b, c);
    return vaddq_u16( a, sub);
}

uint32x4_t vabaq_u32(uint32x4_t a, uint32x4_t b, uint32x4_t c);         // VABA.U32 q0,q0,q0
_NEON2SSE_INLINE uint32x4_t vabaq_u32(uint32x4_t a, uint32x4_t b, uint32x4_t c)
{
    uint32x4_t sub;
    sub = vabdq_u32(b, c);
    return vaddq_u32( a, sub);
}

//************** Absolute difference and accumulate - long ********************************
//*************************************************************************************

//***********************************************************************************
//****************  Maximum and minimum operations **********************************
//***********************************************************************************
//************* Maximum:  vmax -> Vr[i] := (Va[i] >= Vb[i]) ? Va[i] : Vb[i]    *******
//***********************************************************************************

int8x16_t   vmaxq_s8(int8x16_t a, int8x16_t b);         // VMAX.S8 q0,q0,q0
#define vmaxq_s8 _MM_MAX_EPI8         //SSE4.1

int16x8_t   vmaxq_s16(int16x8_t a, int16x8_t b);         // VMAX.S16 q0,q0,q0
#define vmaxq_s16 _mm_max_epi16

int32x4_t   vmaxq_s32(int32x4_t a, int32x4_t b);         // VMAX.S32 q0,q0,q0
#define vmaxq_s32 _MM_MAX_EPI32         //SSE4.1

uint8x16_t   vmaxq_u8(uint8x16_t a, uint8x16_t b);         // VMAX.U8 q0,q0,q0
#define vmaxq_u8 _mm_max_epu8

uint16x8_t   vmaxq_u16(uint16x8_t a, uint16x8_t b);         // VMAX.s16 q0,q0,q0
#define vmaxq_u16 _MM_MAX_EPU16         //SSE4.1

uint32x4_t   vmaxq_u32(uint32x4_t a, uint32x4_t b);         // VMAX.U32 q0,q0,q0
#define vmaxq_u32 _MM_MAX_EPU32         //SSE4.1

float32x4_t vmaxq_f32(float32x4_t a, float32x4_t b);         // VMAX.F32 q0,q0,q0
#define vmaxq_f32 _mm_max_ps

//*************** Minimum: vmin -> Vr[i] := (Va[i] >= Vb[i]) ? Vb[i] : Va[i] ********************************
//***********************************************************************************************************

int8x16_t   vminq_s8(int8x16_t a, int8x16_t b);         // VMIN.S8 q0,q0,q0
#define vminq_s8 _MM_MIN_EPI8         //SSE4.1

int16x8_t   vminq_s16(int16x8_t a, int16x8_t b);         // VMIN.S16 q0,q0,q0
#define vminq_s16 _mm_min_epi16

int32x4_t   vminq_s32(int32x4_t a, int32x4_t b);         // VMIN.S32 q0,q0,q0
#define vminq_s32 _MM_MIN_EPI32         //SSE4.1

uint8x16_t   vminq_u8(uint8x16_t a, uint8x16_t b);         // VMIN.U8 q0,q0,q0
#define vminq_u8 _mm_min_epu8

uint16x8_t   vminq_u16(uint16x8_t a, uint16x8_t b);         // VMIN.s16 q0,q0,q0
#define vminq_u16 _MM_MIN_EPU16         //SSE4.1

uint32x4_t   vminq_u32(uint32x4_t a, uint32x4_t b);         // VMIN.U32 q0,q0,q0
#define vminq_u32 _MM_MIN_EPU32         //SSE4.1

float32x4_t vminq_f32(float32x4_t a, float32x4_t b);         // VMIN.F32 q0,q0,q0
#define vminq_f32 _mm_min_ps

//*************  Pairwise addition operations. **************************************
//************************************************************************************
//Pairwise add - adds adjacent pairs of elements of two vectors, and places the results in the destination vector

//**************************  Long pairwise add  **********************************
//*********************************************************************************
//Adds adjacent pairs of elements of a vector,sign or zero extends the results to twice their original width,
// and places the final results in the destination vector.

#if defined(USE_SSSE3)
int16x8_t vpaddlq_s8(int8x16_t a);         // VPADDL.S8 q0,q0
_NEON2SSE_INLINE int16x8_t vpaddlq_s8(int8x16_t a)         // VPADDL.S8 q0,q0
{         //no 8 bit hadd in IA32, need to go to 16 bit
    __m128i r16_1, r16_2;
    r16_1 = _MM_CVTEPI8_EPI16 (a);         // SSE 4.1
    //swap hi and low part of r to process the remaining data
    r16_2 = _mm_shuffle_epi32 (a, _SWAP_HI_LOW32);
    r16_2 = _MM_CVTEPI8_EPI16 (r16_2);
    return _mm_hadd_epi16 (r16_1, r16_2);
}
#endif

#if defined(USE_SSSE3)
int32x4_t vpaddlq_s16(int16x8_t a);         // VPADDL.S16 q0,q0
_NEON2SSE_INLINE int32x4_t vpaddlq_s16(int16x8_t a)         // VPADDL.S16 q0,q0
{         //no 8 bit hadd in IA32, need to go to 16 bit
    __m128i r32_1, r32_2;
    r32_1 = _MM_CVTEPI16_EPI32(a);
    //swap hi and low part of r to process the remaining data
    r32_2 = _mm_shuffle_epi32 (a, _SWAP_HI_LOW32);
    r32_2 = _MM_CVTEPI16_EPI32 (r32_2);
    return _mm_hadd_epi32 (r32_1, r32_2);
}
#endif

int64x2_t vpaddlq_s32(int32x4_t a);         // VPADDL.S32 q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int64x2_t vpaddlq_s32(int32x4_t a), _NEON2SSE_REASON_SLOW_SERIAL)         // VPADDL.S32 q0,q0
{
    _NEON2SSE_ALIGN_16 int32_t atmp[4];
    _NEON2SSE_ALIGN_16 int64_t res[2];
    _mm_store_si128((__m128i*)atmp, a);
    res[0] = (int64_t)atmp[0] + (int64_t)atmp[1];
    res[1] = (int64_t)atmp[2] + (int64_t)atmp[3];
    return _mm_load_si128((__m128i*)res);
}

#if defined(USE_SSSE3)
uint16x8_t vpaddlq_u8(uint8x16_t a);         // VPADDL.U8 q0,q0
_NEON2SSE_INLINE uint16x8_t vpaddlq_u8(uint8x16_t a)         // VPADDL.U8 q0,q0
{         //no 8 bit hadd in IA32, need to go to 16 bit
    __m128i r16_1, r16_2;
    r16_1 = _MM_CVTEPU8_EPI16(a);
    //swap hi and low part of r to process the remaining data
    r16_2 = _mm_shuffle_epi32 (a, _SWAP_HI_LOW32);
    r16_2 = _MM_CVTEPU8_EPI16 (r16_2);
    return _mm_hadd_epi16 (r16_1, r16_2);
}
#endif

uint32x4_t vpaddlq_u16(uint16x8_t a);         // VPADDL.s16 q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint32x4_t vpaddlq_u16(uint16x8_t a),  _NEON2SSE_REASON_SLOW_SERIAL)
{         //serial solution looks faster than a SIMD one
    _NEON2SSE_ALIGN_16 uint16_t atmp[8];
    _NEON2SSE_ALIGN_16 uint32_t res[4];
    _mm_store_si128((__m128i*)atmp, a);
    res[0] = (uint32_t)atmp[0] + (uint32_t)atmp[1];
    res[1] = (uint32_t)atmp[2] + (uint32_t)atmp[3];
    res[2] = (uint32_t)atmp[4] + (uint32_t)atmp[5];
    res[3] = (uint32_t)atmp[6] + (uint32_t)atmp[7];
    return _mm_load_si128((__m128i*)res);
}

uint64x2_t vpaddlq_u32(uint32x4_t a);         // VPADDL.U32 q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint64x2_t vpaddlq_u32(uint32x4_t a), _NEON2SSE_REASON_SLOW_SERIAL)
{
    _NEON2SSE_ALIGN_16 uint32_t atmp[4];
    _NEON2SSE_ALIGN_16 uint64_t res[2];
    _mm_store_si128((__m128i*)atmp, a);
    res[0] = (uint64_t)atmp[0] + (uint64_t)atmp[1];
    res[1] = (uint64_t)atmp[2] + (uint64_t)atmp[3];
    return _mm_load_si128((__m128i*)res);
}

//************************  Long pairwise add and accumulate **************************
//****************************************************************************************
//VPADAL (Vector Pairwise Add and Accumulate Long) adds adjacent pairs of elements of a vector,
// and accumulates the  values of the results into the elements of the destination (wide) vector

#if defined(USE_SSSE3)
int16x8_t vpadalq_s8(int16x8_t a, int8x16_t b);         // VPADAL.S8 q0,q0
_NEON2SSE_INLINE int16x8_t vpadalq_s8(int16x8_t a, int8x16_t b)         // VPADAL.S8 q0,q0
{
    int16x8_t pad;
    pad = vpaddlq_s8(b);
    return _mm_add_epi16 (a, pad);
}
#endif

#if defined(USE_SSSE3)
int32x4_t vpadalq_s16(int32x4_t a, int16x8_t b);         // VPADAL.S16 q0,q0
_NEON2SSE_INLINE int32x4_t vpadalq_s16(int32x4_t a, int16x8_t b)         // VPADAL.S16 q0,q0
{
    int32x4_t pad;
    pad = vpaddlq_s16(b);
    return _mm_add_epi32(a, pad);
}
#endif

int64x2_t vpadalq_s32(int64x2_t a, int32x4_t b);         // VPADAL.S32 q0,q0
_NEON2SSE_INLINE int64x2_t vpadalq_s32(int64x2_t a, int32x4_t b)
{
    int64x2_t pad;
    pad = vpaddlq_s32(b);
    return _mm_add_epi64 (a, pad);
}

#if defined(USE_SSSE3)
uint16x8_t vpadalq_u8(uint16x8_t a, uint8x16_t b);         // VPADAL.U8 q0,q0
_NEON2SSE_INLINE uint16x8_t vpadalq_u8(uint16x8_t a, uint8x16_t b)         // VPADAL.U8 q0,q0
{
    uint16x8_t pad;
    pad = vpaddlq_u8(b);
    return _mm_add_epi16 (a, pad);
}
#endif

uint32x4_t vpadalq_u16(uint32x4_t a, uint16x8_t b);         // VPADAL.s16 q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint32x4_t vpadalq_u16(uint32x4_t a, uint16x8_t b), _NEON2SSE_REASON_SLOW_SERIAL)
{
    uint32x4_t pad;
    pad = vpaddlq_u16(b);
    return _mm_add_epi32(a, pad);
}         //no optimal SIMD solution, serial is faster

uint64x2_t vpadalq_u32(uint64x2_t a, uint32x4_t b);         // VPADAL.U32 q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint64x2_t vpadalq_u32(uint64x2_t a, uint32x4_t b), _NEON2SSE_REASON_SLOW_SERIAL)
{         //no optimal SIMD solution, serial is faster
    uint64x2_t pad;
    pad = vpaddlq_u32(b);
    return _mm_add_epi64(a, pad);
}         //no optimal SIMD solution, serial is faster

//**********  Folding maximum   *************************************
//*******************************************************************
//VPMAX (Vector Pairwise Maximum) compares adjacent pairs of elements in two vectors,
//and copies the larger of each pair into the corresponding element in the destination
//    no corresponding functionality in IA32 SIMD, so we need to do the vertical comparison

// ***************** Folding minimum  ****************************
// **************************************************************
//vpmin -> takes minimum of adjacent pairs

//***************************************************************
//***********  Reciprocal/Sqrt ************************************
//***************************************************************
//****************** Reciprocal estimate *******************************

//the ARM NEON and x86 SIMD results may be slightly different

float32x4_t vrecpeq_f32(float32x4_t a);         // VRECPE.F32 q0,q0
//the ARM NEON and x86 SIMD results may be slightly different
#define vrecpeq_f32 _mm_rcp_ps

uint32x4_t vrecpeq_u32(uint32x4_t a);         // VRECPE.U32 q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint32x4_t vrecpeq_u32(uint32x4_t a), _NEON2SSE_REASON_SLOW_SERIAL)
{         //no reciprocal for ints in IA32 available, neither for  unsigned int to float 4 lanes conversion, so serial solution looks faster
    _NEON2SSE_ALIGN_16 uint32_t atmp[4], res[4];
    _mm_store_si128((__m128i*)atmp, a);
    res[0] = (atmp[0]) ? 1 / atmp[0] : 0xffffffff;
    res[1] = (atmp[1]) ? 1 / atmp[1] : 0xffffffff;
    return _mm_load_si128((__m128i*)res);
}

//**********Reciprocal square root estimate ****************
//**********************************************************
//no reciprocal square root for ints in IA32 available, neither for unsigned int to float4 lanes conversion, so a serial solution looks faster

float32x4_t vrsqrteq_f32(float32x4_t a);         // VRSQRTE.F32 q0,q0
//the ARM NEON and x86 SIMD results may be slightly different
#define vrsqrteq_f32 _mm_rsqrt_ps

uint32x4_t vrsqrteq_u32(uint32x4_t a);         // VRSQRTE.U32 q0,q0
#define vrsqrteq_u32(a) _mm_castps_si128(_mm_rsqrt_ps(_M128(a)) )

//************ Reciprocal estimate/step and 1/sqrt estimate/step ***************************
//******************************************************************************************
//******VRECPS (Vector Reciprocal Step) ***************************************************
//multiplies the elements of one vector by the corresponding elements of another vector,
//subtracts each of the results from 2, and places the final results into the elements of the destination vector.

float32x4_t vrecpsq_f32(float32x4_t a, float32x4_t b);         // VRECPS.F32 q0, q0, q0
_NEON2SSE_INLINE float32x4_t vrecpsq_f32(float32x4_t a, float32x4_t b)         // VRECPS.F32 q0, q0, q0
{
    __m128 f2, mul;
    f2 =  _mm_set1_ps(2.);
    mul = _mm_mul_ps(a,b);
    return _mm_sub_ps(f2,mul);
}

//*****************VRSQRTS (Vector Reciprocal Square Root Step) *****************************
//multiplies the elements of one vector by the corresponding elements of another vector,
//subtracts each of the results from 3, divides these results by two, and places the final results into the elements of the destination vector.

float32x4_t vrsqrtsq_f32(float32x4_t a, float32x4_t b);         // VRSQRTS.F32 q0, q0, q0
_NEON2SSE_INLINE float32x4_t vrsqrtsq_f32(float32x4_t a, float32x4_t b)         // VRSQRTS.F32 q0, q0, q0
{
    __m128 f3, f05, mul;
    f3 =  _mm_set1_ps(3.);
    f05 =  _mm_set1_ps(0.5);
    mul = _mm_mul_ps(a,b);
    f3 = _mm_sub_ps(f3,mul);
    return _mm_mul_ps (f3, f05);
}
//********************************************************************************************
//***************************** Shifts by signed variable ***********************************
//********************************************************************************************
//***** Vector shift left: Vr[i] := Va[i] << Vb[i] (negative values shift right) ***********************
//********************************************************************************************
//No such operations in IA32 SIMD unfortunately, constant shift only available, so need to do the serial solution
//helper macro. It matches ARM implementation for big shifts
#define SERIAL_SHIFT(TYPE, INTERNAL_TYPE, LENMAX, LEN) \
        _NEON2SSE_ALIGN_16 TYPE atmp[LENMAX], res[LENMAX]; _NEON2SSE_ALIGN_16 INTERNAL_TYPE btmp[LENMAX]; int i, lanesize = sizeof(INTERNAL_TYPE) << 3; \
        _mm_store_si128((__m128i*)atmp, a); _mm_store_si128((__m128i*)btmp, b); \
        for (i = 0; i<LEN; i++) { \
        if( (btmp[i] >= lanesize)||(btmp[i] <= -lanesize) ) res[i] = 0; \
        else res[i] = (btmp[i] >=0) ? atmp[i] << btmp[i] : atmp[i] >> (-btmp[i]); } \
        return _mm_load_si128((__m128i*)res);

int8x16_t vshlq_s8(int8x16_t a, int8x16_t b);         // VSHL.S8 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int8x16_t vshlq_s8(int8x16_t a, int8x16_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SHIFT(int8_t, int8_t, 16, 16)
}

int16x8_t vshlq_s16(int16x8_t a, int16x8_t b);         // VSHL.S16 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int16x8_t vshlq_s16(int16x8_t a, int16x8_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SHIFT(int16_t, int16_t, 8, 8)
}

int32x4_t vshlq_s32(int32x4_t a, int32x4_t b);         // VSHL.S32 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int32x4_t vshlq_s32(int32x4_t a, int32x4_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SHIFT(int32_t, int32_t, 4, 4)
}

int64x2_t vshlq_s64(int64x2_t a, int64x2_t b);         // VSHL.S64 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int64x2_t vshlq_s64(int64x2_t a, int64x2_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SHIFT(int64_t, int64_t, 2, 2)
}

uint8x16_t vshlq_u8(uint8x16_t a, int8x16_t b);         // VSHL.U8 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint8x16_t vshlq_u8(uint8x16_t a, int8x16_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SHIFT(uint8_t, int8_t, 16, 16)
}

uint16x8_t vshlq_u16(uint16x8_t a, int16x8_t b);         // VSHL.s16 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint16x8_t vshlq_u16(uint16x8_t a, int16x8_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SHIFT(uint16_t, int16_t, 8, 8)
}

uint32x4_t vshlq_u32(uint32x4_t a, int32x4_t b);         // VSHL.U32 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint32x4_t vshlq_u32(uint32x4_t a, int32x4_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SHIFT(uint32_t, int32_t, 4, 4)
}

uint64x2_t vshlq_u64(uint64x2_t a, int64x2_t b);         // VSHL.U64 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING( uint64x2_t vshlq_u64(uint64x2_t a, int64x2_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SHIFT(uint64_t, int64_t, 2, 2)
}

//*********** Vector saturating shift left: (negative values shift right) **********************
//********************************************************************************************
//No such operations in IA32 SIMD available yet, constant shift only available, so need to do the serial solution
#define SERIAL_SATURATING_SHIFT_SIGNED(TYPE, LENMAX, LEN) \
        _NEON2SSE_ALIGN_16 TYPE atmp[LENMAX], res[LENMAX], btmp[LENMAX]; TYPE limit; int i; \
        int lanesize_1 = (sizeof(TYPE) << 3) - 1; \
        _mm_store_si128((__m128i*)atmp, a); _mm_store_si128((__m128i*)btmp, b); \
        for (i = 0; i<LEN; i++) { \
        if (atmp[i] ==0) res[i] = 0; \
        else{ \
            if(btmp[i] <0) res[i] = atmp[i] >> (-btmp[i]); \
            else{ \
                if (btmp[i]>lanesize_1) { \
                    res[i] = ((_UNSIGNED_T(TYPE))atmp[i] >> lanesize_1 ) + ((TYPE)1 << lanesize_1) - 1; \
                }else{ \
                    limit = (TYPE)1 << (lanesize_1 - btmp[i]); \
                    if((atmp[i] >= limit)||(atmp[i] <= -limit)) \
                        res[i] = ((_UNSIGNED_T(TYPE))atmp[i] >> lanesize_1 ) + ((TYPE)1 << lanesize_1) - 1; \
                    else res[i] = atmp[i] << btmp[i]; }}}} \
        return _mm_load_si128((__m128i*)res);

#define SERIAL_SATURATING_SHIFT_UNSIGNED(TYPE, LENMAX, LEN) \
        _NEON2SSE_ALIGN_16 _UNSIGNED_T(TYPE) atmp[LENMAX], res[LENMAX]; _NEON2SSE_ALIGN_16 TYPE btmp[LENMAX]; _UNSIGNED_T(TYPE) limit; int i; \
        TYPE lanesize = (sizeof(TYPE) << 3); \
        _mm_store_si128((__m128i*)atmp, a); _mm_store_si128((__m128i*)btmp, b); \
        for (i = 0; i<LEN; i++) { \
        if (atmp[i] ==0) {res[i] = 0; \
        }else{ \
            if(btmp[i] < 0) res[i] = atmp[i] >> (-btmp[i]); \
            else{ \
                if (btmp[i]>lanesize) res[i] = ~((TYPE)0); \
                else{ \
                    limit = (TYPE) 1 << (lanesize - btmp[i]); \
                    res[i] = ( atmp[i] >= limit) ? res[i] = ~((TYPE)0) : atmp[i] << btmp[i]; }}}} \
        return _mm_load_si128((__m128i*)res);

int8x16_t vqshlq_s8(int8x16_t a, int8x16_t b);         // VQSHL.S8 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int8x16_t vqshlq_s8(int8x16_t a, int8x16_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_SHIFT_SIGNED(int8_t, 16, 16)
}

int16x8_t vqshlq_s16(int16x8_t a, int16x8_t b);         // VQSHL.S16 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int16x8_t vqshlq_s16(int16x8_t a, int16x8_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_SHIFT_SIGNED(int16_t, 8, 8)
}

int32x4_t vqshlq_s32(int32x4_t a, int32x4_t b);         // VQSHL.S32 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int32x4_t vqshlq_s32(int32x4_t a, int32x4_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_SHIFT_SIGNED(int32_t, 4, 4)
}

int64x2_t vqshlq_s64(int64x2_t a, int64x2_t b);         // VQSHL.S64 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int64x2_t vqshlq_s64(int64x2_t a, int64x2_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_SHIFT_SIGNED(int64_t, 2, 2)
}

uint8x16_t vqshlq_u8(uint8x16_t a, int8x16_t b);         // VQSHL.U8 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint8x16_t vqshlq_u8(uint8x16_t a, int8x16_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_SHIFT_UNSIGNED(int8_t, 16, 16)
}

uint16x8_t vqshlq_u16(uint16x8_t a, int16x8_t b);         // VQSHL.s16 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint16x8_t vqshlq_u16(uint16x8_t a, int16x8_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_SHIFT_UNSIGNED(int16_t, 8, 8)
}

uint32x4_t vqshlq_u32(uint32x4_t a, int32x4_t b);         // VQSHL.U32 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint32x4_t vqshlq_u32(uint32x4_t a, int32x4_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_SHIFT_UNSIGNED(int32_t, 4, 4)
}

uint64x2_t vqshlq_u64(uint64x2_t a, int64x2_t b);         // VQSHL.U64 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint64x2_t vqshlq_u64(uint64x2_t a, int64x2_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_SHIFT_UNSIGNED(int64_t, 2, 2)
}

//******** Vector rounding shift left: (negative values shift right) **********
//****************************************************************************
//No such operations in IA32 SIMD available yet, constant shift only available, so need to do the serial solution
//rounding makes sense for right shifts only.
#define SERIAL_ROUNDING_SHIFT(TYPE, INTERNAL_TYPE, LENMAX, LEN) \
        _NEON2SSE_ALIGN_16 TYPE atmp[LENMAX], res[LENMAX]; _NEON2SSE_ALIGN_16 INTERNAL_TYPE btmp[LENMAX]; INTERNAL_TYPE i, lanesize = sizeof(INTERNAL_TYPE) << 3; \
        _mm_store_si128((__m128i*)atmp, a); _mm_store_si128((__m128i*)btmp, b); \
        for (i = 0; i<LEN; i++) { \
        if( btmp[i] >= 0) { \
            if(btmp[i] >= lanesize) res[i] = 0; \
            else res[i] = (atmp[i] << btmp[i]); \
        }else{ \
            res[i] = (btmp[i] < -lanesize) ? res[i] = 0 : \
                            (btmp[i] == -lanesize) ? (atmp[i] & ((INTERNAL_TYPE)1 << (-btmp[i] - 1))) >> (-btmp[i] - 1) : \
                            (atmp[i] >> (-btmp[i])) + ( (atmp[i] & ((INTERNAL_TYPE)1 << (-btmp[i] - 1))) >> (-btmp[i] - 1) );    }} \
        return _mm_load_si128((__m128i*)res);

int8x16_t vrshlq_s8(int8x16_t a, int8x16_t b);         // VRSHL.S8 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int8x16_t vrshlq_s8(int8x16_t a, int8x16_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_ROUNDING_SHIFT(int8_t, int8_t, 16, 16)
}

int16x8_t vrshlq_s16(int16x8_t a, int16x8_t b);         // VRSHL.S16 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int16x8_t vrshlq_s16(int16x8_t a, int16x8_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_ROUNDING_SHIFT(int16_t, int16_t, 8, 8)
}

int32x4_t vrshlq_s32(int32x4_t a, int32x4_t b);         // VRSHL.S32 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int32x4_t vrshlq_s32(int32x4_t a, int32x4_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_ROUNDING_SHIFT(int32_t, int32_t, 4, 4)
}

int64x2_t vrshlq_s64(int64x2_t a, int64x2_t b);         // VRSHL.S64 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int64x2_t vrshlq_s64(int64x2_t a, int64x2_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_ROUNDING_SHIFT(int64_t, int64_t, 2, 2)
}

uint8x16_t vrshlq_u8(uint8x16_t a, int8x16_t b);         // VRSHL.U8 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint8x16_t vrshlq_u8(uint8x16_t a, int8x16_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_ROUNDING_SHIFT(uint8_t, int8_t, 16, 16)
}

uint16x8_t vrshlq_u16(uint16x8_t a, int16x8_t b);         // VRSHL.s16 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint16x8_t vrshlq_u16(uint16x8_t a, int16x8_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_ROUNDING_SHIFT(uint16_t, int16_t, 8, 8)
}

uint32x4_t vrshlq_u32(uint32x4_t a, int32x4_t b);         // VRSHL.U32 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint32x4_t vrshlq_u32(uint32x4_t a, int32x4_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_ROUNDING_SHIFT(uint32_t, int32_t, 4, 4)
}

uint64x2_t vrshlq_u64(uint64x2_t a, int64x2_t b);         // VRSHL.U64 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint64x2_t vrshlq_u64(uint64x2_t a, int64x2_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_ROUNDING_SHIFT(uint64_t, int64_t, 2, 2)
}

//********** Vector saturating rounding shift left: (negative values shift right) ****************
//*************************************************************************************************
//No such operations in IA32 SIMD unfortunately, constant shift only available, so need to do the serial solution
//Saturation happens for left shifts only while rounding makes sense for right shifts only.
#define SERIAL_SATURATING_ROUNDING_SHIFT_SIGNED(TYPE, LENMAX, LEN) \
        _NEON2SSE_ALIGN_16 TYPE atmp[LENMAX], res[LENMAX], btmp[LENMAX]; TYPE limit; int i; \
        int lanesize_1 = (sizeof(TYPE) << 3) - 1; \
        _mm_store_si128((__m128i*)atmp, a); _mm_store_si128((__m128i*)btmp, b); \
        for (i = 0; i<LEN; i++) { \
        if (atmp[i] ==0) res[i] = 0; \
        else{ \
            if(btmp[i] <0) res[i] = (btmp[i] < (-lanesize_1)) ? 0 : (atmp[i] >> (-btmp[i])) + ( (atmp[i] & ((TYPE)1 << (-btmp[i] - 1))) >> (-btmp[i] - 1) ); \
            else{ \
                if (btmp[i]>lanesize_1) { \
                    res[i] = ((_UNSIGNED_T(TYPE))atmp[i] >> lanesize_1 ) + ((TYPE)1 << lanesize_1) - 1; \
                }else{ \
                    limit = (TYPE)1 << (lanesize_1 - btmp[i]); \
                    if((atmp[i] >= limit)||(atmp[i] <= -limit)) \
                        res[i] = ((_UNSIGNED_T(TYPE))atmp[i] >> lanesize_1 ) + ((TYPE)1 << lanesize_1) - 1; \
                    else res[i] = atmp[i] << btmp[i]; }}}} \
        return _mm_load_si128((__m128i*)res);

#define SERIAL_SATURATING_ROUNDING_SHIFT_UNSIGNED(TYPE, LENMAX, LEN) \
        _NEON2SSE_ALIGN_16 _UNSIGNED_T(TYPE) atmp[LENMAX], res[LENMAX]; _NEON2SSE_ALIGN_16 TYPE btmp[LENMAX]; _UNSIGNED_T(TYPE) limit; int i; \
        int lanesize = (sizeof(TYPE) << 3); \
        _mm_store_si128((__m128i*)atmp, a); _mm_store_si128((__m128i*)btmp, b); \
        for (i = 0; i<LEN; i++) { \
        if (atmp[i] ==0) {res[i] = 0; \
        }else{ \
            if(btmp[i] < 0) res[i] = (btmp[i] < (-lanesize)) ? 0 : (atmp[i] >> (-btmp[i])) + ( (atmp[i] & ((TYPE)1 << (-btmp[i] - 1))) >> (-btmp[i] - 1) ); \
            else{ \
                if (btmp[i]>lanesize) res[i] = ~((TYPE)0); \
                else{ \
                    limit = (TYPE) 1 << (lanesize - btmp[i]); \
                    res[i] = ( atmp[i] >= limit) ? res[i] = ~((TYPE)0) : atmp[i] << btmp[i]; }}}} \
        return _mm_load_si128((__m128i*)res);

int8x16_t vqrshlq_s8(int8x16_t a, int8x16_t b);         // VQRSHL.S8 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int8x16_t vqrshlq_s8(int8x16_t a, int8x16_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_ROUNDING_SHIFT_SIGNED(int8_t, 16, 16)
}

int16x8_t vqrshlq_s16(int16x8_t a, int16x8_t b);         // VQRSHL.S16 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int16x8_t vqrshlq_s16(int16x8_t a, int16x8_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_ROUNDING_SHIFT_SIGNED(int16_t, 8, 8)
}

int32x4_t vqrshlq_s32(int32x4_t a, int32x4_t b);         // VQRSHL.S32 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int32x4_t vqrshlq_s32(int32x4_t a, int32x4_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_ROUNDING_SHIFT_SIGNED(int32_t, 4, 4)
}

int64x2_t vqrshlq_s64(int64x2_t a, int64x2_t b);         // VQRSHL.S64 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int64x2_t vqrshlq_s64(int64x2_t a, int64x2_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_ROUNDING_SHIFT_SIGNED(int64_t, 2, 2)
}

uint8x16_t vqrshlq_u8(uint8x16_t a, int8x16_t b);         // VQRSHL.U8 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint8x16_t vqrshlq_u8(uint8x16_t a, int8x16_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_ROUNDING_SHIFT_UNSIGNED(int8_t, 16, 16)
}

uint16x8_t vqrshlq_u16(uint16x8_t a, int16x8_t b);         // VQRSHL.s16 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint16x8_t vqrshlq_u16(uint16x8_t a, int16x8_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_ROUNDING_SHIFT_UNSIGNED(int16_t, 8, 8)
}

uint32x4_t vqrshlq_u32(uint32x4_t a, int32x4_t b);         // VQRSHL.U32 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint32x4_t vqrshlq_u32(uint32x4_t a, int32x4_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_ROUNDING_SHIFT_UNSIGNED(int32_t, 4, 4)
}

uint64x2_t vqrshlq_u64(uint64x2_t a, int64x2_t b);         // VQRSHL.U64 q0,q0,q0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint64x2_t vqrshlq_u64(uint64x2_t a, int64x2_t b),  _NEON2SSE_REASON_SLOW_SERIAL)
{
    SERIAL_SATURATING_ROUNDING_SHIFT_UNSIGNED(int64_t, 2, 2)
}

// *********************************************************************************
// *****************************  Shifts by a constant *****************************
// *********************************************************************************
//**************** Vector shift right by constant*************************************
//************************************************************************************

int8x16_t vshrq_n_s8(int8x16_t a, __constrange(1,8) int b);         // VSHR.S8 q0,q0,#8
_NEON2SSE_INLINE int8x16_t vshrq_n_s8(int8x16_t a, __constrange(1,8) int b)         // VSHR.S8 q0,q0,#8
{         //no 8 bit shift available, go to 16 bit trick
    __m128i zero, mask0, a_sign, r, a_sign_mask;
    _NEON2SSE_ALIGN_16 int16_t mask0_16[9] = {0x0000, 0x0080, 0x00c0, 0x00e0, 0x00f0,  0x00f8, 0x00fc, 0x00fe, 0x00ff};
    zero = _mm_setzero_si128();
    mask0 = _mm_set1_epi16(mask0_16[b]);         //to mask the bits to be "spoiled"  by 16 bit shift
    a_sign =  _mm_cmpgt_epi8 (zero, a);         //ff if a<0 or zero if a>0
    r = _mm_srai_epi16 (a, b);
    a_sign_mask =  _mm_and_si128 (mask0, a_sign);
    r =  _mm_andnot_si128 (mask0, r);
    return _mm_or_si128 (r, a_sign_mask);
}

int16x8_t vshrq_n_s16(int16x8_t a, __constrange(1,16) int b);         // VSHR.S16 q0,q0,#16
#define vshrq_n_s16 _mm_srai_epi16

int32x4_t vshrq_n_s32(int32x4_t a, __constrange(1,32) int b);         // VSHR.S32 q0,q0,#32
#define vshrq_n_s32 _mm_srai_epi32

int64x2_t vshrq_n_s64(int64x2_t a, __constrange(1,64) int b);         // VSHR.S64 q0,q0,#64
_NEON2SSE_INLINE int64x2_t vshrq_n_s64(int64x2_t a, __constrange(1,64) int b)
{         //SIMD implementation may be not optimal due to 64 bit arithmetic shift absense in x86 SIMD
    __m128i c1, signmask,a0,  res64;
    _NEON2SSE_ALIGN_16 uint64_t mask[] = {0x8000000000000000, 0x8000000000000000};
    c1 =  _mm_cmpeq_epi32(a,a);         //0xffffffffffffffff
    signmask  =  _mm_slli_epi64 (c1, (64 - b));
    a0 = _mm_or_si128(a, *(__m128i*)mask);         //get the first bit
    #ifdef USE_SSE4
        a0 = _mm_cmpeq_epi64 (a, a0);         //SSE4.1
    #else
        a0 = _mm_cmpeq_epi32 (a, a0);
        a0 = _mm_shuffle_epi32 (a0, 1 | (1 << 2) | (3 << 4) | (3 << 6));         //copy the information from hi to low part of the 64 bit data
    #endif
    signmask = _mm_and_si128(a0, signmask);
    res64 = _mm_srli_epi64 (a, b);
    return _mm_or_si128(res64, signmask);
}

uint8x16_t vshrq_n_u8(uint8x16_t a, __constrange(1,8) int b);         // VSHR.U8 q0,q0,#8
_NEON2SSE_INLINE uint8x16_t vshrq_n_u8(uint8x16_t a, __constrange(1,8) int b)         // VSHR.U8 q0,q0,#8
{         //no 8 bit shift available, need the special trick
    __m128i mask0, r;
    _NEON2SSE_ALIGN_16 uint16_t mask10_16[9] = {0xffff, 0xff7f, 0xff3f, 0xff1f, 0xff0f,  0xff07, 0xff03, 0xff01, 0xff00};
    mask0 = _mm_set1_epi16(mask10_16[b]);         //to mask the bits to be "spoiled"  by 16 bit shift
    r = _mm_srli_epi16 ( a, b);
    return _mm_and_si128 (r,  mask0);
}

uint16x8_t vshrq_n_u16(uint16x8_t a, __constrange(1,16) int b);         // VSHR.s16 q0,q0,#16
#define vshrq_n_u16 _mm_srli_epi16

uint32x4_t vshrq_n_u32(uint32x4_t a, __constrange(1,32) int b);         // VSHR.U32 q0,q0,#32
#define vshrq_n_u32 _mm_srli_epi32

uint64x2_t vshrq_n_u64(uint64x2_t a, __constrange(1,64) int b);         // VSHR.U64 q0,q0,#64
#define vshrq_n_u64 _mm_srli_epi64

//*************************** Vector shift left by constant *************************
//*********************************************************************************

int8x16_t vshlq_n_s8(int8x16_t a, __constrange(0,7) int b);         // VSHL.I8 q0,q0,#0
#define vshlq_n_s8 vshlq_n_u8

int16x8_t vshlq_n_s16(int16x8_t a, __constrange(0,15) int b);         // VSHL.I16 q0,q0,#0
#define vshlq_n_s16 _mm_slli_epi16

int32x4_t vshlq_n_s32(int32x4_t a, __constrange(0,31) int b);         // VSHL.I32 q0,q0,#0
#define vshlq_n_s32 _mm_slli_epi32

int64x2_t vshlq_n_s64(int64x2_t a, __constrange(0,63) int b);         // VSHL.I64 q0,q0,#0
#define vshlq_n_s64 _mm_slli_epi64

uint8x16_t vshlq_n_u8(uint8x16_t a, __constrange(0,7) int b);         // VSHL.I8 q0,q0,#0
_NEON2SSE_INLINE uint8x16_t vshlq_n_u8(uint8x16_t a, __constrange(0,7) int b)
{         //no 8 bit shift available, need the special trick
    __m128i mask0, r;
    _NEON2SSE_ALIGN_16 uint16_t mask10_16[9] = {0xffff, 0xfeff, 0xfcff, 0xf8ff, 0xf0ff,  0xe0ff, 0xc0ff, 0x80ff, 0xff};
    mask0 = _mm_set1_epi16(mask10_16[b]);         //to mask the bits to be "spoiled"  by 16 bit shift
    r = _mm_slli_epi16 ( a, b);
    return _mm_and_si128 (r,  mask0);
}

uint16x8_t vshlq_n_u16(uint16x8_t a, __constrange(0,15) int b);         // VSHL.I16 q0,q0,#0
#define vshlq_n_u16 vshlq_n_s16

uint32x4_t vshlq_n_u32(uint32x4_t a, __constrange(0,31) int b);         // VSHL.I32 q0,q0,#0
#define vshlq_n_u32 vshlq_n_s32

uint64x2_t vshlq_n_u64(uint64x2_t a, __constrange(0,63) int b);         // VSHL.I64 q0,q0,#0
#define vshlq_n_u64 vshlq_n_s64

//************* Vector rounding shift right by constant ******************
//*************************************************************************
//No corresponding  x86 intrinsics exist, need to do some tricks

int8x16_t vrshrq_n_s8(int8x16_t a, __constrange(1,8) int b);         // VRSHR.S8 q0,q0,#8
_NEON2SSE_INLINE int8x16_t vrshrq_n_s8(int8x16_t a, __constrange(1,8) int b)         // VRSHR.S8 q0,q0,#8
{         //no 8 bit shift available, go to 16 bit trick
    __m128i r, mask1, maskb;
    _NEON2SSE_ALIGN_16 uint16_t mask2b[9] = {0x0000, 0x0101, 0x0202, 0x0404, 0x0808, 0x1010, 0x2020, 0x4040, 0x8080};         // 2^b-th bit set to 1
    r = vshrq_n_s8 (a, b);
    mask1 = _mm_set1_epi16(mask2b[b]);         // 2^b-th bit set to 1 for 16bit, need it for rounding
    maskb = _mm_and_si128(a, mask1);         //get b or 0 for rounding
    maskb =  _mm_srli_epi16 (maskb, b - 1);         // to add 1
    return _mm_add_epi8(r, maskb);         //actual rounding
}

int16x8_t vrshrq_n_s16(int16x8_t a, __constrange(1,16) int b);         // VRSHR.S16 q0,q0,#16
_NEON2SSE_INLINE int16x8_t vrshrq_n_s16(int16x8_t a, __constrange(1,16) int b)         // VRSHR.S16 q0,q0,#16
{
    __m128i maskb, r;
    maskb =  _mm_slli_epi16(a, (16 - b));         //to get rounding (b-1)th bit
    maskb = _mm_srli_epi16(maskb, 15);         //1 or 0
    r = _mm_srai_epi16 (a, b);
    return _mm_add_epi16 (r, maskb);         //actual rounding
}

int32x4_t vrshrq_n_s32(int32x4_t a, __constrange(1,32) int b);         // VRSHR.S32 q0,q0,#32
_NEON2SSE_INLINE int32x4_t vrshrq_n_s32(int32x4_t a, __constrange(1,32) int b)         // VRSHR.S32 q0,q0,#32
{
    __m128i maskb,  r;
    maskb = _mm_slli_epi32 (a, (32 - b));         //to get rounding (b-1)th bit
    maskb = _mm_srli_epi32 (maskb,31);         //1 or 0
    r = _mm_srai_epi32(a, b);
    return _mm_add_epi32 (r, maskb);         //actual rounding
}

int64x2_t vrshrq_n_s64(int64x2_t a, __constrange(1,64) int b);         // VRSHR.S64 q0,q0,#64
_NEON2SSE_INLINE int64x2_t vrshrq_n_s64(int64x2_t a, __constrange(1,64) int b)
{         //solution may be not optimal compared with a serial one
    __m128i maskb;
    int64x2_t r;
    maskb = _mm_slli_epi64 (a, (64 - b));         //to get rounding (b-1)th bit
    maskb = _mm_srli_epi64 (maskb,63);         //1 or 0
    r = vshrq_n_s64(a, b);
    return _mm_add_epi64 (r, maskb);         //actual rounding
}

uint8x16_t vrshrq_n_u8(uint8x16_t a, __constrange(1,8) int b);         // VRSHR.U8 q0,q0,#8
_NEON2SSE_INLINE uint8x16_t vrshrq_n_u8(uint8x16_t a, __constrange(1,8) int b)         // VRSHR.U8 q0,q0,#8
{         //no 8 bit shift available, go to 16 bit trick
    __m128i r, mask1, maskb;
    _NEON2SSE_ALIGN_16 uint16_t mask2b[9] = {0x0000, 0x0101, 0x0202, 0x0404, 0x0808, 0x1010, 0x2020, 0x4040, 0x8080};         // 2^b-th bit set to 1
    r = vshrq_n_u8 (a, b);
    mask1 = _mm_set1_epi16(mask2b[b]);         // 2^b-th bit set to 1 for 16bit, need it for rounding
    maskb = _mm_and_si128(a, mask1);         //get b or 0 for rounding
    maskb =  _mm_srli_epi16 (maskb, b - 1);         // to add 1
    return _mm_add_epi8(r, maskb);         //actual rounding
}

uint16x8_t vrshrq_n_u16(uint16x8_t a, __constrange(1,16) int b);         // VRSHR.s16 q0,q0,#16
_NEON2SSE_INLINE uint16x8_t vrshrq_n_u16(uint16x8_t a, __constrange(1,16) int b)         // VRSHR.S16 q0,q0,#16
{
    __m128i maskb, r;
    maskb =  _mm_slli_epi16(a, (16 - b));         //to get rounding (b-1)th bit
    maskb = _mm_srli_epi16(maskb, 15);         //1 or 0
    r = _mm_srli_epi16 (a, b);
    return _mm_add_epi16 (r, maskb);         //actual rounding
}

uint32x4_t vrshrq_n_u32(uint32x4_t a, __constrange(1,32) int b);         // VRSHR.U32 q0,q0,#32
_NEON2SSE_INLINE uint32x4_t vrshrq_n_u32(uint32x4_t a, __constrange(1,32) int b)         // VRSHR.S32 q0,q0,#32
{
    __m128i maskb,  r;
    maskb = _mm_slli_epi32 (a, (32 - b));         //to get rounding (b-1)th bit
    maskb = _mm_srli_epi32 (maskb,31);         //1 or 0
    r = _mm_srli_epi32(a, b);
    return _mm_add_epi32 (r, maskb);         //actual rounding
}

uint64x2_t vrshrq_n_u64(uint64x2_t a, __constrange(1,64) int b);         // VRSHR.U64 q0,q0,#64
_NEON2SSE_INLINE uint64x2_t vrshrq_n_u64(uint64x2_t a, __constrange(1,64) int b)
{         //solution may be not optimal compared with a serial one
    __m128i maskb,  r;
    maskb = _mm_slli_epi64 (a, (64 - b));         //to get rounding (b-1)th bit
    maskb = _mm_srli_epi64 (maskb,63);         //1 or 0
    r = _mm_srli_epi64(a, b);
    return _mm_add_epi64 (r, maskb);         //actual rounding
}

//************* Vector shift right by constant and accumulate *********
//*********************************************************************

int8x16_t vsraq_n_s8(int8x16_t a, int8x16_t b, __constrange(1,8) int c);         // VSRA.S8 q0,q0,#8
_NEON2SSE_INLINE int8x16_t vsraq_n_s8(int8x16_t a, int8x16_t b, __constrange(1,8) int c)         // VSRA.S8 q0,q0,#8
{
    int8x16_t shift;
    shift = vshrq_n_s8(b, c);
    return vaddq_s8(a, shift);
}

int16x8_t vsraq_n_s16(int16x8_t a, int16x8_t b, __constrange(1,16) int c);         // VSRA.S16 q0,q0,#16
_NEON2SSE_INLINE int16x8_t vsraq_n_s16(int16x8_t a, int16x8_t b, __constrange(1,16) int c)         // VSRA.S16 q0,q0,#16
{
    int16x8_t shift;
    shift = vshrq_n_s16(b, c);
    return vaddq_s16(a, shift);
}

int32x4_t vsraq_n_s32(int32x4_t a, int32x4_t b, __constrange(1,32) int c);         // VSRA.S32 q0,q0,#32
_NEON2SSE_INLINE int32x4_t vsraq_n_s32(int32x4_t a, int32x4_t b, __constrange(1,32) int c)         // VSRA.S32 q0,q0,#32
{
    int32x4_t shift;
    shift = vshrq_n_s32(b, c);
    return vaddq_s32(a, shift);
}

int64x2_t vsraq_n_s64(int64x2_t a, int64x2_t b, __constrange(1,64) int c);         // VSRA.S64 q0,q0,#64
_NEON2SSE_INLINE int64x2_t vsraq_n_s64(int64x2_t a, int64x2_t b, __constrange(1,64) int c)         // VSRA.S64 q0,q0,#64
{
    int64x2_t shift;
    shift = vshrq_n_s64(b, c);
    return vaddq_s64( a, shift);
}

uint8x16_t vsraq_n_u8(uint8x16_t a, uint8x16_t b, __constrange(1,8) int c);         // VSRA.U8 q0,q0,#8
_NEON2SSE_INLINE uint8x16_t vsraq_n_u8(uint8x16_t a, uint8x16_t b, __constrange(1,8) int c)         // VSRA.U8 q0,q0,#8
{
    uint8x16_t shift;
    shift = vshrq_n_u8(b, c);
    return vaddq_u8(a, shift);
}

uint16x8_t vsraq_n_u16(uint16x8_t a, uint16x8_t b, __constrange(1,16) int c);         // VSRA.s16 q0,q0,#16
_NEON2SSE_INLINE uint16x8_t vsraq_n_u16(uint16x8_t a, uint16x8_t b, __constrange(1,16) int c)         // VSRA.s16 q0,q0,#16
{
    uint16x8_t shift;
    shift = vshrq_n_u16(b, c);
    return vaddq_u16(a,  shift);
}

uint32x4_t vsraq_n_u32(uint32x4_t a, uint32x4_t b, __constrange(1,32) int c);         // VSRA.U32 q0,q0,#32
_NEON2SSE_INLINE uint32x4_t vsraq_n_u32(uint32x4_t a, uint32x4_t b, __constrange(1,32) int c)         // VSRA.U32 q0,q0,#32
{
    uint32x4_t shift;
    shift = vshrq_n_u32(b, c);
    return vaddq_u32(a, shift);
}

uint64x2_t vsraq_n_u64(uint64x2_t a, uint64x2_t b, __constrange(1,64) int c);         // VSRA.U64 q0,q0,#64
_NEON2SSE_INLINE uint64x2_t vsraq_n_u64(uint64x2_t a, uint64x2_t b, __constrange(1,64) int c)         // VSRA.U64 q0,q0,#64
{
    uint64x2_t shift;
    shift = vshrq_n_u64(b, c);
    return vaddq_u64(a, shift);
}

//************* Vector rounding shift right by constant and accumulate ****************************
//************************************************************************************************

int8x16_t vrsraq_n_s8(int8x16_t a, int8x16_t b, __constrange(1,8) int c);         // VRSRA.S8 q0,q0,#8
_NEON2SSE_INLINE int8x16_t vrsraq_n_s8(int8x16_t a, int8x16_t b, __constrange(1,8) int c)         // VRSRA.S8 q0,q0,#8
{
    int8x16_t shift;
    shift = vrshrq_n_s8(b, c);
    return vaddq_s8(a, shift);
}

int16x8_t vrsraq_n_s16(int16x8_t a, int16x8_t b, __constrange(1,16) int c);         // VRSRA.S16 q0,q0,#16
_NEON2SSE_INLINE int16x8_t vrsraq_n_s16(int16x8_t a, int16x8_t b, __constrange(1,16) int c)         // VRSRA.S16 q0,q0,#16
{
    int16x8_t shift;
    shift = vrshrq_n_s16(b, c);
    return vaddq_s16(a, shift);
}

int32x4_t vrsraq_n_s32(int32x4_t a, int32x4_t b, __constrange(1,32) int c);         // VRSRA.S32 q0,q0,#32
_NEON2SSE_INLINE int32x4_t vrsraq_n_s32(int32x4_t a, int32x4_t b, __constrange(1,32) int c)         // VRSRA.S32 q0,q0,#32
{
    int32x4_t shift;
    shift = vrshrq_n_s32(b, c);
    return vaddq_s32(a, shift);
}

int64x2_t vrsraq_n_s64(int64x2_t a, int64x2_t b, __constrange(1,64) int c);         // VRSRA.S64 q0,q0,#64
_NEON2SSE_INLINE int64x2_t vrsraq_n_s64(int64x2_t a, int64x2_t b, __constrange(1,64) int c)
{
    int64x2_t shift;
    shift = vrshrq_n_s64(b, c);
    return vaddq_s64(a, shift);
}

uint8x16_t vrsraq_n_u8(uint8x16_t a, uint8x16_t b, __constrange(1,8) int c);         // VRSRA.U8 q0,q0,#8
_NEON2SSE_INLINE uint8x16_t vrsraq_n_u8(uint8x16_t a, uint8x16_t b, __constrange(1,8) int c)         // VRSRA.U8 q0,q0,#8
{
    uint8x16_t shift;
    shift = vrshrq_n_u8(b, c);
    return vaddq_u8(a, shift);
}

uint16x8_t vrsraq_n_u16(uint16x8_t a, uint16x8_t b, __constrange(1,16) int c);         // VRSRA.s16 q0,q0,#16
_NEON2SSE_INLINE uint16x8_t vrsraq_n_u16(uint16x8_t a, uint16x8_t b, __constrange(1,16) int c)         // VRSRA.s16 q0,q0,#16
{
    uint16x8_t shift;
    shift = vrshrq_n_u16(b, c);
    return vaddq_u16(a,  shift);
}

uint32x4_t vrsraq_n_u32(uint32x4_t a, uint32x4_t b, __constrange(1,32) int c);         // VRSRA.U32 q0,q0,#32
_NEON2SSE_INLINE uint32x4_t vrsraq_n_u32(uint32x4_t a, uint32x4_t b, __constrange(1,32) int c)         // VRSRA.U32 q0,q0,#32
{
    uint32x4_t shift;
    shift = vrshrq_n_u32(b, c);
    return vaddq_u32(a, shift);
}

uint64x2_t vrsraq_n_u64(uint64x2_t a, uint64x2_t b, __constrange(1,64) int c);         // VRSRA.U64 q0,q0,#64
_NEON2SSE_INLINE uint64x2_t vrsraq_n_u64(uint64x2_t a, uint64x2_t b, __constrange(1,64) int c)
{
    uint64x2_t shift;
    shift = vrshrq_n_u64(b, c);
    return vaddq_u64(a, shift);
}

//**********************Vector saturating shift left by constant *****************************
//********************************************************************************************
//we don't check const ranges  assuming they are met

int8x16_t vqshlq_n_s8(int8x16_t a, __constrange(0,7) int b);         // VQSHL.S8 q0,q0,#0
_NEON2SSE_INLINE int8x16_t vqshlq_n_s8(int8x16_t a, __constrange(0,7) int b)         // VQSHL.S8 q0,q0,#0
{         // go to 16 bit to get the auto saturation (in packs function)
    __m128i a128, r128_1, r128_2;
    a128 = _MM_CVTEPI8_EPI16 (a);         //SSE 4.1
    r128_1 = _mm_slli_epi16 (a128, b);
    //swap hi and low part of a128 to process the remaining data
    a128 = _mm_shuffle_epi32 (a, _SWAP_HI_LOW32);
    a128 = _MM_CVTEPI8_EPI16 (a128);
    r128_2 = _mm_slli_epi16 (a128, b);
    return _mm_packs_epi16 (r128_1, r128_2);         //saturated s8
}

int16x8_t vqshlq_n_s16(int16x8_t a, __constrange(0,15) int b);         // VQSHL.S16 q0,q0,#0
_NEON2SSE_INLINE int16x8_t vqshlq_n_s16(int16x8_t a, __constrange(0,15) int b)         // VQSHL.S16 q0,q0,#0
{         // manual saturation solution looks LESS optimal than 32 bits conversion one
      // go to 32 bit to get the auto saturation (in packs function)
    __m128i a128, r128_1, r128_2;
    a128 = _MM_CVTEPI16_EPI32 (a);         //SSE 4.1
    r128_1 = _mm_slli_epi32 (a128, b);         //shift_res
    //swap hi and low part of a128 to process the remaining data
    a128 = _mm_shuffle_epi32 (a, _SWAP_HI_LOW32);
    a128 = _MM_CVTEPI16_EPI32 (a128);
    r128_2 = _mm_slli_epi32 (a128, b);
    return _mm_packs_epi32 (r128_1, r128_2);         //saturated s16
}

int32x4_t vqshlq_n_s32(int32x4_t a, __constrange(0,31) int b);         // VQSHL.S32 q0,q0,#0
_NEON2SSE_INLINE int32x4_t vqshlq_n_s32(int32x4_t a, __constrange(0,31) int b)         // VQSHL.S32 q0,q0,#0
{         // no 64 bit saturation option available, special tricks necessary
    __m128i c1, maskA, saturation_mask, c7ffffff_mask, shift_res, shift_res_mask;
    c1 = _mm_cmpeq_epi32(a,a);         //0xff..ff
    maskA = _mm_srli_epi32(c1, b + 1);         //mask for positive numbers (32-b+1) zeros and b-1 ones
    saturation_mask = _mm_cmpgt_epi32 (a, maskA);         //0xff...ff if we need saturation, 0  otherwise
    c7ffffff_mask  = _mm_srli_epi32(saturation_mask, 1);         //saturated to 0x7f..ff when needed and zeros if not
    shift_res = _mm_slli_epi32 (a, b);
    shift_res_mask = _mm_andnot_si128(saturation_mask, shift_res);
    //result with positive numbers saturated
    shift_res = _mm_or_si128 (c7ffffff_mask, shift_res_mask);
    //treat negative numbers
    maskA = _mm_slli_epi32(c1, 31 - b);         //mask for negative numbers b-1 ones  and (32-b+1)  zeros
    saturation_mask = _mm_cmpgt_epi32 (maskA,a);         //0xff...ff if we need saturation, 0  otherwise
    c7ffffff_mask  = _mm_slli_epi32(saturation_mask, 31);         //saturated to 0x80..00 when needed and zeros if not
    shift_res_mask = _mm_andnot_si128(saturation_mask, shift_res);
    return _mm_or_si128 (c7ffffff_mask, shift_res_mask);
}

int64x2_t vqshlq_n_s64(int64x2_t a, __constrange(0,63) int b);         // VQSHL.S64 q0,q0,#0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int64x2_t vqshlq_n_s64(int64x2_t a, __constrange(0,63) int b), _NEON2SSE_REASON_SLOW_SERIAL)
{         // no effective SIMD solution here
    _NEON2SSE_ALIGN_16 int64_t atmp[2], res[2];
    int64_t bmask;
    int i;
    bmask = ( int64_t)1 << (63 - b);         //positive
    _mm_store_si128((__m128i*)atmp, a);
    for (i = 0; i<2; i++) {
        if (atmp[i] >= bmask) {
            res[i] = ~(_SIGNBIT64);
        } else {
            res[i] = (atmp[i] <= -bmask) ? _SIGNBIT64 : atmp[i] << b;
        }
    }
    return _mm_load_si128((__m128i*)res);
}

uint8x16_t vqshlq_n_u8(uint8x16_t a, __constrange(0,7) int b);         // VQSHL.U8 q0,q0,#0
_NEON2SSE_INLINE uint8x16_t vqshlq_n_u8(uint8x16_t a, __constrange(0,7) int b)         // VQSHL.U8 q0,q0,#0
{         // go to 16 bit to get the auto saturation (in packs function)
    __m128i a128, r128_1, r128_2;
    a128 = _MM_CVTEPU8_EPI16 (a);         //SSE 4.1
    r128_1 = _mm_slli_epi16 (a128, b);
    //swap hi and low part of a128 to process the remaining data
    a128 = _mm_shuffle_epi32 (a, _SWAP_HI_LOW32);
    a128 = _MM_CVTEPU8_EPI16 (a128);
    r128_2 = _mm_slli_epi16 (a128, b);
    return _mm_packus_epi16 (r128_1, r128_2);         //saturated u8
}

uint16x8_t vqshlq_n_u16(uint16x8_t a, __constrange(0,15) int b);         // VQSHL.s16 q0,q0,#0
_NEON2SSE_INLINE uint16x8_t vqshlq_n_u16(uint16x8_t a, __constrange(0,15) int b)         // VQSHL.s16 q0,q0,#0
{         // manual saturation solution looks more optimal than 32 bits conversion one
    __m128i cb, c8000, a_signed, saturation_mask,  shift_res;
    cb = _mm_set1_epi16((1 << (16 - b)) - 1 - 0x8000 );
    c8000 = _mm_set1_epi16 (0x8000);
//no unsigned shorts comparison in SSE, only signed available, so need the trick
    a_signed = _mm_sub_epi16(a, c8000);         //go to signed
    saturation_mask = _mm_cmpgt_epi16 (a_signed, cb);
    shift_res = _mm_slli_epi16 (a, b);
    return _mm_or_si128 (shift_res, saturation_mask);
}

uint32x4_t vqshlq_n_u32(uint32x4_t a, __constrange(0,31) int b);         // VQSHL.U32 q0,q0,#0
_NEON2SSE_INLINE uint32x4_t vqshlq_n_u32(uint32x4_t a, __constrange(0,31) int b)         // VQSHL.U32 q0,q0,#0
{         // manual saturation solution, no 64 bit saturation option, the serial version may be faster
    __m128i cb, c80000000, a_signed, saturation_mask,  shift_res;
    cb = _mm_set1_epi32((1 << (32 - b)) - 1 - 0x80000000 );
    c80000000 = _mm_set1_epi32 (0x80000000);
//no unsigned ints comparison in SSE, only signed available, so need the trick
    a_signed = _mm_sub_epi32(a, c80000000);         //go to signed
    saturation_mask = _mm_cmpgt_epi32 (a_signed, cb);
    shift_res = _mm_slli_epi32 (a, b);
    return _mm_or_si128 (shift_res, saturation_mask);
}

uint64x2_t vqshlq_n_u64(uint64x2_t a, __constrange(0,63) int b);         // VQSHL.U64 q0,q0,#0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint64x2_t vqshlq_n_u64(uint64x2_t a, __constrange(0,63) int b), _NEON2SSE_REASON_SLOW_SERIAL)
{         // no effective SIMD solution here
    _NEON2SSE_ALIGN_16 uint64_t atmp[2], res[2];
    uint64_t bmask;
    int i;
    bmask = ( uint64_t)1 << (64 - b);
    _mm_store_si128((__m128i*)atmp, a);
    for (i = 0; i<2; i++) {
        res[i] = (atmp[i] >= bmask)&&(b>0) ? 0xffffffffffffffff : atmp[i] << b;         //if b=0 we are fine with any a
    }
    return _mm_load_si128((__m128i*)res);
}

//**************Vector signed->unsigned saturating shift left by constant *************
//*************************************************************************************

uint8x16_t vqshluq_n_s8(int8x16_t a, __constrange(0,7) int b);         // VQSHLU.S8 q0,q0,#0
_NEON2SSE_INLINE uint8x16_t vqshluq_n_s8(int8x16_t a, __constrange(0,7) int b)         // VQSHLU.S8 q0,q0,#0
{
    __m128i a128, r128_1, r128_2;
    a128 = _MM_CVTEPI8_EPI16 (a);         //SSE 4.1
    r128_1 = _mm_slli_epi16 (a128, b);
    //swap hi and low part of a128 to process the remaining data
    a128 = _mm_shuffle_epi32 (a, _SWAP_HI_LOW32);
    a128 = _MM_CVTEPI8_EPI16 (a128);
    r128_2 = _mm_slli_epi16 (a128, b);
    return _mm_packus_epi16 (r128_1, r128_2);         //saturated u8
}

#if defined(USE_SSSE3)
uint16x8_t vqshluq_n_s16(int16x8_t a, __constrange(0,15) int b);         // VQSHLU.S16 q0,q0,#0
_NEON2SSE_INLINE uint16x8_t vqshluq_n_s16(int16x8_t a, __constrange(0,15) int b)         // VQSHLU.S16 q0,q0,#0
{         // manual saturation solution looks LESS optimal than 32 bits conversion one
    __m128i a128, r128_1, r128_2;
    a128 = _MM_CVTEPI16_EPI32 (a);         //SSE 4.1
    r128_1 = _mm_slli_epi32 (a128, b);         //shift_res
    //swap hi and low part of a128 to process the remaining data
    a128 = _mm_shuffle_epi32 (a, _SWAP_HI_LOW32);
    a128 = _MM_CVTEPI16_EPI32 (a128);
    r128_2 = _mm_slli_epi32 (a128, b);
    return _MM_PACKUS_EPI32 (r128_1, r128_2);         //saturated s16
}
#endif

uint32x4_t vqshluq_n_s32(int32x4_t a, __constrange(0,31) int b);         // VQSHLU.S32 q0,q0,#0
_NEON2SSE_INLINE uint32x4_t vqshluq_n_s32(int32x4_t a, __constrange(0,31) int b)         // VQSHLU.S32 q0,q0,#0
{         //solution may be  not optimal compared with the serial one
    __m128i zero, maskA, maskGT0, a0,  a_masked, a_shift;
    zero = _mm_setzero_si128();
    maskA = _mm_cmpeq_epi32(a, a);
    maskA = _mm_slli_epi32(maskA,(32 - b));         // b ones and (32-b)zeros
    //saturate negative numbers to zero
    maskGT0   = _mm_cmpgt_epi32 (a, zero);         // //0xffffffff if positive number and zero otherwise (negative numbers)
    a0 = _mm_and_si128 (a,  maskGT0);         //negative are zeros now
    //saturate positive to 0xffffffff
    a_masked = _mm_and_si128 (a0, maskA);
    a_masked = _mm_cmpgt_epi32 (a_masked, zero);         //0xffffffff if saturation necessary 0 otherwise
    a_shift = _mm_slli_epi32 (a0, b);
    return _mm_or_si128 (a_shift, a_masked);         //actual saturation
}

uint64x2_t vqshluq_n_s64(int64x2_t a, __constrange(0,63) int b);         // VQSHLU.S64 q0,q0,#0
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(uint64x2_t vqshluq_n_s64(int64x2_t a, __constrange(0,63) int b),  _NEON2SSE_REASON_SLOW_SERIAL)
{         // no effective SIMD solution here, serial execution looks faster
    _NEON2SSE_ALIGN_16 int64_t atmp[2];
    _NEON2SSE_ALIGN_16 uint64_t res[2];
    uint64_t limit;
    int i;
    _mm_store_si128((__m128i*)atmp, a);
    for (i = 0; i<2; i++) {
        if (atmp[i]<=0) {
            res[i] = 0;
        } else {
            limit = (uint64_t) 1 << (64 - b);
            res[i] = ( ((uint64_t)atmp[i]) >= limit) ? res[i] = ~((uint64_t)0) : atmp[i] << b;
        }
    }
    return _mm_load_si128((__m128i*)res);
}

//************** Vector narrowing  shift right by constant **************
//**********************************************************************

//************** Vector signed->unsigned narrowing saturating shift right by constant ********
//*********************************************************************************************

//**** Vector signed->unsigned rounding narrowing saturating shift right by constant *****

//***** Vector narrowing saturating shift right by constant ******
//*****************************************************************

//********* Vector rounding narrowing shift right by constant *************************
//****************************************************************************************

//************* Vector rounding narrowing saturating shift right by constant ************
//****************************************************************************************

//************** Vector widening shift left by constant ****************
//************************************************************************

//************************************************************************************
//**************************** Shifts with insert ************************************
//************************************************************************************
//takes each element in a vector,  shifts them by an immediate value,
//and inserts the results in the destination vector. Bits shifted out of the each element are lost.

//**************** Vector shift right and insert ************************************
//Actually the "c" left bits from "a" are the only bits remained from "a"  after the shift.
//All other bits are taken from b shifted.

int8x16_t vsriq_n_s8(int8x16_t a, int8x16_t b, __constrange(1,8) int c);         // VSRI.8 q0,q0,#8
_NEON2SSE_INLINE int8x16_t vsriq_n_s8(int8x16_t a, int8x16_t b, __constrange(1,8) int c)         // VSRI.8 q0,q0,#8
{
    __m128i maskA, a_masked;
    uint8x16_t b_shift;
    _NEON2SSE_ALIGN_16 uint8_t maskLeft[9] = {0x0, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};         //"a" bits mask, 0 bit not used
    maskA = _mm_set1_epi8(maskLeft[c]);         // c ones and (8-c)zeros
    a_masked = _mm_and_si128 (a, maskA);
    b_shift = vshrq_n_u8( b, c);         // c zeros on the left in b due to logical shift
    return _mm_or_si128 (a_masked, b_shift);         //combine (insert b into a)
}

int16x8_t vsriq_n_s16(int16x8_t a, int16x8_t b, __constrange(1,16) int c);         // VSRI.16 q0,q0,#16
_NEON2SSE_INLINE int16x8_t vsriq_n_s16(int16x8_t a, int16x8_t b, __constrange(1,16) int c)         // VSRI.16 q0,q0,#16
{         //to cut "c" left bits from a we do shift right and then  shift back left providing c right zeros in a
    uint16x8_t b_shift;
    uint16x8_t a_c;
    b_shift = vshrq_n_u16( b, c);         // c zeros on the left in b due to logical shift
    a_c = vshrq_n_u16( a, (16 - c));
    a_c  = _mm_slli_epi16(a_c, (16 - c));         //logical shift provides right "c" bits zeros in a
    return _mm_or_si128 (a_c, b_shift);         //combine (insert b into a)
}

int32x4_t vsriq_n_s32(int32x4_t a, int32x4_t b, __constrange(1,32) int c);         // VSRI.32 q0,q0,#32
_NEON2SSE_INLINE int32x4_t vsriq_n_s32(int32x4_t a, int32x4_t b, __constrange(1,32) int c)         // VSRI.32 q0,q0,#32
{         //to cut "c" left bits from a we do shift right and then  shift back left providing c right zeros in a
    uint32x4_t b_shift;
    uint32x4_t a_c;
    b_shift = vshrq_n_u32( b, c);         // c zeros on the left in b due to logical shift
    a_c = vshrq_n_u32( a, (32 - c));
    a_c  = _mm_slli_epi32(a_c, (32 - c));         //logical shift provides right "c" bits zeros in a
    return _mm_or_si128 (a_c, b_shift);         //combine (insert b into a)
}

int64x2_t vsriq_n_s64(int64x2_t a, int64x2_t b, __constrange(1,64) int c);         // VSRI.64 q0,q0,#64
_NEON2SSE_INLINE int64x2_t vsriq_n_s64(int64x2_t a, int64x2_t b, __constrange(1,64) int c)
{         //serial solution may be faster
    uint64x2_t b_shift;
    uint64x2_t a_c;
    b_shift = _mm_srli_epi64(b, c);         // c zeros on the left in b due to logical shift
    a_c = _mm_srli_epi64(a, (64 - c));
    a_c  = _mm_slli_epi64(a_c, (64 - c));         //logical shift provides right "c" bits zeros in a
    return _mm_or_si128 (a_c, b_shift);         //combine (insert b into a)
}

uint8x16_t vsriq_n_u8(uint8x16_t a, uint8x16_t b, __constrange(1,8) int c);         // VSRI.8 q0,q0,#8
#define vsriq_n_u8 vsriq_n_s8

uint16x8_t vsriq_n_u16(uint16x8_t a, uint16x8_t b, __constrange(1,16) int c);         // VSRI.16 q0,q0,#16
#define vsriq_n_u16 vsriq_n_s16

uint32x4_t vsriq_n_u32(uint32x4_t a, uint32x4_t b, __constrange(1,32) int c);         // VSRI.32 q0,q0,#32
#define vsriq_n_u32 vsriq_n_s32

uint64x2_t vsriq_n_u64(uint64x2_t a, uint64x2_t b, __constrange(1,64) int c);         // VSRI.64 q0,q0,#64
#define vsriq_n_u64 vsriq_n_s64

poly8x16_t vsriq_n_p8(poly8x16_t a, poly8x16_t b, __constrange(1,8) int c);         // VSRI.8 q0,q0,#8
#define vsriq_n_p8 vsriq_n_u8

poly16x8_t vsriq_n_p16(poly16x8_t a, poly16x8_t b, __constrange(1,16) int c);         // VSRI.16 q0,q0,#16
#define vsriq_n_p16 vsriq_n_u16

//***** Vector shift left and insert *********************************************
//*********************************************************************************
//Actually the "c" right bits from "a" are the only bits remained from "a"  after the shift.
//All other bits are taken from b shifted. Ending zeros are inserted in b in the shift proces. We need to combine "a" and "b shifted".

int8x16_t vsliq_n_s8(int8x16_t a, int8x16_t b, __constrange(0,7) int c);         // VSLI.8 q0,q0,#0
_NEON2SSE_INLINE int8x16_t vsliq_n_s8(int8x16_t a, int8x16_t b, __constrange(0,7) int c)         // VSLI.8 q0,q0,#0
{
    __m128i maskA, a_masked;
    int8x16_t b_shift;
    _NEON2SSE_ALIGN_16 uint8_t maskRight[8] = {0x0, 0x1, 0x3, 0x7, 0x0f, 0x1f, 0x3f, 0x7f};         //"a" bits mask
    maskA = _mm_set1_epi8(maskRight[c]);         // (8-c)zeros and c ones
    b_shift = vshlq_n_s8( b, c);
    a_masked = _mm_and_si128 (a, maskA);
    return _mm_or_si128 (b_shift, a_masked);         //combine (insert b into a)
}

int16x8_t vsliq_n_s16(int16x8_t a, int16x8_t b, __constrange(0,15) int c);         // VSLI.16 q0,q0,#0
_NEON2SSE_INLINE int16x8_t vsliq_n_s16(int16x8_t a, int16x8_t b, __constrange(0,15) int c)         // VSLI.16 q0,q0,#0
{         //to cut "c" right bits from a we do shift left and then logical shift back right providing (16-c)zeros in a
    int16x8_t b_shift;
    int16x8_t a_c;
    b_shift = vshlq_n_s16( b, c);
    a_c = vshlq_n_s16( a, (16 - c));
    a_c  = _mm_srli_epi16(a_c, (16 - c));
    return _mm_or_si128 (b_shift, a_c);         //combine (insert b into a)
}

int32x4_t vsliq_n_s32(int32x4_t a, int32x4_t b, __constrange(0,31) int c);         // VSLI.32 q0,q0,#0
_NEON2SSE_INLINE int32x4_t vsliq_n_s32(int32x4_t a, int32x4_t b, __constrange(0,31) int c)         // VSLI.32 q0,q0,#0
{         //solution may be  not optimal compared with the serial one
      //to cut "c" right bits from a we do shift left and then logical shift back right providing (32-c)zeros in a
    int32x4_t b_shift;
    int32x4_t a_c;
    b_shift = vshlq_n_s32( b, c);
    a_c = vshlq_n_s32( a, (32 - c));
    a_c  = _mm_srli_epi32(a_c, (32 - c));
    return _mm_or_si128 (b_shift, a_c);         //combine (insert b into a)
}

int64x2_t vsliq_n_s64(int64x2_t a, int64x2_t b, __constrange(0,63) int c);         // VSLI.64 q0,q0,#0
_NEON2SSE_INLINE int64x2_t vsliq_n_s64(int64x2_t a, int64x2_t b, __constrange(0,63) int c)         // VSLI.64 q0,q0,#0
{         //solution may be  not optimal compared with the serial one
      //to cut "c" right bits from a we do shift left and then logical shift back right providing (64-c)zeros in a
    int64x2_t b_shift;
    int64x2_t a_c;
    b_shift = vshlq_n_s64( b, c);
    a_c = vshlq_n_s64( a, (64 - c));
    a_c  = _mm_srli_epi64(a_c, (64 - c));
    return _mm_or_si128 (b_shift, a_c);         //combine (insert b into a)
}

uint8x16_t vsliq_n_u8(uint8x16_t a, uint8x16_t b, __constrange(0,7) int c);         // VSLI.8 q0,q0,#0
#define vsliq_n_u8 vsliq_n_s8

uint16x8_t vsliq_n_u16(uint16x8_t a, uint16x8_t b, __constrange(0,15) int c);         // VSLI.16 q0,q0,#0
#define vsliq_n_u16 vsliq_n_s16

uint32x4_t vsliq_n_u32(uint32x4_t a, uint32x4_t b, __constrange(0,31) int c);         // VSLI.32 q0,q0,#0
#define vsliq_n_u32 vsliq_n_s32

uint64x2_t vsliq_n_u64(uint64x2_t a, uint64x2_t b, __constrange(0,63) int c);         // VSLI.64 q0,q0,#0
#define vsliq_n_u64 vsliq_n_s64

poly8x16_t vsliq_n_p8(poly8x16_t a, poly8x16_t b, __constrange(0,7) int c);         // VSLI.8 q0,q0,#0
#define vsliq_n_p8 vsliq_n_u8

poly16x8_t vsliq_n_p16(poly16x8_t a, poly16x8_t b, __constrange(0,15) int c);         // VSLI.16 q0,q0,#0
#define vsliq_n_p16 vsliq_n_u16

// ***********************************************************************************************
// ****************** Loads and stores of a single vector ***************************************
// ***********************************************************************************************
//Performs loads and stores of a single vector of some type.
//*******************************  Loads ********************************************************
// ***********************************************************************************************
//We assume ptr is NOT aligned in general case and use __m128i _mm_loadu_si128 ((__m128i*) ptr);.
//also for SSE3  supporting systems the __m128i _mm_lddqu_si128 (__m128i const* p) usage for unaligned access may be advantageous.
// it loads a 32-byte block aligned on a 16-byte boundary and extracts the 16 bytes corresponding to the unaligned access
//If the ptr is aligned then could use __m128i _mm_load_si128 ((__m128i*) ptr) instead;
#define LOAD_SI128(ptr) \
        ( ((unsigned long)(ptr) & 15) == 0 ) ? _mm_load_si128((__m128i*)(ptr)) : _mm_loadu_si128((__m128i*)(ptr));

uint8x16_t vld1q_u8(__transfersize(16) uint8_t const * ptr);         // VLD1.8 {d0, d1}, [r0]
#define vld1q_u8 LOAD_SI128

uint16x8_t vld1q_u16(__transfersize(8) uint16_t const * ptr);         // VLD1.16 {d0, d1}, [r0]
#define vld1q_u16 LOAD_SI128

uint32x4_t vld1q_u32(__transfersize(4) uint32_t const * ptr);         // VLD1.32 {d0, d1}, [r0]
#define vld1q_u32 LOAD_SI128

uint64x2_t vld1q_u64(__transfersize(2) uint64_t const * ptr);         // VLD1.64 {d0, d1}, [r0]
#define vld1q_u64 LOAD_SI128

int8x16_t vld1q_s8(__transfersize(16) int8_t const * ptr);         // VLD1.8 {d0, d1}, [r0]
#define vld1q_s8 LOAD_SI128

int16x8_t vld1q_s16(__transfersize(8) int16_t const * ptr);         // VLD1.16 {d0, d1}, [r0]
#define vld1q_s16 LOAD_SI128

int32x4_t vld1q_s32(__transfersize(4) int32_t const * ptr);         // VLD1.32 {d0, d1}, [r0]
#define vld1q_s32 LOAD_SI128

int64x2_t vld1q_s64(__transfersize(2) int64_t const * ptr);         // VLD1.64 {d0, d1}, [r0]
#define vld1q_s64 LOAD_SI128

float16x8_t vld1q_f16(__transfersize(8) __fp16 const * ptr);         // VLD1.16 {d0, d1}, [r0]
// IA32 SIMD doesn't work with 16bit floats currently, so need to go to 32 bit and then work with two 128bit registers
/* _NEON2SSE_INLINE float16x8_t vld1q_f16(__transfersize(8) __fp16 const * ptr)// VLD1.16 {d0, d1}, [r0]
{__m128 f1 = _mm_set_ps (ptr[3], ptr[2], ptr[1], ptr[0]);
__m128 f2;
f2 = _mm_set_ps (ptr[7], ptr[6], ptr[5], ptr[4]);
}*/

float32x4_t vld1q_f32(__transfersize(4) float32_t const * ptr);         // VLD1.32 {d0, d1}, [r0]
_NEON2SSE_INLINE float32x4_t vld1q_f32(__transfersize(4) float32_t const * ptr)
{
    if( (((unsigned long)(ptr)) & 15 ) == 0 )         //16 bits aligned
        return _mm_load_ps(ptr);
    else
        return _mm_loadu_ps(ptr);
}

poly8x16_t vld1q_p8(__transfersize(16) poly8_t const * ptr);         // VLD1.8 {d0, d1}, [r0]
#define vld1q_p8  LOAD_SI128

poly16x8_t vld1q_p16(__transfersize(8) poly16_t const * ptr);         // VLD1.16 {d0, d1}, [r0]
#define vld1q_p16 LOAD_SI128

// IA32 SIMD doesn't work with 16bit floats currently, so need to go to 32 bit like _mm_set_ps (ptr[3], ptr[2], ptr[1], ptr[0]);

//***********************************************************************************************************
//******* Lane load functions - insert the data at  vector's given position (lane) *************************
//***********************************************************************************************************
uint8x16_t vld1q_lane_u8(__transfersize(1) uint8_t const * ptr, uint8x16_t vec, __constrange(0,15) int lane);         // VLD1.8 {d0[0]}, [r0]
#define vld1q_lane_u8(ptr, vec, lane) _MM_INSERT_EPI8(vec, *(ptr), lane)

uint16x8_t vld1q_lane_u16(__transfersize(1)    uint16_t const * ptr, uint16x8_t vec, __constrange(0,7) int lane);         // VLD1.16 {d0[0]}, [r0]
#define vld1q_lane_u16(ptr, vec, lane) _MM_INSERT_EPI16(vec, *(ptr), lane)

uint32x4_t vld1q_lane_u32(__transfersize(1) uint32_t const * ptr, uint32x4_t vec, __constrange(0,3) int lane);         // VLD1.32 {d0[0]}, [r0]
#define vld1q_lane_u32(ptr, vec, lane) _MM_INSERT_EPI32(vec, *(ptr), lane)

uint64x2_t vld1q_lane_u64(__transfersize(1) uint64_t const * ptr, uint64x2_t vec, __constrange(0,1) int lane);         // VLD1.64 {d0}, [r0]
#define vld1q_lane_u64(ptr, vec, lane) _MM_INSERT_EPI64(vec, *(ptr), lane);         // _p;

int8x16_t vld1q_lane_s8(__transfersize(1) int8_t const * ptr, int8x16_t vec, __constrange(0,15) int lane);         // VLD1.8 {d0[0]}, [r0]
#define vld1q_lane_s8(ptr, vec, lane) _MM_INSERT_EPI8(vec, *(ptr), lane)

int16x8_t vld1q_lane_s16(__transfersize(1) int16_t const * ptr, int16x8_t vec, __constrange(0,7) int lane);         // VLD1.16 {d0[0]}, [r0]
#define vld1q_lane_s16(ptr, vec, lane) _MM_INSERT_EPI16(vec, *(ptr), lane)

int32x4_t vld1q_lane_s32(__transfersize(1) int32_t const * ptr, int32x4_t vec, __constrange(0,3) int lane);         // VLD1.32 {d0[0]}, [r0]
#define vld1q_lane_s32(ptr, vec, lane) _MM_INSERT_EPI32(vec, *(ptr), lane)

//current IA SIMD doesn't support float16

float32x4_t vld1q_lane_f32(__transfersize(1) float32_t const * ptr, float32x4_t vec, __constrange(0,3) int lane);         // VLD1.32 {d0[0]}, [r0]
_NEON2SSE_INLINE float32x4_t vld1q_lane_f32(__transfersize(1) float32_t const * ptr, float32x4_t vec, __constrange(0,3) int lane)
{         //we need to deal with  ptr  16bit NOT aligned case
    __m128 p;
    p = _mm_set1_ps(*(ptr));
    return _MM_INSERT_PS(vec,  p, _INSERTPS_NDX(0, lane));
}

int64x2_t vld1q_lane_s64(__transfersize(1) int64_t const * ptr, int64x2_t vec, __constrange(0,1) int lane);         // VLD1.64 {d0}, [r0]
#define vld1q_lane_s64(ptr, vec, lane) _MM_INSERT_EPI64(vec, *(ptr), lane)

poly8x16_t vld1q_lane_p8(__transfersize(1) poly8_t const * ptr, poly8x16_t vec, __constrange(0,15) int lane);         // VLD1.8 {d0[0]}, [r0]
#define vld1q_lane_p8(ptr, vec, lane) _MM_INSERT_EPI8(vec, *(ptr), lane)

poly16x8_t vld1q_lane_p16(__transfersize(1) poly16_t const * ptr, poly16x8_t vec, __constrange(0,7) int lane);         // VLD1.16 {d0[0]}, [r0]
#define vld1q_lane_p16(ptr, vec, lane) _MM_INSERT_EPI16(vec, *(ptr), lane)

//serial solution may be faster

//current IA SIMD doesn't support float16

// ****************** Load single value ( set all lanes of vector with same value from memory)**********************
// ******************************************************************************************************************
uint8x16_t vld1q_dup_u8(__transfersize(1) uint8_t const * ptr);         // VLD1.8 {d0[]}, [r0]
#define vld1q_dup_u8(ptr) _mm_set1_epi8(*(ptr))

uint16x8_t vld1q_dup_u16(__transfersize(1) uint16_t const * ptr);         // VLD1.16 {d0[]}, [r0]
#define vld1q_dup_u16(ptr) _mm_set1_epi16(*(ptr))

uint32x4_t vld1q_dup_u32(__transfersize(1) uint32_t const * ptr);         // VLD1.32 {d0[]}, [r0]
#define vld1q_dup_u32(ptr) _mm_set1_epi32(*(ptr))

uint64x2_t vld1q_dup_u64(__transfersize(1) uint64_t const * ptr);         // VLD1.64 {d0}, [r0]
_NEON2SSE_INLINE uint64x2_t   vld1q_dup_u64(__transfersize(1) uint64_t const * ptr)
{
    _NEON2SSE_ALIGN_16 uint64_t val[2] = {*(ptr), *(ptr)};
    return LOAD_SI128(val);
}

int8x16_t vld1q_dup_s8(__transfersize(1) int8_t const * ptr);         // VLD1.8 {d0[]}, [r0]
#define vld1q_dup_s8(ptr) _mm_set1_epi8(*(ptr))

int16x8_t vld1q_dup_s16(__transfersize(1) int16_t const * ptr);         // VLD1.16 {d0[]}, [r0]
#define vld1q_dup_s16(ptr) _mm_set1_epi16 (*(ptr))

int32x4_t vld1q_dup_s32(__transfersize(1) int32_t const * ptr);         // VLD1.32 {d0[]}, [r0]
#define vld1q_dup_s32(ptr) _mm_set1_epi32 (*(ptr))

int64x2_t vld1q_dup_s64(__transfersize(1) int64_t const * ptr);         // VLD1.64 {d0}, [r0]
#define vld1q_dup_s64(ptr) vld1q_dup_u64((uint64_t*)ptr)

float16x8_t vld1q_dup_f16(__transfersize(1) __fp16 const * ptr);         // VLD1.16 {d0[]}, [r0]
//current IA SIMD doesn't support float16, need to go to 32 bits

float32x4_t vld1q_dup_f32(__transfersize(1) float32_t const * ptr);         // VLD1.32 {d0[]}, [r0]
#define vld1q_dup_f32(ptr) _mm_set1_ps (*(ptr))

poly8x16_t vld1q_dup_p8(__transfersize(1) poly8_t const * ptr);         // VLD1.8 {d0[]}, [r0]
#define vld1q_dup_p8(ptr) _mm_set1_epi8(*(ptr))

poly16x8_t vld1q_dup_p16(__transfersize(1) poly16_t const * ptr);         // VLD1.16 {d0[]}, [r0]
#define vld1q_dup_p16(ptr) _mm_set1_epi16 (*(ptr))

//current IA SIMD doesn't support float16

//*************************************************************************************
//********************************* Store **********************************************
//*************************************************************************************
// If ptr is 16bit aligned and you  need to store data without cache pollution then use void _mm_stream_si128 ((__m128i*)ptr, val);
//here we assume the case of  NOT 16bit aligned ptr possible. If it is aligned we could to use _mm_store_si128 like shown in the following macro
#define STORE_SI128(ptr, val) \
        (((unsigned long)(ptr) & 15) == 0 ) ? _mm_store_si128 ((__m128i*)(ptr), val) : _mm_storeu_si128 ((__m128i*)(ptr), val);

void vst1q_u8(__transfersize(16) uint8_t * ptr, uint8x16_t val);         // VST1.8 {d0, d1}, [r0]
#define vst1q_u8 STORE_SI128

void vst1q_u16(__transfersize(8) uint16_t * ptr, uint16x8_t val);         // VST1.16 {d0, d1}, [r0]
#define vst1q_u16 STORE_SI128

void vst1q_u32(__transfersize(4) uint32_t * ptr, uint32x4_t val);         // VST1.32 {d0, d1}, [r0]
#define vst1q_u32 STORE_SI128

void vst1q_u64(__transfersize(2) uint64_t * ptr, uint64x2_t val);         // VST1.64 {d0, d1}, [r0]
#define vst1q_u64 STORE_SI128

void vst1q_s8(__transfersize(16) int8_t * ptr, int8x16_t val);         // VST1.8 {d0, d1}, [r0]
#define vst1q_s8 STORE_SI128

void vst1q_s16(__transfersize(8) int16_t * ptr, int16x8_t val);         // VST1.16 {d0, d1}, [r0]
#define vst1q_s16 STORE_SI128

void vst1q_s32(__transfersize(4) int32_t * ptr, int32x4_t val);         // VST1.32 {d0, d1}, [r0]
#define vst1q_s32 STORE_SI128

void vst1q_s64(__transfersize(2) int64_t * ptr, int64x2_t val);         // VST1.64 {d0, d1}, [r0]
#define vst1q_s64 STORE_SI128

void vst1q_f16(__transfersize(8) __fp16 * ptr, float16x8_t val);         // VST1.16 {d0, d1}, [r0]
// IA32 SIMD doesn't work with 16bit floats currently

void vst1q_f32(__transfersize(4) float32_t * ptr, float32x4_t val);         // VST1.32 {d0, d1}, [r0]
_NEON2SSE_INLINE void vst1q_f32(__transfersize(4) float32_t * ptr, float32x4_t val)
{
    if( ((unsigned long)(ptr) & 15)  == 0 )         //16 bits aligned
        _mm_store_ps (ptr, val);
    else
        _mm_storeu_ps (ptr, val);
}

void vst1q_p8(__transfersize(16) poly8_t * ptr, poly8x16_t val);         // VST1.8 {d0, d1}, [r0]
#define vst1q_p8  vst1q_u8

void vst1q_p16(__transfersize(8) poly16_t * ptr, poly16x8_t val);         // VST1.16 {d0, d1}, [r0]
#define vst1q_p16 vst1q_u16

//current IA SIMD doesn't support float16

//***********Store a lane of a vector into memory (extract given lane) *********************
//******************************************************************************************
void vst1q_lane_u8(__transfersize(1) uint8_t * ptr, uint8x16_t val, __constrange(0,15) int lane);         // VST1.8 {d0[0]}, [r0]
#define vst1q_lane_u8(ptr, val, lane) *(ptr) = _MM_EXTRACT_EPI8 (val, lane)

void vst1q_lane_u16(__transfersize(1) uint16_t * ptr, uint16x8_t val, __constrange(0,7) int lane);         // VST1.16 {d0[0]}, [r0]
#define vst1q_lane_u16(ptr, val, lane) *(ptr) = _MM_EXTRACT_EPI16 (val, lane)

void vst1q_lane_u32(__transfersize(1) uint32_t * ptr, uint32x4_t val, __constrange(0,3) int lane);         // VST1.32 {d0[0]}, [r0]
#define vst1q_lane_u32(ptr, val, lane) *(ptr) = _MM_EXTRACT_EPI32 (val, lane)

void vst1q_lane_u64(__transfersize(1) uint64_t * ptr, uint64x2_t val, __constrange(0,1) int lane);         // VST1.64 {d0}, [r0]
#define vst1q_lane_u64(ptr, val, lane) *(ptr) = _MM_EXTRACT_EPI64 (val, lane)

void vst1q_lane_s8(__transfersize(1) int8_t * ptr, int8x16_t val, __constrange(0,15) int lane);         // VST1.8 {d0[0]}, [r0]
#define vst1q_lane_s8(ptr, val, lane) *(ptr) = _MM_EXTRACT_EPI8 (val, lane)

void vst1q_lane_s16(__transfersize(1) int16_t * ptr, int16x8_t val, __constrange(0,7) int lane);         // VST1.16 {d0[0]}, [r0]
#define vst1q_lane_s16(ptr, val, lane) *(ptr) = _MM_EXTRACT_EPI16 (val, lane)

void vst1q_lane_s32(__transfersize(1) int32_t * ptr, int32x4_t val, __constrange(0,3) int lane);         // VST1.32 {d0[0]}, [r0]
#define vst1q_lane_s32(ptr, val, lane) *(ptr) = _MM_EXTRACT_EPI32 (val, lane)

void vst1q_lane_s64(__transfersize(1) int64_t * ptr, int64x2_t val, __constrange(0,1) int lane);         // VST1.64 {d0}, [r0]
#define vst1q_lane_s64(ptr, val, lane) *(ptr) = _MM_EXTRACT_EPI64 (val, lane)

void vst1q_lane_f16(__transfersize(1) __fp16 * ptr, float16x8_t val, __constrange(0,7) int lane);         // VST1.16 {d0[0]}, [r0]
//current IA SIMD doesn't support float16

void vst1q_lane_f32(__transfersize(1) float32_t * ptr, float32x4_t val, __constrange(0,3) int lane);         // VST1.32 {d0[0]}, [r0]
_NEON2SSE_INLINE void vst1q_lane_f32(__transfersize(1) float32_t * ptr, float32x4_t val, __constrange(0,3) int lane)
{
    int32_t ilane;
    ilane = _MM_EXTRACT_PS(val,lane);
    *(ptr) =  *((float*)&ilane);
}

void vst1q_lane_p8(__transfersize(1) poly8_t * ptr, poly8x16_t val, __constrange(0,15) int lane);         // VST1.8 {d0[0]}, [r0]
#define vst1q_lane_p8   vst1q_lane_u8

void vst1q_lane_p16(__transfersize(1) poly16_t * ptr, poly16x8_t val, __constrange(0,7) int lane);         // VST1.16 {d0[0]}, [r0]
#define vst1q_lane_p16   vst1q_lane_s16

//current IA SIMD doesn't support float16

//***********************************************************************************************
//**************** Loads and stores of an N-element structure **********************************
//***********************************************************************************************
//These intrinsics load or store an n-element structure. The array structures are defined in the beginning
//We assume ptr is NOT aligned in general case, for more details see  "Loads and stores of a single vector functions"
//****************** 2 elements load  *********************************************
uint8x16x2_t vld2q_u8(__transfersize(32) uint8_t const * ptr);         // VLD2.8 {d0, d2}, [r0]
_NEON2SSE_INLINE uint8x16x2_t vld2q_u8(__transfersize(32) uint8_t const * ptr)         // VLD2.8 {d0, d2}, [r0]
{
    uint8x16x2_t v;
    v.val[0] = vld1q_u8(ptr);
    v.val[1] = vld1q_u8((ptr + 16));
    v = vuzpq_s8(v.val[0], v.val[1]);
    return v;
}

#if defined(USE_SSSE3)
uint16x8x2_t vld2q_u16(__transfersize(16) uint16_t const * ptr);         // VLD2.16 {d0, d2}, [r0]
_NEON2SSE_INLINE uint16x8x2_t vld2q_u16(__transfersize(16) uint16_t const * ptr)         // VLD2.16 {d0, d2}, [r0]
{
    uint16x8x2_t v;
    v.val[0] = vld1q_u16( ptr);
    v.val[1] = vld1q_u16( (ptr + 8));
    v = vuzpq_s16(v.val[0], v.val[1]);
    return v;
}
#endif

uint32x4x2_t vld2q_u32(__transfersize(8) uint32_t const * ptr);         // VLD2.32 {d0, d2}, [r0]
_NEON2SSE_INLINE uint32x4x2_t vld2q_u32(__transfersize(8) uint32_t const * ptr)         // VLD2.32 {d0, d2}, [r0]
{
    uint32x4x2_t v;
    v.val[0] = vld1q_u32 ( ptr);
    v.val[1] = vld1q_u32 ( (ptr + 4));
    v = vuzpq_s32(v.val[0], v.val[1]);
    return v;
}

int8x16x2_t vld2q_s8(__transfersize(32) int8_t const * ptr);
#define  vld2q_s8(ptr) vld2q_u8((uint8_t*) ptr)

#if defined(USE_SSSE3)
int16x8x2_t vld2q_s16(__transfersize(16) int16_t const * ptr);         // VLD2.16 {d0, d2}, [r0]
#define vld2q_s16(ptr) vld2q_u16((uint16_t*) ptr)
#endif

int32x4x2_t vld2q_s32(__transfersize(8) int32_t const * ptr);         // VLD2.32 {d0, d2}, [r0]
#define vld2q_s32(ptr) vld2q_u32((uint32_t*) ptr)

float16x8x2_t vld2q_f16(__transfersize(16) __fp16 const * ptr);         // VLD2.16 {d0, d2}, [r0]
// IA32 SIMD doesn't work with 16bit floats currently, so need to go to 32 bit and then work with two 128bit registers. See vld1q_f16 for example

float32x4x2_t vld2q_f32(__transfersize(8) float32_t const * ptr);         // VLD2.32 {d0, d2}, [r0]
_NEON2SSE_INLINE float32x4x2_t vld2q_f32(__transfersize(8) float32_t const * ptr)         // VLD2.32 {d0, d2}, [r0]
{
    float32x4x2_t v;
    v.val[0] =  vld1q_f32 (ptr);
    v.val[1] =  vld1q_f32 ((ptr + 4));
    v = vuzpq_f32(v.val[0], v.val[1]);
    return v;
}

poly8x16x2_t vld2q_p8(__transfersize(32) poly8_t const * ptr);         // VLD2.8 {d0, d2}, [r0]
#define  vld2q_p8 vld2q_u8

#if defined(USE_SSSE3)
poly16x8x2_t vld2q_p16(__transfersize(16) poly16_t const * ptr);         // VLD2.16 {d0, d2}, [r0]
#define vld2q_p16 vld2q_u16
#endif

#if defined(USE_SSSE3)
uint8x8x2_t vld2_u8(__transfersize(16) uint8_t const * ptr);         // VLD2.8 {d0, d1}, [r0]
_NEON2SSE_INLINE uint8x8x2_t vld2_u8(__transfersize(16) uint8_t const * ptr)
{
    uint8x8x2_t v;
    _NEON2SSE_ALIGN_16 int8_t mask8_even_odd[16] = { 0, 2, 4, 6, 8, 10, 12, 14, 1, 3, 5, 7, 9, 11, 13, 15};
    __m128i ld128;
    ld128 = vld1q_u8(ptr);         //merge two 64-bits in 128 bit
    v.val[0] = _mm_shuffle_epi8(ld128, *(__m128i*)mask8_even_odd);
    v.val[1] = _mm_shuffle_epi32(v.val[0], _SWAP_HI_LOW32);
    return v;
}
#endif

#if defined(USE_SSSE3)
uint16x4x2_t vld2_u16(__transfersize(8) uint16_t const * ptr);         // VLD2.16 {d0, d1}, [r0]
_NEON2SSE_INLINE uint16x4x2_t vld2_u16(__transfersize(8) uint16_t const * ptr)
{
    uint16x4x2_t v;
    _NEON2SSE_ALIGN_16 int8_t mask16_even_odd[16] = { 0,1, 4,5, 8,9, 12,13, 2,3, 6,7, 10,11, 14,15};
    __m128i ld128;
    ld128 = vld1q_u16(ptr);         //merge two 64-bits in 128 bit
    v.val[0] = _mm_shuffle_epi8(ld128, *(__m128i*)mask16_even_odd);
    v.val[1] = _mm_shuffle_epi32(v.val[0], _SWAP_HI_LOW32);
    return v;
}
#endif

uint32x2x2_t vld2_u32(__transfersize(4) uint32_t const * ptr);         // VLD2.32 {d0, d1}, [r0]
_NEON2SSE_INLINE uint32x2x2_t vld2_u32(__transfersize(4) uint32_t const * ptr)
{
    uint32x2x2_t v;
    __m128i ld128;
    ld128 = vld1q_u32(ptr);         //merge two 64-bits in 128 bit
    v.val[0] = _mm_shuffle_epi32(ld128,  0 | (2 << 2) | (1 << 4) | (3 << 6));
    v.val[1] = _mm_shuffle_epi32(v.val[0], _SWAP_HI_LOW32);
    return v;
}

uint64x1x2_t vld2_u64(__transfersize(2) uint64_t const * ptr);         // VLD1.64 {d0, d1}, [r0]
_NEON2SSE_INLINE uint64x1x2_t vld2_u64(__transfersize(2) uint64_t const * ptr)
{
    uint64x1x2_t v;
    v.val[0] = vld1q_u64(ptr);
    v.val[1] = _mm_shuffle_epi32(v.val[0], _SWAP_HI_LOW32);
    return v;
}

#if defined(USE_SSSE3)
int8x8x2_t vld2_s8(__transfersize(16) int8_t const * ptr);         // VLD2.8 {d0, d1}, [r0]
#define vld2_s8(ptr) vld2_u8((uint8_t*)ptr)

int16x4x2_t vld2_s16(__transfersize(8) int16_t const * ptr);         // VLD2.16 {d0, d1}, [r0]
#define vld2_s16(ptr) vld2_u16((uint16_t*)ptr)
#endif

int32x2x2_t vld2_s32(__transfersize(4) int32_t const * ptr);         // VLD2.32 {d0, d1}, [r0]
#define vld2_s32(ptr) vld2_u32((uint32_t*)ptr)

int64x1x2_t vld2_s64(__transfersize(2) int64_t const * ptr);         // VLD1.64 {d0, d1}, [r0]
#define vld2_s64(ptr) vld2_u64((uint64_t*)ptr)

float16x4x2_t vld2_f16(__transfersize(8) __fp16 const * ptr);         // VLD2.16 {d0, d1}, [r0]

float32x2x2_t vld2_f32(__transfersize(4) float32_t const * ptr);         // VLD2.32 {d0, d1}, [r0]
_NEON2SSE_INLINE float32x2x2_t vld2_f32(__transfersize(4) float32_t const * ptr)
{
    float32x2x2_t v;
    v.val[0] = vld1q_f32(ptr);
    v.val[0] = _mm_shuffle_ps(v.val[0], v.val[0], _MM_SHUFFLE(3,1, 2, 0));
    v.val[1] = _mm_movehl_ps(v.val[0],v.val[0]);
    return v;
}

#if defined(USE_SSSE3)
poly8x8x2_t vld2_p8(__transfersize(16) poly8_t const * ptr);         // VLD2.8 {d0, d1}, [r0]
#define vld2_p8 vld2_u8

poly16x4x2_t vld2_p16(__transfersize(8) poly16_t const * ptr);         // VLD2.16 {d0, d1}, [r0]
#define vld2_p16 vld2_u16
#endif

//******************** Triplets ***************************************
//*********************************************************************
#if defined(USE_SSSE3)
uint8x16x3_t vld3q_u8(__transfersize(48) uint8_t const * ptr);         // VLD3.8 {d0, d2, d4}, [r0]
_NEON2SSE_INLINE uint8x16x3_t vld3q_u8(__transfersize(48) uint8_t const * ptr)         // VLD3.8 {d0, d2, d4}, [r0]
{  //a0,a1,a2,a3,...a7,a8,...a15,  b0,b1,b2,...b7,b8,...b15, c0,c1,c2,...c7,c8,...c15 ->
   //a:0,3,6,9,12,15,b:2,5,8,11,14,  c:1,4,7,10,13
   //a:1,4,7,10,13,  b:0,3,6,9,12,15,c:2,5,8,11,14,
   //a:2,5,8,11,14,  b:1,4,7,10,13,  c:0,3,6,9,12,15
    uint8x16x3_t v;
    __m128i tmp0, tmp1,tmp2, tmp3;
    _NEON2SSE_ALIGN_16 int8_t mask8_0[16] = {0,3,6,9,12,15,1,4,7,10,13,2,5,8,11,14};
    _NEON2SSE_ALIGN_16 int8_t mask8_1[16] = {2,5,8,11,14,0,3,6,9,12,15,1,4,7,10,13};
    _NEON2SSE_ALIGN_16 int8_t mask8_2[16] = {1,4,7,10,13,2,5,8,11,14,0,3,6,9,12,15};

    v.val[0] =  vld1q_u8 (ptr);        //a0,a1,a2,a3,...a7, ...a15
    v.val[1] =  vld1q_u8 ((ptr + 16));	//b0,b1,b2,b3...b7, ...b15
    v.val[2] =  vld1q_u8 ((ptr + 32));  //c0,c1,c2,c3,...c7,...c15

    tmp0 = _mm_shuffle_epi8(v.val[0], *(__m128i*)mask8_0); //a:0,3,6,9,12,15,1,4,7,10,13,2,5,8,11
    tmp1 = _mm_shuffle_epi8(v.val[1], *(__m128i*)mask8_1); //b:2,5,8,11,14,0,3,6,9,12,15,1,4,7,10,13
    tmp2 = _mm_shuffle_epi8(v.val[2], *(__m128i*)mask8_2); //c:1,4,7,10,13,2,5,8,11,14,3,6,9,12,15

    tmp3 = _mm_slli_si128(tmp0,10); //0,0,0,0,0,0,0,0,0,0,a0,a3,a6,a9,a12,a15
    tmp3 = _mm_alignr_epi8(tmp1,tmp3, 10); //a:0,3,6,9,12,15,b:2,5,8,11,14,x,x,x,x,x
    tmp3 = _mm_slli_si128(tmp3, 5); //0,0,0,0,0,a:0,3,6,9,12,15,b:2,5,8,11,14,
    tmp3 = _mm_srli_si128(tmp3, 5); //a:0,3,6,9,12,15,b:2,5,8,11,14,:0,0,0,0,0
    v.val[0] = _mm_slli_si128(tmp2, 11); //0,0,0,0,0,0,0,0,0,0,0,0, 1,4,7,10,13,
    v.val[0] = _mm_or_si128(v.val[0],tmp3) ;//a:0,3,6,9,12,15,b:2,5,8,11,14,c:1,4,7,10,13,

    tmp3 = _mm_slli_si128(tmp0, 5);//0,0,0,0,0,a:0,3,6,9,12,15,1,4,7,10,13,
    tmp3 = _mm_srli_si128(tmp3, 11); //a:1,4,7,10,13, 0,0,0,0,0,0,0,0,0,0,0
    v.val[1] = _mm_srli_si128(tmp1,5); //b:0,3,6,9,12,15,C:1,4,7,10,13, 0,0,0,0,0
    v.val[1] = _mm_slli_si128(v.val[1], 5);//0,0,0,0,0,b:0,3,6,9,12,15,C:1,4,7,10,13,
    v.val[1] = _mm_or_si128(v.val[1],tmp3);//a:1,4,7,10,13,b:0,3,6,9,12,15,C:1,4,7,10,13,
    v.val[1] =	_mm_slli_si128(v.val[1],5);//0,0,0,0,0,a:1,4,7,10,13,b:0,3,6,9,12,15,
    v.val[1] = _mm_srli_si128(v.val[1], 5);//a:1,4,7,10,13,b:0,3,6,9,12,15,0,0,0,0,0
    tmp3 = _mm_srli_si128(tmp2,5); //c:2,5,8,11,14,0,3,6,9,12,15,0,0,0,0,0
    tmp3 = _mm_slli_si128(tmp3,11);//0,0,0,0,0,0,0,0,0,0,0,c:2,5,8,11,14,
    v.val[1] = _mm_or_si128(v.val[1],tmp3);//a:1,4,7,10,13,b:0,3,6,9,12,15,c:2,5,8,11,14,

    tmp3 = _mm_srli_si128(tmp2,10); //c:0,3,6,9,12,15, 0,0,0,0,0,0,0,0,0,0,
    tmp3 = _mm_slli_si128(tmp3,10); //0,0,0,0,0,0,0,0,0,0, c:0,3,6,9,12,15,
    v.val[2] = _mm_srli_si128(tmp1,11); //b:1,4,7,10,13,0,0,0,0,0,0,0,0,0,0,0
    v.val[2] = _mm_slli_si128(v.val[2],5);//0,0,0,0,0,b:1,4,7,10,13, 0,0,0,0,0,0
    v.val[2] = _mm_or_si128(v.val[2],tmp3);//0,0,0,0,0,b:1,4,7,10,13,c:0,3,6,9,12,15,
    tmp0 = _mm_srli_si128(tmp0, 11); //a:2,5,8,11,14, 0,0,0,0,0,0,0,0,0,0,0,
    v.val[2] = _mm_or_si128(v.val[2],tmp0);//a:2,5,8,11,14,b:1,4,7,10,13,c:0,3,6,9,12,15,
    return v;
}
#endif

#if defined(USE_SSSE3)
uint16x8x3_t vld3q_u16(__transfersize(24) uint16_t const * ptr);         // VLD3.16 {d0, d2, d4}, [r0]
_NEON2SSE_INLINE uint16x8x3_t vld3q_u16(__transfersize(24) uint16_t const * ptr)         // VLD3.16 {d0, d2, d4}, [r0]
{  //a0, a1,a2,a3,...a7,  b0,b1,b2,b3,...b7, c0,c1,c2,c3...c7 -> a0,a3,a6,b1,b4,b7,c2,c5, a1,a4,a7,b2,b5,c0,c3,c6, a2,a5,b0,b3,b6,c1,c4,c7
    uint16x8x3_t v;
    __m128i tmp0, tmp1,tmp2, tmp3;
    _NEON2SSE_ALIGN_16 int8_t mask16_0[16] = {0,1, 6,7, 12,13, 2,3, 8,9, 14,15, 4,5, 10,11};
    _NEON2SSE_ALIGN_16 int8_t mask16_1[16] = {2,3, 8,9, 14,15, 4,5, 10,11, 0,1, 6,7, 12,13};
    _NEON2SSE_ALIGN_16 int8_t mask16_2[16] = {4,5, 10,11, 0,1, 6,7, 12,13, 2,3, 8,9, 14,15};

    v.val[0] =  vld1q_u16 (ptr);        //a0,a1,a2,a3,...a7,
    v.val[1] =  vld1q_u16 ((ptr + 8));	//b0,b1,b2,b3...b7
    v.val[2] =  vld1q_u16 ((ptr + 16));  //c0,c1,c2,c3,...c7

    tmp0 = _mm_shuffle_epi8(v.val[0], *(__m128i*)mask16_0); //a0,a3,a6,a1,a4,a7,a2,a5,
    tmp1 = _mm_shuffle_epi8(v.val[1], *(__m128i*)mask16_1); //b1,b4,b7,b2,b5,b0,b3,b6
    tmp2 = _mm_shuffle_epi8(v.val[2], *(__m128i*)mask16_2); //c2,c5, c0,c3,c6, c1,c4,c7

    tmp3 = _mm_slli_si128(tmp0,10); //0,0,0,0,0,a0,a3,a6,
    tmp3 = _mm_alignr_epi8(tmp1,tmp3, 10); //a0,a3,a6,b1,b4,b7,x,x
    tmp3 = _mm_slli_si128(tmp3, 4); //0,0, a0,a3,a6,b1,b4,b7
    tmp3 = _mm_srli_si128(tmp3, 4); //a0,a3,a6,b1,b4,b7,0,0
    v.val[0] = _mm_slli_si128(tmp2, 12); //0,0,0,0,0,0, c2,c5,
    v.val[0] = _mm_or_si128(v.val[0],tmp3);//a0,a3,a6,b1,b4,b7,c2,c5

    tmp3 = _mm_slli_si128(tmp0, 4);//0,0,a0,a3,a6,a1,a4,a7
    tmp3 = _mm_srli_si128(tmp3,10); //a1,a4,a7, 0,0,0,0,0
    v.val[1] = _mm_srli_si128(tmp1,6); //b2,b5,b0,b3,b6,0,0
    v.val[1] = _mm_slli_si128(v.val[1], 6); //0,0,0,b2,b5,b0,b3,b6,
    v.val[1] = _mm_or_si128(v.val[1],tmp3);//a1,a4,a7,b2,b5,b0,b3,b6,
    v.val[1] =	_mm_slli_si128(v.val[1],6);//0,0,0,a1,a4,a7,b2,b5,
    v.val[1] = _mm_srli_si128(v.val[1], 6);//a1,a4,a7,b2,b5,0,0,0,
    tmp3 = _mm_srli_si128(tmp2,4);  //c0,c3,c6, c1,c4,c7,0,0
    tmp3 = _mm_slli_si128(tmp3,10);  //0,0,0,0,0,c0,c3,c6,
    v.val[1] = _mm_or_si128(v.val[1],tmp3); //a1,a4,a7,b2,b5,c0,c3,c6,

    tmp3 = _mm_srli_si128(tmp2,10); //c1,c4,c7, 0,0,0,0,0
    tmp3 = _mm_slli_si128(tmp3,10); //0,0,0,0,0, c1,c4,c7,
    v.val[2] = _mm_srli_si128(tmp1,10); //b0,b3,b6,0,0, 0,0,0
    v.val[2] = _mm_slli_si128(v.val[2],4);//0,0, b0,b3,b6,0,0,0
    v.val[2] = _mm_or_si128(v.val[2],tmp3);//0,0, b0,b3,b6,c1,c4,c7,
    tmp0 = _mm_srli_si128(tmp0, 12); //a2,a5,0,0,0,0,0,0
    v.val[2] = _mm_or_si128(v.val[2],tmp0);//a2,a5,b0,b3,b6,c1,c4,c7,
    return v;
}
#endif

uint32x4x3_t vld3q_u32(__transfersize(12) uint32_t const * ptr);         // VLD3.32 {d0, d2, d4}, [r0]
_NEON2SSE_INLINE uint32x4x3_t vld3q_u32(__transfersize(12) uint32_t const * ptr)         // VLD3.32 {d0, d2, d4}, [r0]
{//a0,a1,a2,a3,  b0,b1,b2,b3, c0,c1,c2,c3 -> a0,a3,b2,c1,  a1,b0,b3,c2, a2,b1,c0,c3,
    uint32x4x3_t v;
    __m128i tmp0, tmp1,tmp2, tmp3;
    v.val[0] =  vld1q_u32 (ptr);        //a0,a1,a2,a3,
    v.val[1] =  vld1q_u32 ((ptr + 4));	//b0,b1,b2,b3
    v.val[2] =  vld1q_u32 ((ptr + 8));  //c0,c1,c2,c3,

    tmp0 = _mm_shuffle_epi32(v.val[0], 0 | (3 << 2) | (1 << 4) | (2 << 6)); //a0,a3,a1,a2
    tmp1 = _mm_shuffle_epi32(v.val[1], _SWAP_HI_LOW32); //b2,b3,b0,b1
    tmp2 = _mm_shuffle_epi32(v.val[2], 1 | (2 << 2) | (0 << 4) | (3 << 6)); //c1,c2, c0,c3

    tmp3 = _mm_unpacklo_epi32(tmp1, tmp2); //b2,c1, b3,c2
    v.val[0] = _mm_unpacklo_epi64(tmp0,tmp3); //a0,a3,b2,c1
    tmp0 = _mm_unpackhi_epi32(tmp0, tmp1); //a1,b0, a2,b1
    v.val[1] = _mm_shuffle_epi32(tmp0, _SWAP_HI_LOW32 ); //a2,b1, a1,b0,
    v.val[1] = _mm_unpackhi_epi64(v.val[1], tmp3); //a1,b0, b3,c2
    v.val[2] = _mm_unpackhi_epi64(tmp0, tmp2); //a2,b1, c0,c3
    return v;
}

#if defined(USE_SSSE3)
int8x16x3_t vld3q_s8(__transfersize(48) int8_t const * ptr);         // VLD3.8 {d0, d2, d4}, [r0]
#define  vld3q_s8(ptr) vld3q_u8((uint8_t*) (ptr))

int16x8x3_t vld3q_s16(__transfersize(24) int16_t const * ptr);         // VLD3.16 {d0, d2, d4}, [r0]
#define  vld3q_s16(ptr) vld3q_u16((uint16_t*) (ptr))
#endif

int32x4x3_t vld3q_s32(__transfersize(12) int32_t const * ptr);         // VLD3.32 {d0, d2, d4}, [r0]
#define  vld3q_s32(ptr) vld3q_u32((uint32_t*) (ptr))

float16x8x3_t vld3q_f16(__transfersize(24) __fp16 const * ptr);         // VLD3.16 {d0, d2, d4}, [r0]
// IA32 SIMD doesn't work with 16bit floats currently, so need to go to 32 bit and then work with two 128bit registers. See vld1q_f16 for example

float32x4x3_t vld3q_f32(__transfersize(12) float32_t const * ptr);         // VLD3.32 {d0, d2, d4}, [r0]
_NEON2SSE_INLINE float32x4x3_t vld3q_f32(__transfersize(12) float32_t const * ptr)         // VLD3.32 {d0, d2, d4}, [r0]
{ //a0,a1,a2,a3,  b0,b1,b2,b3, c0,c1,c2,c3 -> a0,a3,b2,c1,  a1,b0,b3,c2, a2,b1,c0,c3,
    float32x4x3_t v;
    __m128 tmp0, tmp1,tmp2, tmp3;
    v.val[0] =  vld1q_f32 (ptr);        //a0,a1,a2,a3,
    v.val[1] =  vld1q_f32 ((ptr + 4));	//b0,b1,b2,b3
    v.val[2] =  vld1q_f32 ((ptr + 8));  //c0,c1,c2,c3,

    tmp0 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.val[0]), 0 | (3 << 2) | (1 << 4) | (2 << 6))); //a0,a3,a1,a2
    tmp1 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.val[1]), _SWAP_HI_LOW32)); //b2,b3,b0,b1
    tmp2 = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(v.val[2]), 1 | (2 << 2) | (0 << 4) | (3 << 6))); //c1,c2, c0,c3
    tmp3 = _mm_unpacklo_ps(tmp1, tmp2); //b2,c1, b3,c2

    v.val[0] = _mm_movelh_ps(tmp0,tmp3); //a0,a3,b2,c1
    tmp0 = _mm_unpackhi_ps(tmp0, tmp1); //a1,b0, a2,b1
    v.val[1] = _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(tmp0), _SWAP_HI_LOW32 )); //a2,b1, a1,b0,
    v.val[1] = _mm_movehl_ps(tmp3,v.val[1]); //a1,b0, b3,c2
    v.val[2] = _mm_movehl_ps(tmp2,tmp0); //a2,b1, c0,c3
    return v;
}

#if defined(USE_SSSE3)
poly8x16x3_t vld3q_p8(__transfersize(48) poly8_t const * ptr);         // VLD3.8 {d0, d2, d4}, [r0]
#define vld3q_p8 vld3q_u8

poly16x8x3_t vld3q_p16(__transfersize(24) poly16_t const * ptr);         // VLD3.16 {d0, d2, d4}, [r0]
#define vld3q_p16 vld3q_u16
#endif

#if defined(USE_SSSE3)
uint8x8x3_t vld3_u8(__transfersize(24) uint8_t const * ptr);         // VLD3.8 {d0, d1, d2}, [r0]
_NEON2SSE_INLINE uint8x8x3_t vld3_u8(__transfersize(24) uint8_t const * ptr)         // VLD3.8 {d0, d1, d2}, [r0]
{ //a0, a1,a2,a3,...a7,  b0,b1,b2,b3,...b7, c0,c1,c2,c3...c7 -> a0,a3,a6,b1,b4,b7,c2,c5, a1,a4,a7,b2,b5,c0,c3,c6, a2,a5,b0,b3,b6,c1,c4,c7
    uint8x8x3_t v;
    __m128i tmp0, tmp1;
    _NEON2SSE_ALIGN_16 int8_t mask8_0[16] = {0,3,6,9,12,15, 1,4,7,10,13, 2,5,8,11,14};
    _NEON2SSE_ALIGN_16 int8_t mask8_1[16] = {2,5, 0,3,6, 1,4,7, 0,0,0,0,0,0,0,0};
    v.val[0] =  vld1q_u8 (ptr);        //a0,a1,a2,a3,...a7, b0,b1,b2,b3...b7

    tmp0 = _mm_shuffle_epi8(v.val[0], *(__m128i*)mask8_0); //a0,a3,a6,b1,b4,b7, a1,a4,a7,b2,b5, a2,a5,b0,b3,b6,
    tmp1 = _mm_shuffle_epi8(v.val[2], *(__m128i*)mask8_1); //c2,c5, c0,c3,c6, c1,c4,c7,x,x,x,x,x,x,x,x
    v.val[0] = _mm_slli_si128(tmp0,10);
    v.val[0] = _mm_srli_si128(v.val[0],10);  //a0,a3,a6,b1,b4,b7, 0,0,0,0,0,0,0,0,0,0
    v.val[2] = _mm_slli_si128(tmp1,6);//0,0,0,0,0,0,c2,c5,x,x,x,x,x,x,x,x
    v.val[0] = _mm_or_si128(v.val[0],v.val[2]) ;//a0,a3,a6,b1,b4,b7,c2,c5 x,x,x,x,x,x,x,x

    v.val[1] = _mm_slli_si128(tmp0,5);  //0,0,0,0,0,0,0,0,0,0,0, a1,a4,a7,b2,b5,
    v.val[1] = _mm_srli_si128(v.val[1],11);  //a1,a4,a7,b2,b5,0,0,0,0,0,0,0,0,0,0,0,
    v.val[2] = _mm_srli_si128(tmp1,2); //c0,c3,c6,c1,c4,c7,x,x,x,x,x,x,x,x,0,0
    v.val[2] = _mm_slli_si128(v.val[2],5);//0,0,0,0,0,c0,c3,c6,0,0,0,0,0,0,0,0
    v.val[1] = _mm_or_si128(v.val[1],v.val[2]) ;//a1,a4,a7,b2,b5,c0,c3,c6,x,x,x,x,x,x,x,x

    tmp0 = _mm_srli_si128(tmp0,11);  //a2,a5,b0,b3,b6,0,0,0,0,0,0,0,0,0,0,0,
    v.val[2] = _mm_srli_si128(tmp1,5); //c1,c4,c7,0,0,0,0,0,0,0,0,0,0,0,0,0
    v.val[2] = _mm_slli_si128(v.val[2],5);//0,0,0,0,0,c1,c4,c7,
    v.val[2] = _mm_or_si128(tmp0, v.val[2]) ;//a2,a5,b0,b3,b6,c1,c4,c7,x,x,x,x,x,x,x,x
    return v;
}
#endif

#if defined(USE_SSSE3)
uint16x4x3_t vld3_u16(__transfersize(12) uint16_t const * ptr);         // VLD3.16 {d0, d1, d2}, [r0]
_NEON2SSE_INLINE uint16x4x3_t vld3_u16(__transfersize(12) uint16_t const * ptr)         // VLD3.16 {d0, d1, d2}, [r0]
{ //a0,a1,a2,a3,  b0,b1,b2,b3, c0,c1,c2,c3 -> a0,a3,b2,c1,  a1,b0,b3,c2, a2,b1,c0,c3,
    uint16x4x3_t v;
    __m128i tmp0, tmp1;
    _NEON2SSE_ALIGN_16 int8_t mask16[16] = {0,1, 6,7, 12,13, 2,3, 8,9, 14,15, 4,5, 10,11};
    v.val[0] =  vld1q_u16 (ptr);        //a0,a1,a2,a3,  b0,b1,b2,b3

    tmp0 = _mm_shuffle_epi8(v.val[0], *(__m128i*)mask16); //a0, a3, b2,a1, b0, b3, a2, b1
    tmp1 = _mm_shufflelo_epi16(v.val[2], 201); //11 00 10 01     : c1, c2, c0, c3,
    v.val[0] = _mm_slli_si128(tmp0,10);
    v.val[0] = _mm_srli_si128(v.val[0],10);  //a0, a3, b2, 0,0, 0,0,
    v.val[2] = _mm_slli_si128(tmp1,14);//0,0,0,0,0,0,0,c1
    v.val[2] = _mm_srli_si128(v.val[2],8);//0,0,0,c1,0,0,0,0
    v.val[0] = _mm_or_si128(v.val[0],v.val[2]) ;//a0, a3, b2, c1, x,x,x,x

    v.val[1] = _mm_slli_si128(tmp0,4);  //0,0,0,0,0,a1, b0, b3
    v.val[1] = _mm_srli_si128(v.val[1],10);  //a1, b0, b3, 0,0, 0,0,
    v.val[2] = _mm_srli_si128(tmp1,2);//c2, 0,0,0,0,0,0,0,
    v.val[2] = _mm_slli_si128(v.val[2],6);//0,0,0,c2,0,0,0,0
    v.val[1] = _mm_or_si128(v.val[1],v.val[2]); //a1, b0, b3, c2, x,x,x,x

    tmp0 = _mm_srli_si128(tmp0,12); //a2, b1,0,0,0,0,0,0
    tmp1 = _mm_srli_si128(tmp1,4);
    tmp1 = _mm_slli_si128(tmp1,4);  //0,0,c0, c3,
    v.val[2] = _mm_or_si128(tmp0, tmp1); //a2, b1, c0, c3,
    return v;
}
#endif

uint32x2x3_t vld3_u32(__transfersize(6) uint32_t const * ptr);         // VLD3.32 {d0, d1, d2}, [r0]
_NEON2SSE_INLINE uint32x2x3_t vld3_u32(__transfersize(6) uint32_t const * ptr)         // VLD3.32 {d0, d1, d2}, [r0]
{ //a0,a1,  b0,b1, c0,c1,  -> a0,b1, a1,c0, b0,c1
    uint32x2x3_t v;
    v.val[0] =  vld1q_u32 (ptr);        //a0,a1,  b0,b1,

    v.val[0] = _mm_shuffle_epi32(v.val[0], 0 | (3 << 2) | (1 << 4) | (2 << 6));  //a0,b1, a1, b0
    v.val[2] =  _mm_slli_si128(v.val[2], 8);  //x, x,c0,c1,
    v.val[1] =  _mm_unpackhi_epi32(v.val[0],v.val[2]); //a1,c0, b0, c1
    v.val[2] =  _mm_srli_si128(v.val[1], 8);  //b0, c1, x, x,
    return v;
}
uint64x1x3_t vld3_u64(__transfersize(3) uint64_t const * ptr);         // VLD1.64 {d0, d1, d2}, [r0]
_NEON2SSE_INLINE uint64x1x3_t vld3_u64(__transfersize(3) uint64_t const * ptr)         // VLD1.64 {d0, d1, d2}, [r0]
{
    uint64x1x3_t v;
    v.val[0] = vld1q_u64 (ptr);
    v.val[1] = _mm_shuffle_epi32(v.val[0], _SWAP_HI_LOW32);
    return v;
}

#if defined(USE_SSSE3)
int8x8x3_t vld3_s8(__transfersize(24) int8_t const * ptr);         // VLD3.8 {d0, d1, d2}, [r0]
#define vld3_s8(ptr) vld3_u8((uint8_t*)ptr)

int16x4x3_t vld3_s16(__transfersize(12) int16_t const * ptr);         // VLD3.16 {d0, d1, d2}, [r0]
#define vld3_s16(ptr) vld3_u16((uint16_t*)ptr)
#endif

int32x2x3_t vld3_s32(__transfersize(6) int32_t const * ptr);         // VLD3.32 {d0, d1, d2}, [r0]
#define vld3_s32(ptr) vld3_u32((uint32_t*)ptr)

int64x1x3_t vld3_s64(__transfersize(3) int64_t const * ptr);         // VLD1.64 {d0, d1, d2}, [r0]
#define vld3_s64(ptr) vld3_u64((uint64_t*)ptr)

float16x4x3_t vld3_f16(__transfersize(12) __fp16 const * ptr);         // VLD3.16 {d0, d1, d2}, [r0]
// IA32 SIMD doesn't work with 16bit floats currently, so need to go to 32 bit and then work with two 128bit registers. See vld1q_f16 for example

float32x2x3_t vld3_f32(__transfersize(6) float32_t const * ptr);         // VLD3.32 {d0, d1, d2}, [r0]
_NEON2SSE_INLINE float32x2x3_t vld3_f32(__transfersize(6) float32_t const * ptr)
{ //a0,a1,  b0,b1, c0,c1,  -> a0,b1, a1,c0, b0,c1
    float32x2x3_t v;
    v.val[0] =  vld1q_f32 (ptr);        //a0,a1,  b0,b1,

    v.val[0] = _mm_shuffle_ps(v.val[0],v.val[0], _MM_SHUFFLE(2,1, 3, 0));  //a0,b1, a1, b0
    v.val[2] =  _mm_movelh_ps(v.val[2], v.val[2]);  //x, x,c0,c1,
    v.val[1] =  _mm_unpackhi_ps(v.val[0],v.val[2]); //a1,c0, b0, c1
    v.val[2] =  _mm_movehl_ps(v.val[1], v.val[1]);  //b0, c1, x, x,
    return v;
}

#if defined(USE_SSSE3)
poly8x8x3_t vld3_p8(__transfersize(24) poly8_t const * ptr);         // VLD3.8 {d0, d1, d2}, [r0]
#define vld3_p8 vld3_u8

poly16x4x3_t vld3_p16(__transfersize(12) poly16_t const * ptr);         // VLD3.16 {d0, d1, d2}, [r0]
#define vld3_p16 vld3_u16
#endif

//***************  Quadruples load ********************************
//*****************************************************************
uint8x16x4_t vld4q_u8(__transfersize(64) uint8_t const * ptr);         // VLD4.8 {d0, d2, d4, d6}, [r0]
_NEON2SSE_INLINE uint8x16x4_t vld4q_u8(__transfersize(64) uint8_t const * ptr)         // VLD4.8 {d0, d2, d4, d6}, [r0]
{
    uint8x16x4_t v;
   __m128i tmp3, tmp2, tmp1, tmp0;

    v.val[0] = vld1q_u8 ( ptr); //a0,a1,a2,...a7, ...a15
    v.val[1] = vld1q_u8 ( (ptr + 16));//b0, b1,b2,...b7.... b15
    v.val[2] = vld1q_u8 ( (ptr + 32));//c0, c1,c2,...c7....c15
    v.val[3] = vld1q_u8 ( (ptr + 48)); //d0,d1,d2,...d7....d15

    tmp0= _mm_unpacklo_epi8(v.val[0],v.val[1]); //a0,b0, a1,b1, a2,b2, a3,b3,....a7,b7
    tmp1= _mm_unpacklo_epi8(v.val[2],v.val[3]); //c0,d0, c1,d1, c2,d2, c3,d3,... c7,d7
    tmp2= _mm_unpackhi_epi8(v.val[0],v.val[1]);//a8,b8, a9,b9, a10,b10, a11,b11,...a15,b15
    tmp3= _mm_unpackhi_epi8(v.val[2],v.val[3]);//c8,d8, c9,d9, c10,d10, c11,d11,...c15,d15

    v.val[0] = _mm_unpacklo_epi8(tmp0, tmp2); //a0,a8, b0,b8,  a1,a9, b1,b9, ....a3,a11, b3,b11
    v.val[1] = _mm_unpackhi_epi8(tmp0, tmp2); //a4,a12, b4,b12, a5,a13, b5,b13,....a7,a15,b7,b15
    v.val[2] = _mm_unpacklo_epi8(tmp1, tmp3); //c0,c8, d0,d8, c1,c9, d1,d9.....d3,d11
    v.val[3] = _mm_unpackhi_epi8(tmp1, tmp3); //c4,c12,d4,d12, c5,c13, d5,d13,....d7,d15

    tmp0 =  _mm_unpacklo_epi32(v.val[0] , v.val[2] ); ///a0,a8, b0,b8, c0,c8,  d0,d8, a1,a9, b1,b9, c1,c9, d1,d9
    tmp1 =  _mm_unpackhi_epi32(v.val[0] , v.val[2] ); //a2,a10, b2,b10, c2,c10, d2,d10, a3,a11, b3,b11, c3,c11, d3,d11
    tmp2 =  _mm_unpacklo_epi32(v.val[1] , v.val[3] ); //a4,a12, b4,b12, c4,c12, d4,d12, a5,a13, b5,b13, c5,c13, d5,d13,
    tmp3 =  _mm_unpackhi_epi32(v.val[1] , v.val[3] ); //a6,a14, b6,b14, c6,c14, d6,d14, a7,a15,b7,b15,c7,c15,d7,d15

    v.val[0] = _mm_unpacklo_epi8(tmp0, tmp2); //a0,a4,a8,a12,b0,b4,b8,b12,c0,c4,c8,c12,d0,d4,d8,d12
    v.val[1] = _mm_unpackhi_epi8(tmp0, tmp2); //a1,a5, a9, a13, b1,b5, b9,b13, c1,c5, c9, c13, d1,d5, d9,d13
    v.val[2] = _mm_unpacklo_epi8(tmp1, tmp3); //a2,a6, a10,a14, b2,b6, b10,b14,c2,c6, c10,c14, d2,d6, d10,d14
    v.val[3] = _mm_unpackhi_epi8(tmp1, tmp3); //a3,a7, a11,a15, b3,b7, b11,b15,c3,c7, c11, c15,d3,d7, d11,d15
    return v;
}

uint16x8x4_t vld4q_u16(__transfersize(32) uint16_t const * ptr);         // VLD4.16 {d0, d2, d4, d6}, [r0]
_NEON2SSE_INLINE uint16x8x4_t vld4q_u16(__transfersize(32) uint16_t const * ptr)         // VLD4.16 {d0, d2, d4, d6}, [r0]
{
    uint16x8x4_t v;
    __m128i tmp3, tmp2, tmp1, tmp0;
    tmp0  =  vld1q_u16 (ptr);       //a0,a1,a2,...a7
    tmp1  =  vld1q_u16 ((ptr + 8)); //b0, b1,b2,...b7
    tmp2  =  vld1q_u16 ((ptr + 16)); //c0, c1,c2,...c7
    tmp3  =  vld1q_u16 ((ptr + 24)); //d0,d1,d2,...d7
    v.val[0]= _mm_unpacklo_epi16(tmp0,tmp1); //a0,b0, a1,b1, a2,b2, a3,b3,
    v.val[1]= _mm_unpacklo_epi16(tmp2,tmp3); //c0,d0, c1,d1, c2,d2, c3,d3,
    v.val[2]= _mm_unpackhi_epi16(tmp0,tmp1);//a4,b4, a5,b5, a6,b6, a7,b7
    v.val[3]= _mm_unpackhi_epi16(tmp2,tmp3);//c4,d4, c5,d5, c6,d6, c7,d7
    tmp0 = _mm_unpacklo_epi16(v.val[0], v.val[2]);//a0,a4, b0,b4, a1,a5, b1,b5
    tmp1 = _mm_unpackhi_epi16(v.val[0], v.val[2]); //a2,a6, b2,b6, a3,a7, b3,b7
    tmp2 = _mm_unpacklo_epi16(v.val[1], v.val[3]); //c0,c4, d0,d4, c1,c5, d1,d5
    tmp3 = _mm_unpackhi_epi16(v.val[1], v.val[3]);//c2,c6, d2,d6, c3,c7, d3,d7
    v.val[0] =  _mm_unpacklo_epi64(tmp0, tmp2); //a0,a4, b0,b4, c0,c4, d0,d4,
    v.val[1] =  _mm_unpackhi_epi64(tmp0, tmp2); //a1,a5, b1,b5, c1,c5, d1,d5
    v.val[2] =  _mm_unpacklo_epi64(tmp1, tmp3); //a2,a6, b2,b6, c2,c6, d2,d6,
    v.val[3] =  _mm_unpackhi_epi64(tmp1, tmp3); //a3,a7, b3,b7, c3,c7, d3,d7
    return v;
}

uint32x4x4_t vld4q_u32(__transfersize(16) uint32_t const * ptr);         // VLD4.32 {d0, d2, d4, d6}, [r0]
_NEON2SSE_INLINE uint32x4x4_t vld4q_u32(__transfersize(16) uint32_t const * ptr)         // VLD4.32 {d0, d2, d4, d6}, [r0]
{
    uint32x4x4_t v;
    __m128i tmp3, tmp2, tmp1, tmp0;
    v.val[0] =  vld1q_u32 (ptr);
    v.val[1] =  vld1q_u32 ((ptr + 4));
    v.val[2] =  vld1q_u32 ((ptr + 8));
    v.val[3] =  vld1q_u32 ((ptr + 12));
    tmp0 = _mm_unpacklo_epi32(v.val[0],v.val[1]);
    tmp1 = _mm_unpacklo_epi32(v.val[2],v.val[3]);
    tmp2 = _mm_unpackhi_epi32(v.val[0],v.val[1]);
    tmp3 = _mm_unpackhi_epi32(v.val[2],v.val[3]);
    v.val[0] = _mm_unpacklo_epi64(tmp0, tmp1);
    v.val[1] = _mm_unpackhi_epi64(tmp0, tmp1);
    v.val[2] = _mm_unpacklo_epi64(tmp2, tmp3);
    v.val[3] = _mm_unpackhi_epi64(tmp2, tmp3);
    return v;
}

int8x16x4_t vld4q_s8(__transfersize(64) int8_t const * ptr);         // VLD4.8 {d0, d2, d4, d6}, [r0]
#define vld4q_s8(ptr) vld4q_u8((uint8_t*)ptr)

int16x8x4_t vld4q_s16(__transfersize(32) int16_t const * ptr);         // VLD4.16 {d0, d2, d4, d6}, [r0]
#define  vld4q_s16(ptr) vld4q_u16((uint16_t*)ptr)

int32x4x4_t vld4q_s32(__transfersize(16) int32_t const * ptr);         // VLD4.32 {d0, d2, d4, d6}, [r0]
#define  vld4q_s32(ptr) vld4q_u32((uint32_t*)ptr)

float16x8x4_t vld4q_f16(__transfersize(32) __fp16 const * ptr);         // VLD4.16 {d0, d2, d4, d6}, [r0]
// IA32 SIMD doesn't work with 16bit floats currently, so need to go to 32 bit and then work with two 128bit registers. See vld1q_f16 for example

float32x4x4_t vld4q_f32(__transfersize(16) float32_t const * ptr);         // VLD4.32 {d0, d2, d4, d6}, [r0]
_NEON2SSE_INLINE float32x4x4_t vld4q_f32(__transfersize(16) float32_t const * ptr)         // VLD4.32 {d0, d2, d4, d6}, [r0]
{
    float32x4x4_t v;
    __m128 tmp3, tmp2, tmp1, tmp0;

    v.val[0] =  vld1q_f32 ((float*) ptr);
    v.val[1] =  vld1q_f32 ((float*) (ptr + 4));
    v.val[2] =  vld1q_f32 ((float*) (ptr + 8));
    v.val[3] =  vld1q_f32 ((float*) (ptr + 12));
    tmp0 = _mm_unpacklo_ps(v.val[0], v.val[1]);
    tmp2 = _mm_unpacklo_ps(v.val[2], v.val[3]);
    tmp1 = _mm_unpackhi_ps(v.val[0], v.val[1]);
    tmp3 = _mm_unpackhi_ps(v.val[2], v.val[3]);
    v.val[0] = _mm_movelh_ps(tmp0, tmp2);
    v.val[1] = _mm_movehl_ps(tmp2, tmp0);
    v.val[2] = _mm_movelh_ps(tmp1, tmp3);
    v.val[3] = _mm_movehl_ps(tmp3, tmp1);
    return v;
}

poly8x16x4_t vld4q_p8(__transfersize(64) poly8_t const * ptr);         // VLD4.8 {d0, d2, d4, d6}, [r0]
#define vld4q_p8 vld4q_u8

poly16x8x4_t vld4q_p16(__transfersize(32) poly16_t const * ptr);         // VLD4.16 {d0, d2, d4, d6}, [r0]
#define vld4q_p16 vld4q_s16

#if defined(USE_SSSE3)
uint8x8x4_t vld4_u8(__transfersize(32) uint8_t const * ptr);         // VLD4.8 {d0, d1, d2, d3}, [r0]
_NEON2SSE_INLINE uint8x8x4_t vld4_u8(__transfersize(32) uint8_t const * ptr)         // VLD4.8 {d0, d1, d2, d3}, [r0]
{
    uint8x8x4_t v;
    __m128i sh0, sh1;
    _NEON2SSE_ALIGN_16 int8_t mask4_8[16] = {0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15};

    v.val[0] = vld1q_u8(( ptr));         //load first 64-bits in val[0] and val[1]
    v.val[1] = vld1q_u8(( ptr + 16));         //load third and forth 64-bits in val[2], val[3]

    sh0 = _mm_shuffle_epi8(v.val[0], *(__m128i*)mask4_8);
    sh1 = _mm_shuffle_epi8(v.val[1], *(__m128i*)mask4_8);
    v.val[0] = _mm_unpacklo_epi32(sh0,sh1);         //0,4,8,12,16,20,24,28, 1,5,9,13,17,21,25,29
    v.val[2] = _mm_unpackhi_epi32(sh0,sh1);         //2,6,10,14,18,22,26,30, 3,7,11,15,19,23,27,31
    v.val[1] = _mm_shuffle_epi32(v.val[0],_SWAP_HI_LOW32);
    v.val[3] = _mm_shuffle_epi32(v.val[2],_SWAP_HI_LOW32);

    return v;
}
#endif

#if defined(USE_SSSE3)
uint16x4x4_t vld4_u16(__transfersize(16) uint16_t const * ptr);         // VLD4.16 {d0, d1, d2, d3}, [r0]
_NEON2SSE_INLINE uint16x4x4_t vld4_u16(__transfersize(16) uint16_t const * ptr)         // VLD4.16 {d0, d1, d2, d3}, [r0]
{
    uint16x4x4_t v;
    __m128i sh0, sh1;
    _NEON2SSE_ALIGN_16 int8_t mask4_16[16] = {0,1, 8,9, 2,3, 10,11, 4,5, 12,13, 6,7, 14,15};         //0, 4, 1, 5, 2, 6, 3, 7
    v.val[0] = vld1q_u16 ( (ptr));         //load first 64-bits in val[0] and val[1]
    v.val[2] = vld1q_u16 ( (ptr + 8));         //load third and forth 64-bits in val[2], val[3]
    sh0 = _mm_shuffle_epi8(v.val[0], *(__m128i*)mask4_16);
    sh1 = _mm_shuffle_epi8(v.val[2], *(__m128i*)mask4_16);
    v.val[0] = _mm_unpacklo_epi32(sh0,sh1);         //0,4,8,12, 1,5,9,13
    v.val[2] = _mm_unpackhi_epi32(sh0,sh1);         //2,6,10,14, 3,7,11,15
    v.val[1] = _mm_shuffle_epi32(v.val[0],_SWAP_HI_LOW32);
    v.val[3] = _mm_shuffle_epi32(v.val[2],_SWAP_HI_LOW32);
    return v;
}
#endif

uint32x2x4_t vld4_u32(__transfersize(8) uint32_t const * ptr);         // VLD4.32 {d0, d1, d2, d3}, [r0]
_NEON2SSE_INLINE uint32x2x4_t vld4_u32(__transfersize(8) uint32_t const * ptr)
{   //a0,a1,  b0,b1, c0,c1, d0,d1 -> a0,c0, a1,c1, b0,d0, b1,d1
     uint32x4x4_t v, res;
    v.val[0] =  vld1q_u32 (ptr);        //a0,a1,  b0,b1,
    v.val[2] =  vld1q_u32 ((ptr + 4));  //c0,c1, d0,d1
    res.val[0] = _mm_unpacklo_epi32(v.val[0],v.val[2]);  //a0, c0, a1,c1,
    res.val[2] = _mm_unpackhi_epi32(v.val[0],v.val[2]);  //b0,d0, b1, d1
    res.val[1] = _mm_shuffle_epi32(res.val[0],_SWAP_HI_LOW32); //a1,c1, a0, c0,
    res.val[3] = _mm_shuffle_epi32(res.val[2],_SWAP_HI_LOW32);//b1, d1,b0,d0,
    return res;
}

uint64x1x4_t vld4_u64(__transfersize(4) uint64_t const * ptr);         // VLD1.64 {d0, d1, d2, d3}, [r0]
_NEON2SSE_INLINE uint64x1x4_t vld4_u64(__transfersize(4) uint64_t const * ptr)         // VLD1.64 {d0, d1, d2, d3}, [r0]
{
    uint64x1x4_t v;
    v.val[0] = vld1q_u64( (ptr));         //load first 64-bits in val[0] and val[1]
    v.val[2] = vld1q_u64( (ptr + 2));         //load third and forth 64-bits in val[2], val[3]
    return v;
}

#if defined(USE_SSSE3)
int8x8x4_t vld4_s8(__transfersize(32) int8_t const * ptr);         // VLD4.8 {d0, d1, d2, d3}, [r0]
#define  vld4_s8(ptr) vld4_u8((uint8_t*)ptr)

int16x4x4_t vld4_s16(__transfersize(16) int16_t const * ptr);         // VLD4.16 {d0, d1, d2, d3}, [r0]
#define vld4_s16(ptr) vld4_u16((uint16_t*)ptr)
#endif

int32x2x4_t vld4_s32(__transfersize(8) int32_t const * ptr);         // VLD4.32 {d0, d1, d2, d3}, [r0]
#define vld4_s32(ptr) vld4_u32((uint32_t*)ptr)

int64x1x4_t vld4_s64(__transfersize(4) int64_t const * ptr);         // VLD1.64 {d0, d1, d2, d3}, [r0]
#define vld4_s64(ptr) vld4_u64((uint64_t*)ptr)

float16x4x4_t vld4_f16(__transfersize(16) __fp16 const * ptr);         // VLD4.16 {d0, d1, d2, d3}, [r0]
// IA32 SIMD doesn't work with 16bit floats currently, so need to go to 32 bit and then work with two 128bit registers. See vld1q_f16 for example

float32x2x4_t vld4_f32(__transfersize(8) float32_t const * ptr);         // VLD4.32 {d0, d1, d2, d3}, [r0]
_NEON2SSE_INLINE float32x2x4_t vld4_f32(__transfersize(8) float32_t const * ptr)         // VLD4.32 {d0, d1, d2, d3}, [r0]
{         //a0,a1,  b0,b1, c0,c1, d0,d1 -> a0,c0, a1,c1, b0,d0, b1,d1
    float32x2x4_t v, res;
    v.val[0] =  vld1q_f32 ((float*) ptr);         //a0,a1,  b0,b1,
    v.val[2] =  vld1q_f32 ((float*) (ptr + 4));         //c0,c1, d0,d1
    res.val[0] = _mm_unpacklo_ps(v.val[0],v.val[2]);         //a0, c0, a1,c1,
    res.val[2] = _mm_unpackhi_ps(v.val[0],v.val[2]);         //b0,d0, b1, d1
    res.val[1] = _mm_movehl_ps(res.val[0],res.val[0]);          // a1,c1, a0, c0,
    res.val[3] = _mm_movehl_ps(res.val[2],res.val[2]);         // b1, d1, b0,d0,
    return res;
}

#if defined(USE_SSSE3)
poly8x8x4_t vld4_p8(__transfersize(32) poly8_t const * ptr);         // VLD4.8 {d0, d1, d2, d3}, [r0]
#define vld4_p8 vld4_u8

poly16x4x4_t vld4_p16(__transfersize(16) poly16_t const * ptr);         // VLD4.16 {d0, d1, d2, d3}, [r0]
#define vld4_p16 vld4_u16
#endif

//************* Duplicate (or propagate) ptr[0] to all val[0] lanes and ptr[1] to all val[1] lanes *******************
//*******************************************************************************************************************
uint8x8x2_t vld2_dup_u8(__transfersize(2) uint8_t const * ptr);         // VLD2.8 {d0[], d1[]}, [r0]
_NEON2SSE_INLINE uint8x8x2_t vld2_dup_u8(__transfersize(2) uint8_t const * ptr)         // VLD2.8 {d0[], d1[]}, [r0]
{
    uint8x8x2_t v;
    v.val[0] = LOAD_SI128(ptr); //0,1,x,x, x,x,x,x,x,x,x,x, x,x,x,x
    v.val[1] = _mm_unpacklo_epi8(v.val[0],v.val[0]);//0,0,1,1,x,x,x,x, x,x,x,x,x,x,x,x,
    v.val[1] = _mm_unpacklo_epi16(v.val[1],v.val[1]);//0,0,0,0, 1,1,1,1,x,x,x,x, x,x,x,x
    v.val[0] = _mm_unpacklo_epi32(v.val[1],v.val[1]);//0,0,0,0, 0,0,0,0,1,1,1,1,1,1,1,1,
    v.val[1] = _mm_shuffle_epi32(v.val[0], _SWAP_HI_LOW32);
    return v;
}

uint16x4x2_t vld2_dup_u16(__transfersize(2) uint16_t const * ptr);         // VLD2.16 {d0[], d1[]}, [r0]
_NEON2SSE_INLINE uint16x4x2_t vld2_dup_u16(__transfersize(2) uint16_t const * ptr)         // VLD2.16 {d0[], d1[]}, [r0]
{
    uint16x4x2_t v;
    v.val[1] = LOAD_SI128(ptr); //0,1,x,x, x,x,x,x
    v.val[0] = _mm_shufflelo_epi16(v.val[1], 0); //00 00 00 00 (all 0)
    v.val[1] = _mm_shufflelo_epi16(v.val[1], 85);//01 01 01 01 (all 1)
    return v;
}

uint32x2x2_t vld2_dup_u32(__transfersize(2) uint32_t const * ptr);         // VLD2.32 {d0[], d1[]}, [r0]
_NEON2SSE_INLINE uint32x2x2_t vld2_dup_u32(__transfersize(2) uint32_t const * ptr)         // VLD2.32 {d0[], d1[]}, [r0]
{
    uint32x2x2_t v;
    v.val[0] = LOAD_SI128(ptr); //0,1,x,x
    v.val[0] = _mm_shuffle_epi32(v.val[0],   0 | (0 << 2) | (1 << 4) | (1 << 6)); //0,0,1,1
    v.val[1] = _mm_srli_si128(v.val[0], 8); //1,1,0x0,0x0
    return v;
}

uint64x1x2_t vld2_dup_u64(__transfersize(2) uint64_t const * ptr);         // VLD1.64 {d0, d1}, [r0]
#define vld2_dup_u64 vld2_u64

int8x8x2_t vld2_dup_s8(__transfersize(2) int8_t const * ptr);         // VLD2.8 {d0[], d1[]}, [r0]
#define vld2_dup_s8(ptr) vld2_dup_u8((uint8_t*)ptr)

int16x4x2_t vld2_dup_s16(__transfersize(2) int16_t const * ptr);         // VLD2.16 {d0[], d1[]}, [r0]
#define vld2_dup_s16(ptr) vld2_dup_u16((uint16_t*)ptr)

int32x2x2_t vld2_dup_s32(__transfersize(2) int32_t const * ptr);         // VLD2.32 {d0[], d1[]}, [r0]
#define vld2_dup_s32(ptr) vld2_dup_u32((uint32_t*)ptr)

int64x1x2_t vld2_dup_s64(__transfersize(2) int64_t const * ptr);         // VLD1.64 {d0, d1}, [r0]
#define vld2_dup_s64(ptr) vld2_dup_u64((uint64_t*)ptr)

float16x4x2_t vld2_dup_f16(__transfersize(2) __fp16 const * ptr);         // VLD2.16 {d0[], d1[]}, [r0]
// IA32 SIMD doesn't work with 16bit floats currently, so need to go to 32 bit and then work with two 128bit registers. See vld1q_f16 for example

float32x2x2_t vld2_dup_f32(__transfersize(2) float32_t const * ptr);         // VLD2.32 {d0[], d1[]}, [r0]
_NEON2SSE_INLINE float32x2x2_t vld2_dup_f32(__transfersize(2) float32_t const * ptr)         // VLD2.32 {d0[], d1[]}, [r0]
{
    float32x2x2_t v;
    v.val[0] = vld1q_f32(ptr);  //0,1,x,x
    v.val[1] = _mm_movehdup_ps(v.val[0]); //1,1,x,x
    v.val[0] = _mm_moveldup_ps(v.val[0]); //0,0,x,x
    return v;
}

poly8x8x2_t vld2_dup_p8(__transfersize(2) poly8_t const * ptr);         // VLD2.8 {d0[], d1[]}, [r0]
#define vld2_dup_p8 vld2_dup_u8

poly16x4x2_t vld2_dup_p16(__transfersize(2) poly16_t const * ptr);         // VLD2.16 {d0[], d1[]}, [r0]
#define vld2_dup_p16 vld2_dup_s16

//************* Duplicate (or propagate)triplets: *******************
//********************************************************************
//ptr[0] to all val[0] lanes, ptr[1] to all val[1] lanes and ptr[2] to all val[2] lanes
uint8x8x3_t vld3_dup_u8(__transfersize(3) uint8_t const * ptr);         // VLD3.8 {d0[], d1[], d2[]}, [r0]
_NEON2SSE_INLINE uint8x8x3_t vld3_dup_u8(__transfersize(3) uint8_t const * ptr)         // VLD3.8 {d0[], d1[], d2[]}, [r0]
{
    uint8x8x3_t v;
    v.val[0] = LOAD_SI128(ptr); //0,1,2,x, x,x,x,x,x,x,x,x, x,x,x,x
    v.val[1] = _mm_unpacklo_epi8(v.val[0],v.val[0]);//0,0,1,1,2,2,x,x, x,x,x,x,x,x,x,x,
    v.val[1] = _mm_unpacklo_epi16(v.val[1],v.val[1]);//0,0,0,0, 1,1,1,1,2,2,2,2,x,x,x,x,
    v.val[0] = _mm_unpacklo_epi32(v.val[1],v.val[1]);//0,0,0,0, 0,0,0,0,1,1,1,1,1,1,1,1,
    v.val[2] = _mm_unpackhi_epi32(v.val[1],v.val[1]);// 2,2,2,2,2,2,2,2, x,x,x,x,x,x,x,x,
    v.val[1] = _mm_shuffle_epi32(v.val[0], _SWAP_HI_LOW32);
    return v;
}

uint16x4x3_t vld3_dup_u16(__transfersize(3) uint16_t const * ptr);         // VLD3.16 {d0[], d1[], d2[]}, [r0]
_NEON2SSE_INLINE uint16x4x3_t vld3_dup_u16(__transfersize(3) uint16_t const * ptr)         // VLD3.16 {d0[], d1[], d2[]}, [r0]
{
    uint16x4x3_t v;
    v.val[2] = LOAD_SI128(ptr); //0,1,2,x, x,x,x,x
    v.val[0] = _mm_shufflelo_epi16(v.val[2], 0); //00 00 00 00 (all 0)
    v.val[1] = _mm_shufflelo_epi16(v.val[2], 85);//01 01 01 01 (all 1)
    v.val[2] = _mm_shufflelo_epi16(v.val[2], 170);//10 10 10 10 (all 2)
    return v;
}

uint32x2x3_t vld3_dup_u32(__transfersize(3) uint32_t const * ptr);         // VLD3.32 {d0[], d1[], d2[]}, [r0]
_NEON2SSE_INLINE uint32x2x3_t vld3_dup_u32(__transfersize(3) uint32_t const * ptr)         // VLD3.32 {d0[], d1[], d2[]}, [r0]
{
    uint32x2x3_t v;
    v.val[2] = LOAD_SI128(ptr); //0,1,2,x
    v.val[0] = _mm_shuffle_epi32(v.val[2],   0 | (0 << 2) | (2 << 4) | (2 << 6)); //0,0,2,2
    v.val[1] = _mm_shuffle_epi32(v.val[2],   1 | (1 << 2) | (2 << 4) | (2 << 6)); //1,1,2,2
    v.val[2] = _mm_srli_si128(v.val[0], 8); //2,2,0x0,0x0
    return v;
}

uint64x1x3_t vld3_dup_u64(__transfersize(3) uint64_t const * ptr);         // VLD1.64 {d0, d1, d2}, [r0]
_NEON2SSE_INLINE uint64x1x3_t vld3_dup_u64(__transfersize(3) uint64_t const * ptr)         // VLD1.64 {d0, d1, d2}, [r0]
{
    uint64x1x3_t v;
    v.val[0] = LOAD_SI128(ptr);//0,1,
    v.val[1] = _mm_shuffle_epi32(v.val[0], _SWAP_HI_LOW32); //1,0
    v.val[2] = LOAD_SI128((ptr + 2));  //2,x
    return v;
}

int8x8x3_t vld3_dup_s8(__transfersize(3) int8_t const * ptr);         // VLD3.8 {d0[], d1[], d2[]}, [r0]
#define vld3_dup_s8(ptr) vld3_dup_u8((uint8_t*)ptr)

int16x4x3_t vld3_dup_s16(__transfersize(3) int16_t const * ptr);         // VLD3.16 {d0[], d1[], d2[]}, [r0]
#define vld3_dup_s16(ptr) vld3_dup_u16((uint16_t*)ptr)

int32x2x3_t vld3_dup_s32(__transfersize(3) int32_t const * ptr);         // VLD3.32 {d0[], d1[], d2[]}, [r0]
#define vld3_dup_s32(ptr) vld3_dup_u32((uint32_t*)ptr)

int64x1x3_t vld3_dup_s64(__transfersize(3) int64_t const * ptr);         // VLD1.64 {d0, d1, d2}, [r0]
#define vld3_dup_s64(ptr) vld3_dup_u64((uint64_t*)ptr)

float16x4x3_t vld3_dup_f16(__transfersize(3) __fp16 const * ptr);         // VLD3.16 {d0[], d1[], d2[]}, [r0]
// IA32 SIMD doesn't work with 16bit floats currently, so need to go to 32 bit and then work with two 128bit registers. See vld1q_f16 for example

float32x2x3_t vld3_dup_f32(__transfersize(3) float32_t const * ptr);         // VLD3.32 {d0[], d1[], d2[]}, [r0]
_NEON2SSE_INLINE float32x2x3_t vld3_dup_f32(__transfersize(3) float32_t const * ptr)         // VLD3.32 {d0[], d1[], d2[]}, [r0]
{
    float32x2x3_t v;
    v.val[0] = vld1q_f32(ptr);  //0,1,2,x
    v.val[1] = _mm_movehdup_ps(v.val[0]); //1,1,x,x
    v.val[0] = _mm_moveldup_ps(v.val[0]); //0,0,2,2
    v.val[2] = _mm_movehl_ps(v.val[0], v.val[0]); //2,2,0,0,
    return v;
}

poly8x8x3_t vld3_dup_p8(__transfersize(3) poly8_t const * ptr);         // VLD3.8 {d0[], d1[], d2[]}, [r0]
#define vld3_dup_p8 vld3_dup_u8

poly16x4x3_t vld3_dup_p16(__transfersize(3) poly16_t const * ptr);         // VLD3.16 {d0[], d1[], d2[]}, [r0]
#define vld3_dup_p16 vld3_dup_s16

//************* Duplicate (or propagate) quadruples: *******************
//***********************************************************************
//ptr[0] to all val[0] lanes, ptr[1] to all val[1] lanes, ptr[2] to all val[2] lanes  and  ptr[3] to all val[3] lanes
uint8x8x4_t vld4_dup_u8(__transfersize(4) uint8_t const * ptr);         // VLD4.8 {d0[], d1[], d2[], d3[]}, [r0]
_NEON2SSE_INLINE uint8x8x4_t vld4_dup_u8(__transfersize(4) uint8_t const * ptr)         // VLD4.8 {d0[], d1[], d2[], d3[]}, [r0]
{
    uint8x8x4_t v;
    v.val[0] = LOAD_SI128(ptr); //0,1,2,3, x,x,x,x,x,x,x,x, x,x,x,x
    v.val[1] = _mm_unpacklo_epi8(v.val[0],v.val[0]);//0,0,1,1,2,2,3,3, x,x,x,x,x,x,x,x,
    v.val[1] = _mm_unpacklo_epi16(v.val[1],v.val[1]);//0,0,0,0, 1,1,1,1,2,2,2,2,3,3,3,3
    v.val[0] = _mm_unpacklo_epi32(v.val[1],v.val[1]);//0,0,0,0, 0,0,0,0,1,1,1,1,1,1,1,1,
    v.val[2] = _mm_unpackhi_epi32(v.val[1],v.val[1]);// 2,2,2,2,2,2,2,2, 3,3,3,3, 3,3,3,3
    v.val[1] = _mm_shuffle_epi32(v.val[0], _SWAP_HI_LOW32);
    v.val[3] = _mm_shuffle_epi32(v.val[2], _SWAP_HI_LOW32);
    return v;
}

uint16x4x4_t vld4_dup_u16(__transfersize(4) uint16_t const * ptr);         // VLD4.16 {d0[], d1[], d2[], d3[]}, [r0]
_NEON2SSE_INLINE uint16x4x4_t vld4_dup_u16(__transfersize(4) uint16_t const * ptr)         // VLD4.16 {d0[], d1[], d2[], d3[]}, [r0]
{
    uint16x4x4_t v;
    v.val[3] = LOAD_SI128(ptr); //0,1,2,3, x,x,x,x
    v.val[0] = _mm_shufflelo_epi16(v.val[3], 0); //00 00 00 00 (all 0)
    v.val[1] = _mm_shufflelo_epi16(v.val[3], 85);//01 01 01 01 (all 1)
    v.val[2] = _mm_shufflelo_epi16(v.val[3], 170);//10 10 10 10 (all 2)
    v.val[3] = _mm_shufflelo_epi16(v.val[3], 255);//11 11 11 11 (all 3)
    return v;
}

uint32x2x4_t vld4_dup_u32(__transfersize(4) uint32_t const * ptr);         // VLD4.32 {d0[], d1[], d2[], d3[]}, [r0]
_NEON2SSE_INLINE uint32x2x4_t vld4_dup_u32(__transfersize(4) uint32_t const * ptr)         // VLD4.32 {d0[], d1[], d2[], d3[]}, [r0]
{
    uint32x2x4_t v;
    v.val[3] = LOAD_SI128(ptr) ; //0,1,2,3
    v.val[0] = _mm_shuffle_epi32(v.val[3],   0 | (0 << 2) | (2 << 4) | (3 << 6)); //0,0,2,3
    v.val[1] = _mm_shuffle_epi32(v.val[3],   1 | (1 << 2) | (2 << 4) | (3 << 6)); //1,1,2,3
    v.val[2] = _mm_shuffle_epi32(v.val[3],   2 | (2 << 2) | (3 << 4) | (3 << 6)); //2,2,3,3
    v.val[3] = _mm_shuffle_epi32(v.val[3],   3 | (3 << 2) | (3 << 4) | (3 << 6)); //3,3,2,2
    return v;
}

uint64x1x4_t vld4_dup_u64(__transfersize(4) uint64_t const * ptr);         // VLD1.64 {d0, d1, d2, d3}, [r0]
_NEON2SSE_INLINE uint64x1x4_t vld4_dup_u64(__transfersize(4) uint64_t const * ptr)         // VLD1.64 {d0, d1, d2, d3}, [r0]
{
    uint64x1x4_t v;
    v.val[0] = LOAD_SI128(ptr); //0,1,
    v.val[1] = _mm_shuffle_epi32(v.val[0], _SWAP_HI_LOW32); //1,0
    v.val[2] = LOAD_SI128((ptr + 2)); //2,3
    v.val[3] = _mm_shuffle_epi32(v.val[2], _SWAP_HI_LOW32); //3,2
    return v;
}

int8x8x4_t vld4_dup_s8(__transfersize(4) int8_t const * ptr);         // VLD4.8 {d0[], d1[], d2[], d3[]}, [r0]
#define vld4_dup_s8(ptr) vld4_dup_u8((uint8_t*)ptr)

int16x4x4_t vld4_dup_s16(__transfersize(4) int16_t const * ptr);         // VLD4.16 {d0[], d1[], d2[], d3[]}, [r0]
#define vld4_dup_s16(ptr) vld4_dup_u16((uint16_t*)ptr)

int32x2x4_t vld4_dup_s32(__transfersize(4) int32_t const * ptr);         // VLD4.32 {d0[], d1[], d2[], d3[]}, [r0]
#define vld4_dup_s32(ptr) vld4_dup_u32((uint32_t*)ptr)

int64x1x4_t vld4_dup_s64(__transfersize(4) int64_t const * ptr);         // VLD1.64 {d0, d1, d2, d3}, [r0]
#define vld4_dup_s64(ptr) vld4_dup_u64((uint64_t*)ptr)

float16x4x4_t vld4_dup_f16(__transfersize(4) __fp16 const * ptr);         // VLD4.16 {d0[], d1[], d2[], d3[]}, [r0]
// IA32 SIMD doesn't work with 16bit floats currently, so need to go to 32 bit and then work with two 128bit registers. See vld1q_f16 for example

float32x2x4_t vld4_dup_f32(__transfersize(4) float32_t const * ptr);         // VLD4.32 {d0[], d1[], d2[], d3[]}, [r0]
_NEON2SSE_INLINE float32x2x4_t vld4_dup_f32(__transfersize(4) float32_t const * ptr)         // VLD4.32 {d0[], d1[], d2[], d3[]}, [r0]
{
    float32x2x4_t v;
    v.val[0] = vld1q_f32(ptr);  //0,1,2,3
    v.val[1] = _mm_movehdup_ps(v.val[0]); //1,1,3,3
    v.val[0] = _mm_moveldup_ps(v.val[0]); //0,0,2,2
    v.val[2] = _mm_movehl_ps(v.val[0], v.val[0]); //2,2,0,0,
    v.val[3] = _mm_movehl_ps(v.val[1], v.val[1]); //3,3,1,1,
    return v;
}

poly8x8x4_t vld4_dup_p8(__transfersize(4) poly8_t const * ptr);         // VLD4.8 {d0[], d1[], d2[], d3[]}, [r0]
#define vld4_dup_p8 vld4_dup_u8

poly16x4x4_t vld4_dup_p16(__transfersize(4) poly16_t const * ptr);         // VLD4.16 {d0[], d1[], d2[], d3[]}, [r0]
#define vld4_dup_p16 vld4_dup_u16

//**********************************************************************************
//*******************Lane loads for  an N-element structures ***********************
//**********************************************************************************
//********************** Lane pairs  ************************************************
//does vld1_lane_xx ptr[0] to src->val[0] at lane positon and ptr[1] to src->val[1] at lane positon
//we assume  src is 16 bit aligned

//!!!!!! Microsoft compiler does not allow xxxxxx_2t function arguments resulting in "formal parameter with __declspec(align('16')) won't be aligned" error
//to fix it the all functions below work with  xxxxxx_2t pointers and the corresponding original functions are redefined

//uint16x8x2_t vld2q_lane_u16(__transfersize(2) uint16_t const * ptr, uint16x8x2_t src,__constrange(0,7) int lane);// VLD2.16 {d0[0], d2[0]}, [r0]
_NEON2SSE_INLINE uint16x8x2_t vld2q_lane_u16_ptr(__transfersize(2) uint16_t const * ptr, uint16x8x2_t* src,__constrange(0,7) int lane)         // VLD2.16 {d0[0], d2[0]}, [r0]
{
    uint16x8x2_t v;
    v.val[0] = vld1q_lane_s16 (ptr, src->val[0],  lane);
    v.val[1] = vld1q_lane_s16 ((ptr + 1), src->val[1],  lane);
    return v;
}
#define vld2q_lane_u16(ptr, src, lane) vld2q_lane_u16_ptr(ptr, &src, lane)

//uint32x4x2_t vld2q_lane_u32(__transfersize(2) uint32_t const * ptr, uint32x4x2_t src,__constrange(0,3) int lane);// VLD2.32 {d0[0], d2[0]}, [r0]
_NEON2SSE_INLINE uint32x4x2_t vld2q_lane_u32_ptr(__transfersize(2) uint32_t const * ptr, uint32x4x2_t* src,__constrange(0,3) int lane)         // VLD2.32 {d0[0], d2[0]}, [r0]
{
    uint32x4x2_t v;
    v.val[0] = _MM_INSERT_EPI32 (src->val[0],  ptr[0], lane);
    v.val[1] = _MM_INSERT_EPI32 (src->val[1],  ptr[1], lane);
    return v;
}
#define vld2q_lane_u32(ptr, src, lane) vld2q_lane_u32_ptr(ptr, &src, lane)

//int16x8x2_t vld2q_lane_s16(__transfersize(2) int16_t const * ptr, int16x8x2_t src, __constrange(0,7)int lane);// VLD2.16 {d0[0], d2[0]}, [r0]
_NEON2SSE_INLINE int16x8x2_t vld2q_lane_s16_ptr(__transfersize(2) int16_t const * ptr, int16x8x2_t* src, __constrange(0,7) int lane)
{
    int16x8x2_t v;
    v.val[0] = vld1q_lane_s16 (ptr, src->val[0],  lane);
    v.val[1] = vld1q_lane_s16 ((ptr + 1), src->val[1],  lane);
    return v;
}
#define vld2q_lane_s16(ptr, src, lane) vld2q_lane_s16_ptr(ptr, &src, lane)

//int32x4x2_t vld2q_lane_s32(__transfersize(2) int32_t const * ptr, int32x4x2_t src, __constrange(0,3)int lane);// VLD2.32 {d0[0], d2[0]}, [r0]
_NEON2SSE_INLINE int32x4x2_t vld2q_lane_s32_ptr(__transfersize(2) int32_t const * ptr, int32x4x2_t* src, __constrange(0,3) int lane)
{
    int32x4x2_t v;
    v.val[0] = _MM_INSERT_EPI32 (src->val[0],  ptr[0], lane);
    v.val[1] = _MM_INSERT_EPI32 (src->val[1],  ptr[1], lane);
    return v;
}
#define vld2q_lane_s32(ptr, src, lane) vld2q_lane_s32_ptr(ptr, &src, lane)

//float16x8x2_t vld2q_lane_f16(__transfersize(2) __fp16 const * ptr, float16x8x2_t src, __constrange(0,7)int lane);// VLD2.16 {d0[0], d2[0]}, [r0]
//current IA SIMD doesn't support float16

//float32x4x2_t vld2q_lane_f32(__transfersize(2) float32_t const * ptr, float32x4x2_t src,__constrange(0,3) int lane);// VLD2.32 {d0[0], d2[0]}, [r0]
_NEON2SSE_INLINE float32x4x2_t vld2q_lane_f32_ptr(__transfersize(2) float32_t const * ptr, float32x4x2_t* src,__constrange(0,3) int lane)         // VLD2.32 {d0[0], d2[0]}, [r0]
{
    float32x4x2_t v;
    v.val[0] = vld1q_lane_f32(ptr, src->val[0], lane);
    v.val[1] = vld1q_lane_f32((ptr + 1), src->val[1], lane);
    return v;
}
#define vld2q_lane_f32(ptr, src, lane) vld2q_lane_f32_ptr(ptr, &src, lane)

//poly16x8x2_t vld2q_lane_p16(__transfersize(2) poly16_t const * ptr, poly16x8x2_t src,__constrange(0,7) int lane);// VLD2.16 {d0[0], d2[0]}, [r0]
#define vld2q_lane_p16 vld2q_lane_u16

//uint8x8x2_t vld2_lane_u8(__transfersize(2) uint8_t const * ptr, uint8x8x2_t src, __constrange(0,7) int lane);// VLD2.8 {d0[0], d1[0]}, [r0]
_NEON2SSE_INLINE uint8x8x2_t vld2_lane_u8_ptr(__transfersize(2) uint8_t const * ptr, uint8x8x2_t* src, __constrange(0,7) int lane)         // VLD2.8 {d0[0], d1[0]}, [r0]
{
    uint8x8x2_t val;
    val.val[0] = _MM_INSERT_EPI8 (src->val[0], (int)ptr[0], lane);
    val.val[1] = _MM_INSERT_EPI8 (src->val[1], (int)ptr[1], lane);
    return val;
}
#define vld2_lane_u8(ptr, src, lane) vld2_lane_u8_ptr(ptr, &src, lane)

//uint16x4x2_t vld2_lane_u16(__transfersize(2) uint16_t const * ptr, uint16x4x2_t src, __constrange(0,3)int lane);// VLD2.16 {d0[0], d1[0]}, [r0]
#define vld2_lane_u16 vld2q_lane_u16

//uint32x2x2_t vld2_lane_u32(__transfersize(2) uint32_t const * ptr, uint32x2x2_t src, __constrange(0,1)int lane);// VLD2.32 {d0[0], d1[0]}, [r0]
#define vld2_lane_u32 vld2q_lane_u32

//int8x8x2_t vld2_lane_s8(__transfersize(2) int8_t const * ptr, int8x8x2_t src, __constrange(0,7) int lane);// VLD2.8 {d0[0], d1[0]}, [r0]
int8x8x2_t vld2_lane_s8_ptr(__transfersize(2) int8_t const * ptr, int8x8x2_t * src, __constrange(0,7) int lane);         // VLD2.8 {d0[0], d1[0]}, [r0]
#define vld2_lane_s8(ptr, src, lane)  vld2_lane_u8(( uint8_t*) ptr, src, lane)

//int16x4x2_t vld2_lane_s16(__transfersize(2) int16_t const * ptr, int16x4x2_t src, __constrange(0,3) int lane);// VLD2.16 {d0[0], d1[0]}, [r0]
int16x4x2_t vld2_lane_s16_ptr(__transfersize(2) int16_t const * ptr, int16x4x2_t * src, __constrange(0,3) int lane);         // VLD2.16 {d0[0], d1[0]}, [r0]
#define vld2_lane_s16(ptr, src, lane) vld2_lane_u16(( uint16_t*) ptr, src, lane)

//int32x2x2_t vld2_lane_s32(__transfersize(2) int32_t const * ptr, int32x2x2_t src, __constrange(0,1) int lane);// VLD2.32 {d0[0], d1[0]}, [r0]
int32x2x2_t vld2_lane_s32_ptr(__transfersize(2) int32_t const * ptr, int32x2x2_t * src, __constrange(0,1) int lane);         // VLD2.32 {d0[0], d1[0]}, [r0]
#define vld2_lane_s32(ptr, src, lane) vld2_lane_u32(( uint32_t*) ptr, src, lane)

//float16x4x2_t vld2_lane_f16(__transfersize(2) __fp16 const * ptr, float16x4x2_t src, __constrange(0,3) int lane); // VLD2.16 {d0[0], d1[0]}, [r0]
//current IA SIMD doesn't support float16

float32x2x2_t vld2_lane_f32_ptr(__transfersize(2) float32_t const * ptr, float32x2x2_t * src,__constrange(0,1) int lane);         // VLD2.32 {d0[0], d1[0]}, [r0]
#define vld2_lane_f32 vld2q_lane_f32

//poly8x8x2_t vld2_lane_p8(__transfersize(2) poly8_t const * ptr, poly8x8x2_t src, __constrange(0,7) int lane);// VLD2.8 {d0[0], d1[0]}, [r0]
poly8x8x2_t vld2_lane_p8_ptr(__transfersize(2) poly8_t const * ptr, poly8x8x2_t * src, __constrange(0,7) int lane);         // VLD2.8 {d0[0], d1[0]}, [r0]
#define vld2_lane_p8 vld2_lane_u8

//poly16x4x2_t vld2_lane_p16(__transfersize(2) poly16_t const * ptr, poly16x4x2_t src, __constrange(0,3)int lane);// VLD2.16 {d0[0], d1[0]}, [r0]
poly16x4x2_t vld2_lane_p16_ptr(__transfersize(2) poly16_t const * ptr, poly16x4x2_t * src, __constrange(0,3) int lane);         // VLD2.16 {d0[0], d1[0]}, [r0]
#define vld2_lane_p16 vld2_lane_u16

//*********** Lane triplets **********************
//*************************************************
//does vld1_lane_xx ptr[0] to src->val[0], ptr[1] to src->val[1] and ptr[2] to src->val[2] at lane positon
//we assume src is 16 bit aligned

//uint16x8x3_t vld3q_lane_u16(__transfersize(3) uint16_t const * ptr, uint16x8x3_t src,__constrange(0,7) int lane);// VLD3.16 {d0[0], d2[0], d4[0]}, [r0]
_NEON2SSE_INLINE uint16x8x3_t vld3q_lane_u16_ptr(__transfersize(3) uint16_t const * ptr, uint16x8x3_t* src,__constrange(0,7) int lane)         // VLD3.16 {d0[0], d2[0], d4[0]}, [r0]
{
    uint16x8x3_t v;
    v.val[0] = _MM_INSERT_EPI16 ( src->val[0],  ptr[0], lane);
    v.val[1] = _MM_INSERT_EPI16 ( src->val[1],  ptr[1], lane);
    v.val[2] = _MM_INSERT_EPI16 ( src->val[2],  ptr[2], lane);
    return v;
}
#define vld3q_lane_u16(ptr, src, lane) vld3q_lane_u16_ptr(ptr, &src, lane)

//uint32x4x3_t vld3q_lane_u32(__transfersize(3) uint32_t const * ptr, uint32x4x3_t src,__constrange(0,3) int lane);// VLD3.32 {d0[0], d2[0], d4[0]}, [r0]
_NEON2SSE_INLINE uint32x4x3_t vld3q_lane_u32_ptr(__transfersize(3) uint32_t const * ptr, uint32x4x3_t* src,__constrange(0,3) int lane)         // VLD3.32 {d0[0], d2[0], d4[0]}, [r0]
{
    uint32x4x3_t v;
    v.val[0] = _MM_INSERT_EPI32 ( src->val[0],  ptr[0], lane);
    v.val[1] = _MM_INSERT_EPI32 ( src->val[1],  ptr[1], lane);
    v.val[2] = _MM_INSERT_EPI32 ( src->val[2],  ptr[2], lane);
    return v;
}
#define vld3q_lane_u32(ptr, src, lane) vld3q_lane_u32_ptr(ptr, &src, lane)

//int16x8x3_t vld3q_lane_s16(__transfersize(3) int16_t const * ptr, int16x8x3_t src, __constrange(0,7)int lane);// VLD3.16 {d0[0], d2[0], d4[0]}, [r0]
_NEON2SSE_INLINE int16x8x3_t vld3q_lane_s16_ptr(__transfersize(3) int16_t const * ptr, int16x8x3_t* src, __constrange(0,7) int lane)         // VLD3.16 {d0[0], d2[0], d4[0]}, [r0]
{
    int16x8x3_t v;
    v.val[0] = _MM_INSERT_EPI16 ( src->val[0],  ptr[0], lane);
    v.val[1] = _MM_INSERT_EPI16 ( src->val[1],  ptr[1], lane);
    v.val[2] = _MM_INSERT_EPI16 ( src->val[2],  ptr[2], lane);
    return v;
}
#define vld3q_lane_s16(ptr, src, lane) vld3q_lane_s16_ptr(ptr, &src, lane)

//int32x4x3_t vld3q_lane_s32(__transfersize(3) int32_t const * ptr, int32x4x3_t src, __constrange(0,3)int lane);// VLD3.32 {d0[0], d2[0], d4[0]}, [r0]
_NEON2SSE_INLINE int32x4x3_t vld3q_lane_s32_ptr(__transfersize(3) int32_t const * ptr, int32x4x3_t* src, __constrange(0,3) int lane)         // VLD3.32 {d0[0], d2[0], d4[0]}, [r0]
{
    int32x4x3_t v;
    v.val[0] = _MM_INSERT_EPI32 ( src->val[0],  ptr[0], lane);
    v.val[1] = _MM_INSERT_EPI32 ( src->val[1],  ptr[1], lane);
    v.val[2] = _MM_INSERT_EPI32 ( src->val[2],  ptr[2], lane);
    return v;
}
#define vld3q_lane_s32(ptr, src, lane) vld3q_lane_s32_ptr(ptr, &src, lane)

float16x8x3_t vld3q_lane_f16_ptr(__transfersize(3) __fp16 const * ptr, float16x8x3_t * src, __constrange(0,7) int lane);         // VLD3.16 {d0[0], d2[0], d4[0]}, [r0]
//current IA SIMD doesn't support float16
#define vld3q_lane_f16(ptr, src, lane) vld3q_lane_f16_ptr(ptr, &src, lane)

//float32x4x3_t vld3q_lane_f32(__transfersize(3) float32_t const * ptr, float32x4x3_t src,__constrange(0,3) int lane);// VLD3.32 {d0[0], d2[0], d4[0]}, [r0]
_NEON2SSE_INLINE float32x4x3_t vld3q_lane_f32_ptr(__transfersize(3) float32_t const * ptr, float32x4x3_t* src,__constrange(0,3) int lane)         // VLD3.32 {d0[0], d2[0], d4[0]}, [r0]
{
    float32x4x3_t v;
    v.val[0] = vld1q_lane_f32(&ptr[0], src->val[0], lane);
    v.val[1] = vld1q_lane_f32(&ptr[1], src->val[1], lane);
    v.val[2] = vld1q_lane_f32(&ptr[2], src->val[2], lane);
    return v;
}
#define vld3q_lane_f32(ptr, src, lane) vld3q_lane_f32_ptr(ptr, &src, lane)

poly16x8x3_t vld3q_lane_p16_ptr(__transfersize(3) poly16_t const * ptr, poly16x8x3_t * src,__constrange(0,7) int lane);         // VLD3.16 {d0[0], d2[0], d4[0]}, [r0]
#define vld3q_lane_p16 vld3q_lane_u16

//uint8x8x3_t vld3_lane_u8(__transfersize(3) uint8_t const * ptr, uint8x8x3_t src, __constrange(0,7) int lane);// VLD3.8 {d0[0], d1[0], d2[0]}, [r0]
_NEON2SSE_INLINE uint8x8x3_t vld3_lane_u8_ptr(__transfersize(3) uint8_t const * ptr, uint8x8x3_t* src, __constrange(0,7) int lane)         // VLD3.8 {d0[0], d1[0], d2[0]}, [r0]
{
    uint8x8x3_t v;
    v.val[0] = _MM_INSERT_EPI8 (src->val[0], ptr[0], lane);
    v.val[1] = _MM_INSERT_EPI8 (src->val[1], ptr[1], lane);
    v.val[2] = _MM_INSERT_EPI8 (src->val[2], ptr[2], lane);
    return v;
}
#define vld3_lane_u8(ptr, src, lane) vld3_lane_u8_ptr(ptr, &src, lane)

//uint16x4x3_t vld3_lane_u16(__transfersize(3) uint16_t   const * ptr, uint16x4x3_t src, __constrange(0,3)int lane);// VLD3.16 {d0[0], d1[0], d2[0]}, [r0]
_NEON2SSE_INLINE uint16x4x3_t vld3_lane_u16_ptr(__transfersize(3) uint16_t const * ptr, uint16x4x3_t* src, __constrange(0,3) int lane)         // VLD3.16 {d0[0], d1[0], d2[0]}, [r0]
{
    uint16x4x3_t v;
    v.val[0] = _MM_INSERT_EPI16 (src->val[0], ptr[0], lane);
    v.val[1] = _MM_INSERT_EPI16 (src->val[1], ptr[1], lane);
    v.val[2] = _MM_INSERT_EPI16 (src->val[2], ptr[2], lane);
    return v;
}
#define vld3_lane_u16(ptr, src, lane) vld3_lane_u16_ptr(ptr, &src, lane)

//uint32x2x3_t vld3_lane_u32(__transfersize(3) uint32_t const * ptr, uint32x2x3_t src, __constrange(0,1)int lane);// VLD3.32 {d0[0], d1[0], d2[0]}, [r0]
_NEON2SSE_INLINE uint32x2x3_t vld3_lane_u32_ptr(__transfersize(3) uint32_t const * ptr, uint32x2x3_t* src, __constrange(0,1) int lane)         // VLD3.32 {d0[0], d1[0], d2[0]}, [r0]
{         //need to merge into 128 bit anyway
    uint32x2x3_t v;
    v.val[0] = _MM_INSERT_EPI32 (src->val[0], ptr[0], lane);
    v.val[1] = _MM_INSERT_EPI32 (src->val[1], ptr[1], lane);
    v.val[2] = _MM_INSERT_EPI32 (src->val[2], ptr[2], lane);
    return v;
}
#define vld3_lane_u32(ptr, src, lane) vld3_lane_u32_ptr(ptr, &src, lane)

int8x8x3_t vld3_lane_s8_ptr(__transfersize(3) int8_t const * ptr, int8x8x3_t * src, __constrange(0,7) int lane);         // VLD3.8 {d0[0], d1[0], d2[0]}, [r0]
#define vld3_lane_s8(ptr, src, lane)  vld3_lane_u8_ptr(( uint8_t*) ptr, &src, lane)

int16x4x3_t vld3_lane_s16_ptr(__transfersize(3) int16_t const * ptr, int16x4x3_t * src, __constrange(0,3) int lane);         // VLD3.16 {d0[0], d1[0], d2[0]}, [r0]
#define vld3_lane_s16(ptr, src, lane)  vld3_lane_u16_ptr(( uint16_t*) ptr, &src, lane)

int32x2x3_t vld3_lane_s32_ptr(__transfersize(3) int32_t const * ptr, int32x2x3_t * src, __constrange(0,1) int lane);         // VLD3.32 {d0[0], d1[0], d2[0]}, [r0]
#define vld3_lane_s32(ptr, src, lane)  vld3_lane_u32_ptr(( uint32_t*) ptr, &src, lane)

float16x4x3_t vld3_lane_f16_ptr(__transfersize(3) __fp16 const * ptr, float16x4x3_t * src, __constrange(0,3) int lane);         // VLD3.16 {d0[0], d1[0], d2[0]}, [r0]
//current IA SIMD doesn't support float16

//float32x2x3_t vld3_lane_f32(__transfersize(3) float32_t const * ptr, float32x2x3_t src,__constrange(0,1) int lane);// VLD3.32 {d0[0], d1[0], d2[0]}, [r0]
_NEON2SSE_INLINE float32x2x3_t vld3_lane_f32_ptr(__transfersize(3) float32_t const * ptr, float32x2x3_t* src,__constrange(0,1) int lane)         // VLD3.32 {d0[0], d1[0], d2[0]}, [r0]
{
    float32x2x3_t v;
    v.val[0] = vld1q_lane_f32(ptr, src->val[0], lane);
    return v;
}
#define vld3_lane_f32(ptr, src, lane) vld3_lane_f32_ptr(ptr, &src, lane)

//poly8x8x3_t vld3_lane_p8_ptr(__transfersize(3) poly8_t const * ptr, poly8x8x3_t * src, __constrange(0,7) int lane); // VLD3.8 {d0[0], d1[0], d2[0]}, [r0]
#define vld3_lane_p8 vld3_lane_u8

//poly16x4x3_t vld3_lane_p16(__transfersize(3) poly16_t const * ptr, poly16x4x3_t * src, __constrange(0,3) int lane); // VLD3.16 {d0[0], d1[0], d2[0]}, [r0]
#define vld3_lane_p16 vld3_lane_u16

//******************* Lane Quadruples  load ***************************
//*********************************************************************
//does vld1_lane_xx ptr[0] to src->val[0], ptr[1] to src->val[1], ptr[2] to src->val[2] and ptr[3] to src->val[3] at lane positon
//we assume src is 16 bit aligned

//uint16x8x4_t vld4q_lane_u16(__transfersize(4) uint16_t const * ptr, uint16x8x4_t src,__constrange(0,7) int lane)// VLD4.16 {d0[0], d2[0], d4[0], d6[0]}, [r0]
_NEON2SSE_INLINE uint16x8x4_t vld4q_lane_u16_ptr(__transfersize(4) uint16_t const * ptr, uint16x8x4_t* src,__constrange(0,7) int lane)
{
    uint16x8x4_t v;
    v.val[0] = _MM_INSERT_EPI16 ( src->val[0],  ptr[0], lane);
    v.val[1] = _MM_INSERT_EPI16 ( src->val[1],  ptr[1], lane);
    v.val[2] = _MM_INSERT_EPI16 ( src->val[2],  ptr[2], lane);
    v.val[3] = _MM_INSERT_EPI16 ( src->val[3],  ptr[3], lane);
    return v;
}
#define vld4q_lane_u16(ptr, src, lane) vld4q_lane_u16_ptr(ptr, &src, lane)

//uint32x4x4_t vld4q_lane_u32(__transfersize(4) uint32_t const * ptr, uint32x4x4_t src,__constrange(0,3) int lane)// VLD4.32 {d0[0], d2[0], d4[0], d6[0]}, [r0]
_NEON2SSE_INLINE uint32x4x4_t vld4q_lane_u32_ptr(__transfersize(4) uint32_t const * ptr, uint32x4x4_t* src,__constrange(0,3) int lane)
{
    uint32x4x4_t v;
    v.val[0] = _MM_INSERT_EPI32 ( src->val[0],  ptr[0], lane);
    v.val[1] = _MM_INSERT_EPI32 ( src->val[1],  ptr[1], lane);
    v.val[2] = _MM_INSERT_EPI32 ( src->val[2],  ptr[2], lane);
    v.val[3] = _MM_INSERT_EPI32 ( src->val[3],  ptr[3], lane);
    return v;
}
#define vld4q_lane_u32(ptr, src, lane) vld4q_lane_u32_ptr(ptr, &src, lane)

//int16x8x4_t vld4q_lane_s16(__transfersize(4) int16_t const * ptr, int16x8x4_t src, __constrange(0,7)int lane);// VLD4.16 {d0[0], d2[0], d4[0], d6[0]}, [r0]
int16x8x4_t vld4q_lane_s16_ptr(__transfersize(4) int16_t const * ptr, int16x8x4_t * src, __constrange(0,7) int lane);         // VLD4.16 {d0[0], d2[0], d4[0], d6[0]}, [r0]
#define vld4q_lane_s16(ptr, src, lane) vld4q_lane_u16(( uint16_t*) ptr, src, lane)

//int32x4x4_t vld4q_lane_s32(__transfersize(4) int32_t const * ptr, int32x4x4_t src, __constrange(0,3)int lane);// VLD4.32 {d0[0], d2[0], d4[0], d6[0]}, [r0]
int32x4x4_t vld4q_lane_s32_ptr(__transfersize(4) int32_t const * ptr, int32x4x4_t * src, __constrange(0,3) int lane);         // VLD4.32 {d0[0], d2[0], d4[0], d6[0]}, [r0]
#define vld4q_lane_s32(ptr, src, lane)  vld4q_lane_u32(( uint32_t*) ptr, src, lane)

//float16x8x4_t vld4q_lane_f16(__transfersize(4) __fp16 const * ptr, float16x8x4_t src, __constrange(0,7)int lane);// VLD4.16 {d0[0], d2[0], d4[0], d6[0]}, [r0]
float16x8x4_t vld4q_lane_f16_ptr(__transfersize(4) __fp16 const * ptr, float16x8x4_t * src, __constrange(0,7) int lane);         // VLD4.16 {d0[0], d2[0], d4[0], d6[0]}, [r0]
//current IA SIMD doesn't support float16

//float32x4x4_t vld4q_lane_f32(__transfersize(4) float32_t const * ptr, float32x4x4_t src,__constrange(0,3) int lane)// VLD4.32 {d0[0], d2[0], d4[0], d6[0]}, [r0]
_NEON2SSE_INLINE float32x4x4_t vld4q_lane_f32_ptr(__transfersize(4) float32_t const * ptr, float32x4x4_t* src,__constrange(0,3) int lane)
{
    float32x4x4_t v;
    v.val[0] = vld1q_lane_f32(&ptr[0], src->val[0], lane);
    v.val[1] = vld1q_lane_f32(&ptr[1], src->val[1], lane);
    v.val[2] = vld1q_lane_f32(&ptr[2], src->val[2], lane);
    v.val[3] = vld1q_lane_f32(&ptr[3], src->val[3], lane);
    return v;
}
#define vld4q_lane_f32(ptr, src, lane) vld4q_lane_f32_ptr(ptr, &src, lane)

//poly16x8x4_t vld4q_lane_p16(__transfersize(4) poly16_t const * ptr, poly16x8x4_t src,__constrange(0,7) int lane);// VLD4.16 {d0[0], d2[0], d4[0], d6[0]}, [r0]
poly16x8x4_t vld4q_lane_p16_ptr(__transfersize(4) poly16_t const * ptr, poly16x8x4_t * src,__constrange(0,7) int lane);         // VLD4.16 {d0[0], d2[0], d4[0], d6[0]}, [r0]
#define vld4q_lane_p16 vld4q_lane_u16

//uint8x8x4_t vld4_lane_u8(__transfersize(4) uint8_t const * ptr, uint8x8x4_t src, __constrange(0,7) int lane)// VLD4.8 {d0[0], d1[0], d2[0], d3[0]}, [r0]
_NEON2SSE_INLINE uint8x8x4_t vld4_lane_u8_ptr(__transfersize(4) uint8_t const * ptr, uint8x8x4_t* src, __constrange(0,7) int lane)
{
    uint8x8x4_t v;
    v.val[0] = _MM_INSERT_EPI8 (src->val[0], ptr[0], lane);
    v.val[1] = _MM_INSERT_EPI8 (src->val[1], ptr[1], lane );
    v.val[2] = _MM_INSERT_EPI8 (src->val[2], ptr[2], lane );
    v.val[3] = _MM_INSERT_EPI8 (src->val[3], ptr[3], lane );
    return v;
}
#define vld4_lane_u8(ptr, src, lane) vld4_lane_u8_ptr(ptr, &src, lane)

//uint16x4x4_t vld4_lane_u16(__transfersize(4) uint16_t const * ptr, uint16x4x4_t src, __constrange(0,3)int lane)// VLD4.16 {d0[0], d1[0], d2[0], d3[0]}, [r0]
_NEON2SSE_INLINE uint16x4x4_t vld4_lane_u16_ptr(__transfersize(4) uint16_t const * ptr, uint16x4x4_t* src, __constrange(0,3) int lane)
{
    uint16x4x4_t v;
    v.val[0] = _MM_INSERT_EPI16 (src->val[0], ptr[0], lane);
    v.val[1] = _MM_INSERT_EPI16 (src->val[1], ptr[1], lane );
    v.val[2] = _MM_INSERT_EPI16 (src->val[2], ptr[2], lane );
    v.val[3] = _MM_INSERT_EPI16 (src->val[3], ptr[3], lane );
    return v;
}
#define vld4_lane_u16(ptr, src, lane) vld4_lane_u16_ptr(ptr, &src, lane)

//uint32x2x4_t vld4_lane_u32(__transfersize(4) uint32_t const * ptr, uint32x2x4_t src, __constrange(0,1)int lane)// VLD4.32 {d0[0], d1[0], d2[0], d3[0]}, [r0]
_NEON2SSE_INLINE uint32x2x4_t vld4_lane_u32_ptr(__transfersize(4) uint32_t const * ptr, uint32x2x4_t* src, __constrange(0,1) int lane)
{
    uint32x2x4_t v;
    v.val[0] = _MM_INSERT_EPI32 (src->val[0], ptr[0], lane);
    v.val[1] = _MM_INSERT_EPI32 (src->val[1], ptr[1], lane );
    v.val[2] = _MM_INSERT_EPI32 (src->val[2], ptr[2], lane );
    v.val[3] = _MM_INSERT_EPI32 (src->val[3], ptr[3], lane );
    return v;
}
#define vld4_lane_u32(ptr, src, lane) vld4_lane_u32_ptr(ptr, &src, lane)

//int8x8x4_t vld4_lane_s8(__transfersize(4) int8_t const * ptr, int8x8x4_t src, __constrange(0,7) int lane);// VLD4.8 {d0[0], d1[0], d2[0], d3[0]}, [r0]
int8x8x4_t vld4_lane_s8_ptr(__transfersize(4) int8_t const * ptr, int8x8x4_t * src, __constrange(0,7) int lane);
#define vld4_lane_s8(ptr,src,lane) vld4_lane_u8((uint8_t*)ptr,src,lane)

//int16x4x4_t vld4_lane_s16(__transfersize(4) int16_t const * ptr, int16x4x4_t src, __constrange(0,3) int lane);// VLD4.16 {d0[0], d1[0], d2[0], d3[0]}, [r0]
int16x4x4_t vld4_lane_s16_ptr(__transfersize(4) int16_t const * ptr, int16x4x4_t * src, __constrange(0,3) int lane);
#define vld4_lane_s16(ptr,src,lane) vld4_lane_u16((uint16_t*)ptr,src,lane)

//int32x2x4_t vld4_lane_s32(__transfersize(4) int32_t const * ptr, int32x2x4_t src, __constrange(0,1) int lane);// VLD4.32 {d0[0], d1[0], d2[0], d3[0]}, [r0]
int32x2x4_t vld4_lane_s32_ptr(__transfersize(4) int32_t const * ptr, int32x2x4_t * src, __constrange(0,1) int lane);
#define vld4_lane_s32(ptr,src,lane) vld4_lane_u32((uint32_t*)ptr,src,lane)

//float16x4x4_t vld4_lane_f16(__transfersize(4) __fp16 const * ptr, float16x4x4_t src, __constrange(0,3)int lane);// VLD4.16 {d0[0], d1[0], d2[0], d3[0]}, [r0]
float16x4x4_t vld4_lane_f16_ptr(__transfersize(4) __fp16 const * ptr, float16x4x4_t * src, __constrange(0,3) int lane);
//current IA SIMD doesn't support float16

//float32x2x4_t vld4_lane_f32(__transfersize(4) float32_t const * ptr, float32x2x4_t src,__constrange(0,1) int lane)// VLD4.32 {d0[0], d1[0], d2[0], d3[0]}, [r0]
_NEON2SSE_INLINE float32x2x4_t vld4_lane_f32_ptr(__transfersize(4) float32_t const * ptr, float32x2x4_t* src,__constrange(0,1) int lane)
{         //serial solution may be faster
    float32x2x4_t v;
    return v;
}
#define vld4_lane_f32(ptr, src, lane) vld4_lane_f32_ptr(ptr, &src, lane)

//poly8x8x4_t vld4_lane_p8(__transfersize(4) poly8_t const * ptr, poly8x8x4_t src, __constrange(0,7) int lane);// VLD4.8 {d0[0], d1[0], d2[0], d3[0]}, [r0]
poly8x8x4_t vld4_lane_p8_ptr(__transfersize(4) poly8_t const * ptr, poly8x8x4_t * src, __constrange(0,7) int lane);
#define vld4_lane_p8 vld4_lane_u8

//poly16x4x4_t vld4_lane_p16(__transfersize(4) poly16_t const * ptr, poly16x4x4_t src, __constrange(0,3)int lane);// VLD4.16 {d0[0], d1[0], d2[0], d3[0]}, [r0]
poly16x4x4_t vld4_lane_p16_ptr(__transfersize(4) poly16_t const * ptr, poly16x4x4_t * src, __constrange(0,3) int lane);
#define vld4_lane_p16 vld4_lane_u16

//******************* Store duplets *********************************************
//********************************************************************************
//here we assume the ptr is 16bit aligned. If not we need to use _mm_storeu_si128 like shown in vst1q_u8 function
//If necessary you need to modify all store functions accordingly. See more comments to "Store single" functions
//void vst2q_u8(__transfersize(32) uint8_t * ptr, uint8x16x2_t val)// VST2.8 {d0, d2}, [r0]
_NEON2SSE_INLINE void vst2q_u8_ptr(__transfersize(32) uint8_t * ptr, uint8x16x2_t* val)
{
    uint8x16x2_t v;
    v.val[0] = _mm_unpacklo_epi8(val->val[0], val->val[1]);
    v.val[1] = _mm_unpackhi_epi8(val->val[0], val->val[1]);
    vst1q_u8 (ptr, v.val[0]);
    vst1q_u8 ((ptr + 16),  v.val[1]);
}
#define vst2q_u8(ptr, val) vst2q_u8_ptr(ptr, &val)

//void vst2q_u16(__transfersize(16) uint16_t * ptr, uint16x8x2_t val)// VST2.16 {d0, d2}, [r0]
_NEON2SSE_INLINE void vst2q_u16_ptr(__transfersize(16) uint16_t * ptr, uint16x8x2_t* val)
{
    uint16x8x2_t v;
    v.val[0] = _mm_unpacklo_epi16(val->val[0], val->val[1]);
    v.val[1] = _mm_unpackhi_epi16(val->val[0], val->val[1]);
    vst1q_u16 (ptr, v.val[0]);
    vst1q_u16 ((ptr + 8),  v.val[1]);
}
#define vst2q_u16(ptr, val) vst2q_u16_ptr(ptr, &val)

//void vst2q_u32(__transfersize(8) uint32_t * ptr, uint32x4x2_t val)// VST2.32 {d0, d2}, [r0]
_NEON2SSE_INLINE void vst2q_u32_ptr(__transfersize(8) uint32_t* ptr, uint32x4x2_t* val)
{
    uint32x4x2_t v;
    v.val[0] = _mm_unpacklo_epi32(val->val[0], val->val[1]);
    v.val[1] = _mm_unpackhi_epi32(val->val[0], val->val[1]);
    vst1q_u32 (ptr, v.val[0]);
    vst1q_u32 ((ptr + 4),  v.val[1]);
}
#define vst2q_u32(ptr, val) vst2q_u32_ptr(ptr, &val)

//void vst2q_s8(__transfersize(32) int8_t * ptr, int8x16x2_t val); // VST2.8 {d0, d2}, [r0]
void vst2q_s8_ptr(__transfersize(32) int8_t * ptr, int8x16x2_t * val);
#define vst2q_s8(ptr, val) vst2q_u8((uint8_t*)(ptr), val)

//void vst2q_s16(__transfersize(16) int16_t * ptr, int16x8x2_t val);// VST2.16 {d0, d2}, [r0]
void vst2q_s16_ptr(__transfersize(16) int16_t * ptr, int16x8x2_t * val);
#define vst2q_s16(ptr, val) vst2q_u16((uint16_t*)(ptr), val)

//void vst2q_s32(__transfersize(8) int32_t * ptr, int32x4x2_t val);// VST2.32 {d0, d2}, [r0]
void vst2q_s32_ptr(__transfersize(8) int32_t * ptr, int32x4x2_t * val);
#define vst2q_s32(ptr, val)  vst2q_u32((uint32_t*)(ptr), val)

//void vst2q_f16(__transfersize(16) __fp16 * ptr, float16x8x2_t val);// VST2.16 {d0, d2}, [r0]
void vst2q_f16_ptr(__transfersize(16) __fp16 * ptr, float16x8x2_t * val);
// IA32 SIMD doesn't work with 16bit floats currently

//void vst2q_f32(__transfersize(8) float32_t * ptr, float32x4x2_t val)// VST2.32 {d0, d2}, [r0]
_NEON2SSE_INLINE void vst2q_f32_ptr(__transfersize(8) float32_t* ptr, float32x4x2_t* val)
{
    float32x4x2_t v;
    v.val[0] = _mm_unpacklo_ps(val->val[0], val->val[1]);
    v.val[1] = _mm_unpackhi_ps(val->val[0], val->val[1]);
    vst1q_f32 (ptr, v.val[0]);
    vst1q_f32 ((ptr + 4),  v.val[1]);
}
#define vst2q_f32(ptr, val) vst2q_f32_ptr(ptr, &val)

//void vst2q_p8(__transfersize(32) poly8_t * ptr, poly8x16x2_t val);// VST2.8 {d0, d2}, [r0]
void vst2q_p8_ptr(__transfersize(32) poly8_t * ptr, poly8x16x2_t * val);
#define vst2q_p8 vst2q_u8

//void vst2q_p16(__transfersize(16) poly16_t * ptr, poly16x8x2_t val);// VST2.16 {d0, d2}, [r0]
void vst2q_p16_ptr(__transfersize(16) poly16_t * ptr, poly16x8x2_t * val);
#define vst2q_p16 vst2q_u16

//void vst2_u8(__transfersize(16) uint8_t * ptr, uint8x8x2_t val);// VST2.8 {d0, d1}, [r0]
_NEON2SSE_INLINE void vst2_u8_ptr(__transfersize(16) uint8_t * ptr, uint8x8x2_t* val)
{
    uint8x8x2_t v;
    v.val[0] = _mm_unpacklo_epi8(val->val[0], val->val[1]);
    vst1q_u8 (ptr, v.val[0]);
}
#define vst2_u8(ptr, val) vst2_u8_ptr(ptr, &val)

//void vst2_u16(__transfersize(8) uint16_t * ptr, uint16x4x2_t val);// VST2.16 {d0, d1}, [r0]
_NEON2SSE_INLINE void vst2_u16_ptr(__transfersize(8) uint16_t * ptr, uint16x4x2_t* val)
{
    uint16x4x2_t v;
    v.val[0] = _mm_unpacklo_epi16(val->val[0], val->val[1]);
    vst1q_u16 (ptr, v.val[0]);
}
#define vst2_u16(ptr, val) vst2_u16_ptr(ptr, &val)

//void vst2_u32(__transfersize(4) uint32_t * ptr, uint32x2x2_t val);// VST2.32 {d0, d1}, [r0]
_NEON2SSE_INLINE void vst2_u32_ptr(__transfersize(4) uint32_t * ptr, uint32x2x2_t* val)
{
    uint32x2x2_t v;
    v.val[0] = _mm_unpacklo_epi32(val->val[0], val->val[1]);
    vst1q_u32 (ptr, v.val[0]);
}
#define vst2_u32(ptr, val) vst2_u32_ptr(ptr, &val)

//void vst2_u64(__transfersize(2) uint64_t * ptr, uint64x1x2_t val);// VST1.64 {d0, d1}, [r0]
void vst2_u64_ptr(__transfersize(2) uint64_t * ptr, uint64x1x2_t * val);
_NEON2SSE_INLINE void vst2_u64_ptr(__transfersize(2) uint64_t * ptr, uint64x1x2_t* val)
{
    uint64x1x2_t v;
    v.val[0] = _mm_unpacklo_epi64(val->val[0], val->val[1]);
    vst1q_u64(ptr, v.val[0]);
}
#define vst2_u64(ptr, val) vst2_u64_ptr(ptr, &val)

//void vst2_s8(__transfersize(16) int8_t * ptr, int8x8x2_t val);// VST2.8 {d0, d1}, [r0]
#define vst2_s8(ptr, val) vst2_u8((uint8_t*) ptr, val)

//void vst2_s16(__transfersize(8) int16_t * ptr, int16x4x2_t val); // VST2.16 {d0, d1}, [r0]
#define vst2_s16(ptr,val) vst2_u16((uint16_t*) ptr, val)

//void vst2_s32(__transfersize(4) int32_t * ptr, int32x2x2_t val); // VST2.32 {d0, d1}, [r0]
#define vst2_s32(ptr,val) vst2_u32((uint32_t*) ptr, val)

//void vst2_s64(__transfersize(2) int64_t * ptr, int64x1x2_t val);
#define vst2_s64(ptr,val) vst2_u64((uint64_t*) ptr,val)

//void vst2_f16(__transfersize(8) __fp16 * ptr, float16x4x2_t val); // VST2.16 {d0, d1}, [r0]
//current IA SIMD doesn't support float16

void vst2_f32_ptr(__transfersize(4) float32_t * ptr, float32x2x2_t * val);         // VST2.32 {d0, d1}, [r0]
_NEON2SSE_INLINE void vst2_f32_ptr(__transfersize(4) float32_t* ptr, float32x2x2_t* val)
{
    float32x4x2_t v;
    v.val[0] = _mm_unpacklo_ps(val->val[0], val->val[1]);
    vst1q_f32 (ptr, v.val[0]);
}
#define vst2_f32(ptr, val) vst2_f32_ptr(ptr, &val)

//void vst2_p8_ptr(__transfersize(16) poly8_t * ptr, poly8x8x2_t * val); // VST2.8 {d0, d1}, [r0]
#define vst2_p8 vst2_u8

//void vst2_p16_ptr(__transfersize(8) poly16_t * ptr, poly16x4x2_t * val); // VST2.16 {d0, d1}, [r0]
#define vst2_p16 vst2_u16

//******************** Triplets store  *****************************************
//******************************************************************************
//void vst3q_u8(__transfersize(48) uint8_t * ptr, uint8x16x3_t val)// VST3.8 {d0, d2, d4}, [r0]
#if defined(USE_SSSE3)
_NEON2SSE_INLINE void vst3q_u8_ptr(__transfersize(48) uint8_t * ptr, uint8x16x3_t* val)
{
    uint8x16x3_t v;
    __m128i v0,v1,v2, cff, bldmask;
    _NEON2SSE_ALIGN_16 uint8_t mask0[16]   = {0, 1, 0xff, 2, 3,0xff, 4, 5,0xff, 6,7,0xff, 8,9,0xff, 10};
    _NEON2SSE_ALIGN_16 uint8_t mask1[16]   = {0, 0xff, 1, 2, 0xff, 3, 4, 0xff, 5, 6, 0xff, 7,8,0xff, 9,10};
    _NEON2SSE_ALIGN_16 uint8_t mask2[16] =    {0xff, 6, 7, 0xff, 8, 9,0xff, 10, 11,0xff, 12,13,0xff, 14,15,0xff};
    _NEON2SSE_ALIGN_16 uint8_t mask2lo[16] = {0xff,0xff, 0, 0xff,0xff, 1, 0xff,0xff, 2, 0xff,0xff, 3, 0xff,0xff, 4, 0xff};
    _NEON2SSE_ALIGN_16 uint8_t mask2med[16] = {0xff, 5, 0xff, 0xff, 6, 0xff,0xff, 7, 0xff,0xff, 8, 0xff,0xff, 9, 0xff, 0xff};
    _NEON2SSE_ALIGN_16 uint8_t mask2hi[16] = {10, 0xff,0xff, 11, 0xff,0xff, 12, 0xff,0xff, 13, 0xff,0xff, 14, 0xff, 0xff, 15};

    v0 =  _mm_unpacklo_epi8(val->val[0], val->val[1]);         //0,1, 3,4, 6,7, 9,10, 12,13, 15,16, 18,19, 21,22
    v2 =  _mm_unpackhi_epi8(val->val[0], val->val[1]);         //24,25,  27,28, 30,31, 33,34, 36,37, 39,40, 42,43, 45,46
    v1 =  _mm_alignr_epi8(v2, v0, 11);         //12,13, 15,16, 18,19, 21,22, 24,25,  27,28, 30,31, 33,34
    v.val[0] =  _mm_shuffle_epi8(v0, *(__m128i*)mask0);         //make holes for the v.val[2] data embedding
    v.val[2] =  _mm_shuffle_epi8(val->val[2], *(__m128i*)mask2lo);         //make plugs for the v.val[2] data embedding
    cff = _mm_cmpeq_epi8(v0, v0);         //all ff
    bldmask = _mm_cmpeq_epi8(*(__m128i*)mask0, cff);
    v.val[0] = _MM_BLENDV_EPI8(v.val[0], v.val[2], bldmask);
    vst1q_u8(ptr,   v.val[0]);
    v.val[0] =  _mm_shuffle_epi8(v1, *(__m128i*)mask1);         //make holes for the v.val[2] data embedding
    v.val[2] =  _mm_shuffle_epi8(val->val[2], *(__m128i*)mask2med);         //make plugs for the v.val[2] data embedding
    bldmask = _mm_cmpeq_epi8(*(__m128i*)mask1, cff);
    v.val[1] = _MM_BLENDV_EPI8(v.val[0],v.val[2], bldmask);
    vst1q_u8((ptr + 16),  v.val[1]);
    v.val[0] =  _mm_shuffle_epi8(v2, *(__m128i*)mask2);         //make holes for the v.val[2] data embedding
    v.val[2] =  _mm_shuffle_epi8(val->val[2], *(__m128i*)mask2hi);         //make plugs for the v.val[2] data embedding
    bldmask = _mm_cmpeq_epi8(*(__m128i*)mask2, cff);
    v.val[2] = _MM_BLENDV_EPI8(v.val[0],v.val[2], bldmask );
    vst1q_u8((ptr + 32),  v.val[2]);
}
#define vst3q_u8(ptr, val) vst3q_u8_ptr(ptr, &val)
#endif

#if defined(USE_SSSE3)
//void vst3q_u16(__transfersize(24) uint16_t * ptr, uint16x8x3_t val)// VST3.16 {d0, d2, d4}, [r0]
_NEON2SSE_INLINE void vst3q_u16_ptr(__transfersize(24) uint16_t * ptr, uint16x8x3_t* val)
{
    uint16x8x3_t v;
    __m128i v0,v1,v2, cff, bldmask;
    _NEON2SSE_ALIGN_16 uint8_t mask0[16]   = {0,1, 2,3, 0xff,0xff, 4,5, 6,7,0xff,0xff, 8,9,10,11};
    _NEON2SSE_ALIGN_16 uint8_t mask1[16]   = {0xff, 0xff, 0,1, 2,3, 0xff,0xff, 4,5, 6,7, 0xff,0xff, 8,9};
    _NEON2SSE_ALIGN_16 uint8_t mask2[16] =    {6,7,0xff,0xff, 8,9,10,11, 0xff, 0xff, 12,13,14,15, 0xff, 0xff};
    _NEON2SSE_ALIGN_16 uint8_t mask2lo[16] = {0xff,0xff, 0xff,0xff, 0,1, 0xff,0xff, 0xff,0xff, 2,3, 0xff,0xff, 0xff,0xff};
    _NEON2SSE_ALIGN_16 uint8_t mask2med[16] = {4,5, 0xff,0xff,0xff,0xff, 6,7, 0xff, 0xff,0xff,0xff, 8,9, 0xff, 0xff};
    _NEON2SSE_ALIGN_16 uint8_t mask2hi[16] = {0xff, 0xff, 10,11, 0xff, 0xff, 0xff, 0xff, 12,13, 0xff, 0xff, 0xff, 0xff,14,15};

    v0 =  _mm_unpacklo_epi16(val->val[0], val->val[1]);         //0,1, 3,4, 6,7, 9,10
    v2 =  _mm_unpackhi_epi16(val->val[0], val->val[1]);         //12,13, 15,16, 18,19, 21,22,
    v1 =  _mm_alignr_epi8(v2, v0, 12);         //9,10, 12,13, 15,16, 18,19
    v.val[0] =  _mm_shuffle_epi8(v0, *(__m128i*)mask0);         //make holes for the v.val[2] data embedding
    v.val[2] =  _mm_shuffle_epi8(val->val[2], *(__m128i*)mask2lo);         //make plugs for the v.val[2] data embedding
    cff = _mm_cmpeq_epi16(v0, v0);         //all ff
    bldmask = _mm_cmpeq_epi16(*(__m128i*)mask0, cff);
    v.val[0] = _MM_BLENDV_EPI8(v.val[0], v.val[2], bldmask);
    vst1q_u16(ptr,      v.val[0]);
    v.val[0] =  _mm_shuffle_epi8(v1, *(__m128i*)mask1);         //make holes for the v.val[2] data embedding
    v.val[2] =  _mm_shuffle_epi8(val->val[2], *(__m128i*)mask2med);         //make plugs for the v.val[2] data embedding
    bldmask = _mm_cmpeq_epi16(*(__m128i*)mask1, cff);
    v.val[1] = _MM_BLENDV_EPI8(v.val[0],v.val[2], bldmask);
    vst1q_u16((ptr + 8),  v.val[1]);
    v.val[0] =  _mm_shuffle_epi8(v2, *(__m128i*)mask2);         //make holes for the v.val[2] data embedding
    v.val[2] =  _mm_shuffle_epi8(val->val[2], *(__m128i*)mask2hi);         //make plugs for the v.val[2] data embedding
    bldmask = _mm_cmpeq_epi16(*(__m128i*)mask2, cff);
    v.val[2] = _MM_BLENDV_EPI8(v.val[0],v.val[2], bldmask );
    vst1q_u16((ptr + 16), v.val[2]);
}
#define vst3q_u16(ptr, val) vst3q_u16_ptr(ptr, &val)
#endif

//void vst3q_u32(__transfersize(12) uint32_t * ptr, uint32x4x3_t val)// VST3.32 {d0, d2, d4}, [r0]
_NEON2SSE_INLINE void vst3q_u32_ptr(__transfersize(12) uint32_t * ptr, uint32x4x3_t* val)
{   //a0,a1,a2,a3,  b0,b1,b2,b3, c0,c1,c2,c3 -> a0,b0,c0,a1, b1,c1,a2,b2, c2,a3,b3,c3
    uint32x4x3_t v;
    __m128i tmp0, tmp1,tmp2;
    tmp0 = _mm_unpacklo_epi32(val->val[0], val->val[1]); //a0,b0,a1,b1
    tmp1 = _mm_unpackhi_epi32(val->val[0], val->val[1]); //a2,b2,a3,b3
    tmp2 = _mm_unpacklo_epi32(val->val[1], val->val[2]); //b0,c0,b1,c1
    v.val[1] = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(tmp2),_mm_castsi128_ps(tmp1), _MM_SHUFFLE(1,0,3,2))); //b1,c1,a2,b2,
    v.val[2] = _mm_unpackhi_epi64(tmp1, val->val[2]); //a3,b3, c2,c3
    v.val[2] = _mm_shuffle_epi32(v.val[2], 2 | (0 << 2) | (1 << 4) | (3 << 6)); //c2,a3,b3,c3
    tmp1 = _mm_unpacklo_epi32(tmp2,val->val[0]); //b0,a0,c0,a1
    v.val[0] = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(tmp0),_mm_castsi128_ps(tmp1), _MM_SHUFFLE(3,2,1,0))); //a0,b0,c0,a1,

    vst1q_u32(ptr,      v.val[0]);
    vst1q_u32((ptr + 4),  v.val[1]);
    vst1q_u32((ptr + 8),  v.val[2]);
}
#define vst3q_u32(ptr, val) vst3q_u32_ptr(ptr, &val)

#if defined(USE_SSSE3)
//void vst3q_s8(__transfersize(48) int8_t * ptr, int8x16x3_t val);
void vst3q_s8_ptr(__transfersize(48) int8_t * ptr, int8x16x3_t * val);
#define vst3q_s8(ptr, val) vst3q_u8((uint8_t*)(ptr), val)

//void vst3q_s16(__transfersize(24) int16_t * ptr, int16x8x3_t val);
void vst3q_s16_ptr(__transfersize(24) int16_t * ptr, int16x8x3_t * val);
#define vst3q_s16(ptr, val) vst3q_u16((uint16_t*)(ptr), val)
#endif

//void vst3q_s32(__transfersize(12) int32_t * ptr, int32x4x3_t val);
void vst3q_s32_ptr(__transfersize(12) int32_t * ptr, int32x4x3_t * val);
#define vst3q_s32(ptr, val)  vst3q_u32((uint32_t*)(ptr), val)

//void vst3q_f16(__transfersize(24) __fp16 * ptr, float16x8x3_t val);// VST3.16 {d0, d2, d4}, [r0]
void vst3q_f16_ptr(__transfersize(24) __fp16 * ptr, float16x8x3_t * val);
// IA32 SIMD doesn't work with 16bit floats currently

//void vst3q_f32(__transfersize(12) float32_t * ptr, float32x4x3_t val)// VST3.32 {d0, d2, d4}, [r0]
_NEON2SSE_INLINE void vst3q_f32_ptr(__transfersize(12) float32_t * ptr, float32x4x3_t* val)
{
     float32x4x3_t  v;
    __m128 tmp0, tmp1,tmp2;
    tmp0 = _mm_unpacklo_ps(val->val[0], val->val[1]); //a0,b0,a1,b1
    tmp1 = _mm_unpackhi_ps(val->val[0], val->val[1]); //a2,b2,a3,b3
    tmp2 = _mm_unpacklo_ps(val->val[1], val->val[2]); //b0,c0,b1,c1
    v.val[1] = _mm_shuffle_ps(tmp2,tmp1, _MM_SHUFFLE(1,0,3,2)); //b1,c1,a2,b2,
    v.val[2] = _mm_movehl_ps(val->val[2],tmp1); //a3,b3, c2,c3
    v.val[2] = _mm_shuffle_ps(v.val[2],v.val[2], _MM_SHUFFLE(3,1,0,2)); //c2,a3,b3,c3
    tmp1 = _mm_unpacklo_ps(tmp2,val->val[0]); //b0,a0,c0,a1
    v.val[0] = _mm_shuffle_ps(tmp0,tmp1, _MM_SHUFFLE(3,2,1,0)); //a0,b0,c0,a1,

    vst1q_f32( ptr,    v.val[0]);
    vst1q_f32( (ptr + 4),  v.val[1]);
    vst1q_f32( (ptr + 8),  v.val[2]);
}
#define vst3q_f32(ptr, val) vst3q_f32_ptr(ptr, &val)

#if defined(USE_SSSE3)
//void vst3q_p8(__transfersize(48) poly8_t * ptr, poly8x16x3_t val);// VST3.8 {d0, d2, d4}, [r0]
void vst3q_p8_ptr(__transfersize(48) poly8_t * ptr, poly8x16x3_t * val);
#define vst3q_p8 vst3q_u8

//void vst3q_p16(__transfersize(24) poly16_t * ptr, poly16x8x3_t val);// VST3.16 {d0, d2, d4}, [r0]
void vst3q_p16_ptr(__transfersize(24) poly16_t * ptr, poly16x8x3_t * val);
#define vst3q_p16 vst3q_u16
#endif

//void vst3_u8(__transfersize(24) uint8_t * ptr, uint8x8x3_t val)// VST3.8 {d0, d1, d2}, [r0]
#if defined(USE_SSSE3)
_NEON2SSE_INLINE void vst3_u8_ptr(__transfersize(24) uint8_t * ptr, uint8x8x3_t* val)
{
    uint8x8x3_t v;
    __m128i tmp, sh0, sh1;
    _NEON2SSE_ALIGN_16 int8_t mask0[16] = { 0, 8, 16, 1, 9, 17, 2, 10, 18, 3, 11, 19, 4, 12, 20, 5};
    _NEON2SSE_ALIGN_16 int8_t mask1[16] = {13, 21, 6, 14, 22, 7, 15, 23, 0,0,0,0,0,0,0,0};
    _NEON2SSE_ALIGN_16 int8_t mask0_sel[16] = {0, 0, 0xff, 0, 0, 0xff, 0, 0, 0xff, 0, 0, 0xff, 0, 0, 0xff, 0};
    _NEON2SSE_ALIGN_16 int8_t mask1_sel[16] = {0, 0xff, 0, 0, 0xff, 0, 0, 0xff, 0,0,0,0,0,0,0,0};
    tmp = _mm_unpacklo_epi64(val->val[0], val->val[1]);
    sh0 =  _mm_shuffle_epi8(tmp, *(__m128i*)mask0);         //for bi>15 bi is wrapped (bi-=15)
    sh1 =  _mm_shuffle_epi8(val->val[2], *(__m128i*)mask0);
    v.val[0] = _MM_BLENDV_EPI8(sh0, sh1, *(__m128i*)mask0_sel);
    vst1q_u8(ptr,   v.val[0]);         //store as 128 bit structure
    sh0 =  _mm_shuffle_epi8(tmp, *(__m128i*)mask1);         //for bi>15 bi is wrapped (bi-=15)
    sh1 =  _mm_shuffle_epi8(val->val[2], *(__m128i*)mask1);
    v.val[1] = _MM_BLENDV_EPI8(sh0, sh1, *(__m128i*)mask1_sel);
}
#define vst3_u8(ptr, val) vst3_u8_ptr(ptr, &val)
#endif

//void vst3_u16(__transfersize(12) uint16_t * ptr, uint16x4x3_t val)// VST3.16 {d0, d1, d2}, [r0]
#if defined(USE_SSSE3)
_NEON2SSE_INLINE void vst3_u16_ptr(__transfersize(12) uint16_t * ptr, uint16x4x3_t* val)
{
    uint16x4x3_t v;
    __m128i tmp;
    _NEON2SSE_ALIGN_16 int8_t mask0[16] = {0,1, 8,9, 16,17, 2,3, 10,11, 18,19, 4,5, 12,13};
    _NEON2SSE_ALIGN_16 int8_t mask1[16] = {20,21, 6,7, 14,15, 22,23,   0,0,0,0,0,0,0,0};
    _NEON2SSE_ALIGN_16 uint16_t mask0f[8] = {0xffff, 0xffff, 0, 0xffff, 0xffff, 0, 0xffff, 0xffff};         //if all ones we take the result from v.val[0]  otherwise from v.val[1]
    _NEON2SSE_ALIGN_16 uint16_t mask1f[8] = {0xffff, 0, 0, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};         //if all ones we take the result from v.val[1]  otherwise from v.val[0]
    tmp = _mm_unpacklo_epi64(val->val[0], val->val[1]);
    v.val[0] = _mm_shuffle_epi8(tmp, *(__m128i*)mask0);
    v.val[1] = _mm_shuffle_epi8(val->val[2], *(__m128i*)mask0);
    v.val[0] = _MM_BLENDV_EPI8(v.val[1], v.val[0], *(__m128i*)mask0f);
    vst1q_u16(ptr,     v.val[0]);         //store as 128 bit structure
    v.val[0] = _mm_shuffle_epi8(tmp, *(__m128i*)mask1);
    v.val[1] = _mm_shuffle_epi8(val->val[2], *(__m128i*)mask1);
    v.val[1] = _MM_BLENDV_EPI8(v.val[0], v.val[1],  *(__m128i*)mask1f);         //change the operands order
}
#define vst3_u16(ptr, val) vst3_u16_ptr(ptr, &val)
#endif

//void vst3_u32(__transfersize(6) uint32_t * ptr, uint32x2x3_t val)// VST3.32 {d0, d1, d2}, [r0]
_NEON2SSE_INLINE void vst3_u32_ptr(__transfersize(6) uint32_t * ptr, uint32x2x3_t* val)
{         //val->val[0]:0,3,val->val[1]:1,4; val->val[2]:2,5,x,x;
    uint32x2x3_t res;
    res.val[0] = _mm_unpacklo_epi64(val->val[1], val->val[2]);         //val[0]: 1,4,2,5
    res.val[0] = _mm_shuffle_epi32(res.val[0], 0 | (2 << 2) | (1 << 4) | (3 << 6));         //1,2,4,5
    res.val[1] = _mm_srli_si128(res.val[0], 8);         //4,5, x,x
    res.val[0] = _mm_unpacklo_epi32(val->val[0], res.val[0]);         //0,1,3,2
    res.val[0] = _mm_shuffle_epi32(res.val[0], 0 | (1 << 2) | (3 << 4) | (2 << 6));         //0,1,2, 3
    vst1q_u32(ptr, res.val[0]);         //store as 128 bit structure
}
#define vst3_u32(ptr, val) vst3_u32_ptr(ptr, &val)

//void vst3_u64(__transfersize(3) uint64_t * ptr, uint64x1x3_t val)// VST1.64 {d0, d1, d2}, [r0]
_NEON2SSE_INLINE void vst3_u64_ptr(__transfersize(3) uint64_t * ptr, uint64x1x3_t* val)
{
    __m128i tmp;
    tmp =  _mm_unpacklo_epi64(val->val[0], val->val[1]);
    vst1q_u64(ptr, tmp);         //store as 128 bit structure
}
#define vst3_u64(ptr, val) vst3_u64_ptr(ptr, &val)

#if defined(USE_SSSE3)
//void vst3_s8(__transfersize(24) int8_t * ptr, int8x8x3_t val)  // VST3.8 {d0, d1, d2}, [r0]
#define vst3_s8(ptr, val) vst3_u8_ptr((uint8_t*)ptr, &val)

//void vst3_s16(__transfersize(12) int16_t * ptr, int16x4x3_t val)  // VST3.16 {d0, d1, d2}, [r0]
#define vst3_s16(ptr, val) vst3_u16_ptr((uint16_t*)ptr, &val)
#endif

//void vst3_s32(__transfersize(6) int32_t * ptr, int32x2x3_t val); // VST3.32 {d0, d1, d2}, [r0]
#define vst3_s32(ptr, val) vst3_u32_ptr((uint32_t*)ptr, &val)

//void vst3_s64(__transfersize(3) int64_t * ptr, int64x1x3_t val) // VST1.64 {d0, d1, d2}, [r0]
#define vst3_s64(ptr, val) vst3_u64_ptr((uint64_t*)ptr, &val)

//void vst3_f16(__transfersize(12) __fp16 * ptr, float16x4x3_t val);// VST3.16 {d0, d1, d2}, [r0]
void vst3_f16_ptr(__transfersize(12) __fp16 * ptr, float16x4x3_t * val);         // VST3.16 {d0, d1, d2}, [r0]
// IA32 SIMD doesn't work with 16bit floats currently, so need to go to 32 bit and then work with two 128bit registers. See vld1q_f16 for example

//void vst3_f32(__transfersize(6) float32_t * ptr, float32x2x3_t val)// VST3.32 {d0, d1, d2}, [r0]
_NEON2SSE_INLINE void vst3_f32_ptr(__transfersize(6) float32_t * ptr, float32x2x3_t* val)
{         //val->val[0]:0,3,val->val[1]:1,4; val->val[2]:2,5,x,x;
    float32x2x3_t res;
    res.val[0] = _mm_castsi128_ps(_mm_unpacklo_epi64(_mm_castps_si128(val->val[1]), _mm_castps_si128(val->val[2])) );
    res.val[0] = _mm_shuffle_ps(res.val[0],res.val[0], _MM_SHUFFLE(3,1,2,0));         //1,2,4,5
    res.val[1] = _mm_shuffle_ps(res.val[0],res.val[0], _MM_SHUFFLE(1,0,3,2));         //4,5, 1,2
    res.val[0] = _mm_unpacklo_ps(val->val[0], res.val[0]);         //0,1,3, 2
    res.val[0] = _mm_shuffle_ps(res.val[0],res.val[0], _MM_SHUFFLE(2,3,1,0));         //0,1,2, 3
    vst1q_f32(ptr, res.val[0]);         //store as 128 bit structure
}
#define vst3_f32(ptr, val) vst3_f32_ptr(ptr, &val)

#if defined(USE_SSSE3)
//void vst3_p8(__transfersize(24) poly8_t * ptr, poly8x8x3_t val);// VST3.8 {d0, d1, d2}, [r0]
void vst3_p8_ptr(__transfersize(24) poly8_t * ptr, poly8x8x3_t * val);
#define vst3_p8 vst3_u8

//void vst3_p16(__transfersize(12) poly16_t * ptr, poly16x4x3_t val);// VST3.16 {d0, d1, d2}, [r0]
void vst3_p16_ptr(__transfersize(12) poly16_t * ptr, poly16x4x3_t * val);
#define vst3_p16 vst3_s16
#endif

//***************  Quadruples store ********************************
//*********************************************************************
//void vst4q_u8(__transfersize(64) uint8_t * ptr, uint8x16x4_t val)// VST4.8 {d0, d2, d4, d6}, [r0]
_NEON2SSE_INLINE void vst4q_u8_ptr(__transfersize(64) uint8_t * ptr, uint8x16x4_t* val)
{
    __m128i tmp1, tmp2, res;
    tmp1 = _mm_unpacklo_epi8(val->val[0], val->val[1]);         //  0,1, 4,5, 8,9, 12,13, 16,17, 20,21, 24,25, 28,29
    tmp2 = _mm_unpacklo_epi8(val->val[2], val->val[3]);         //  2,3, 6,7, 10,11, 14,15, 18,19, 22,23, 26,27, 30,31
    res = _mm_unpacklo_epi16(tmp1, tmp2);         //0,1, 2,3, 4,5, 6,7, 8,9, 10,11, 12,13, 14,15
    vst1q_u8(ptr,  res);
    res = _mm_unpackhi_epi16(tmp1, tmp2);         //16,17, 18,19, 20,21, 22,23, 24,25, 26,27, 28,29, 30,31
    vst1q_u8((ptr + 16), res);
    tmp1 = _mm_unpackhi_epi8(val->val[0], val->val[1]);         //
    tmp2 = _mm_unpackhi_epi8(val->val[2], val->val[3]);         //
    res = _mm_unpacklo_epi16(tmp1, tmp2);         //
    vst1q_u8((ptr + 32), res);
    res = _mm_unpackhi_epi16(tmp1, tmp2);         //
    vst1q_u8((ptr + 48), res);
}
#define vst4q_u8(ptr, val) vst4q_u8_ptr(ptr, &val)

//void vst4q_u16(__transfersize(32) uint16_t * ptr, uint16x8x4_t val)// VST4.16 {d0, d2, d4, d6}, [r0]
_NEON2SSE_INLINE void vst4q_u16_ptr(__transfersize(32) uint16_t * ptr, uint16x8x4_t* val)
{
    uint16x8x4_t v;
    __m128i tmp1, tmp2;
    tmp1 = _mm_unpacklo_epi16(val->val[0], val->val[1]);         //0,1, 4,5, 8,9, 12,13
    tmp2 = _mm_unpacklo_epi16(val->val[2], val->val[3]);         //2,3, 6,7 , 10,11, 14,15
    v.val[0] = _mm_unpacklo_epi32(tmp1, tmp2);
    v.val[1] = _mm_unpackhi_epi32(tmp1, tmp2);
    tmp1 = _mm_unpackhi_epi16(val->val[0], val->val[1]);         //0,1, 4,5, 8,9, 12,13
    tmp2 = _mm_unpackhi_epi16(val->val[2], val->val[3]);         //2,3, 6,7 , 10,11, 14,15
    v.val[2] = _mm_unpacklo_epi32(tmp1, tmp2);
    v.val[3] = _mm_unpackhi_epi32(tmp1, tmp2);
    vst1q_u16(ptr,     v.val[0]);
    vst1q_u16((ptr + 8), v.val[1]);
    vst1q_u16((ptr + 16),v.val[2]);
    vst1q_u16((ptr + 24), v.val[3]);
}
#define vst4q_u16(ptr, val) vst4q_u16_ptr(ptr, &val)

//void vst4q_u32(__transfersize(16) uint32_t * ptr, uint32x4x4_t val)// VST4.32 {d0, d2, d4, d6}, [r0]
_NEON2SSE_INLINE void vst4q_u32_ptr(__transfersize(16) uint32_t * ptr, uint32x4x4_t* val)
{
    uint16x8x4_t v;
    __m128i tmp1, tmp2;
    tmp1 = _mm_unpacklo_epi32(val->val[0], val->val[1]);         //0,1, 4,5, 8,9, 12,13
    tmp2 = _mm_unpacklo_epi32(val->val[2], val->val[3]);         //2,3, 6,7 , 10,11, 14,15
    v.val[0] = _mm_unpacklo_epi64(tmp1, tmp2);
    v.val[1] = _mm_unpackhi_epi64(tmp1, tmp2);
    tmp1 = _mm_unpackhi_epi32(val->val[0], val->val[1]);         //0,1, 4,5, 8,9, 12,13
    tmp2 = _mm_unpackhi_epi32(val->val[2], val->val[3]);         //2,3, 6,7 , 10,11, 14,15
    v.val[2] = _mm_unpacklo_epi64(tmp1, tmp2);
    v.val[3] = _mm_unpackhi_epi64(tmp1, tmp2);
    vst1q_u32(ptr,      v.val[0]);
    vst1q_u32((ptr + 4),  v.val[1]);
    vst1q_u32((ptr + 8),  v.val[2]);
    vst1q_u32((ptr + 12), v.val[3]);
}
#define vst4q_u32(ptr, val) vst4q_u32_ptr(ptr, &val)

//void vst4q_s8(__transfersize(64) int8_t * ptr, int8x16x4_t val);
void vst4q_s8_ptr(__transfersize(64) int8_t * ptr, int8x16x4_t * val);
#define vst4q_s8(ptr, val) vst4q_u8((uint8_t*)(ptr), val)

//void vst4q_s16(__transfersize(32) int16_t * ptr, int16x8x4_t val);
void vst4q_s16_ptr(__transfersize(32) int16_t * ptr, int16x8x4_t * val);
#define vst4q_s16(ptr, val) vst4q_u16((uint16_t*)(ptr), val)

//void vst4q_s32(__transfersize(16) int32_t * ptr, int32x4x4_t val);
void vst4q_s32_ptr(__transfersize(16) int32_t * ptr, int32x4x4_t * val);
#define vst4q_s32(ptr, val) vst4q_u32((uint32_t*)(ptr), val)

//void vst4q_f16(__transfersize(32) __fp16 * ptr, float16x8x4_t val);// VST4.16 {d0, d2, d4, d6}, [r0]
void vst4q_f16_ptr(__transfersize(32) __fp16 * ptr, float16x8x4_t * val);
// IA32 SIMD doesn't work with 16bit floats currently

//void vst4q_f32(__transfersize(16) float32_t * ptr, float32x4x4_t val)// VST4.32 {d0, d2, d4, d6}, [r0]
_NEON2SSE_INLINE void vst4q_f32_ptr(__transfersize(16) float32_t * ptr, float32x4x4_t* val)
{
    __m128 tmp3, tmp2, tmp1, tmp0;
    float32x4x4_t v;
    tmp0 = _mm_unpacklo_ps(val->val[0], val->val[1]);
    tmp2 = _mm_unpacklo_ps(val->val[2], val->val[3]);
    tmp1 = _mm_unpackhi_ps(val->val[0], val->val[1]);
    tmp3 = _mm_unpackhi_ps(val->val[2], val->val[3]);
    v.val[0] = _mm_movelh_ps(tmp0, tmp2);
    v.val[1] = _mm_movehl_ps(tmp2, tmp0);
    v.val[2] = _mm_movelh_ps(tmp1, tmp3);
    v.val[3] = _mm_movehl_ps(tmp3, tmp1);
    vst1q_f32(ptr,   v.val[0]);
    vst1q_f32((ptr + 4), v.val[1]);
    vst1q_f32((ptr + 8), v.val[2]);
    vst1q_f32((ptr + 12), v.val[3]);
}
#define vst4q_f32(ptr, val) vst4q_f32_ptr(ptr, &val)

//void vst4q_p8(__transfersize(64) poly8_t * ptr, poly8x16x4_t val);// VST4.8 {d0, d2, d4, d6}, [r0]
void vst4q_p8_ptr(__transfersize(64) poly8_t * ptr, poly8x16x4_t * val);
#define vst4q_p8 vst4q_u8

//void vst4q_p16(__transfersize(32) poly16_t * ptr, poly16x8x4_t val);// VST4.16 {d0, d2, d4, d6}, [r0]
void vst4q_p16_ptr(__transfersize(32) poly16_t * ptr, poly16x8x4_t * val);
#define vst4q_p16 vst4q_s16

//void vst4_u8(__transfersize(32) uint8_t * ptr, uint8x8x4_t val)// VST4.8 {d0, d1, d2, d3}, [r0]
_NEON2SSE_INLINE void vst4_u8_ptr(__transfersize(32) uint8_t * ptr, uint8x8x4_t* val)
{
    uint8x8x4_t v;
    __m128i sh0, sh1;
    sh0 = _mm_unpacklo_epi8(val->val[0],val->val[1]);         // a0,b0,a1,b1,a2,b2,a3,b3,a4,b4,a5,b5, a6,b6,a7,b7,
    sh1 = _mm_unpacklo_epi8(val->val[2],val->val[3]);         // c0,d0,c1,d1,c2,d2,c3,d3, c4,d4,c5,d5,c6,d6,c7,d7
    v.val[0] = _mm_unpacklo_epi16(sh0,sh1);         // a0,b0,c0,d0,a1,b1,c1,d1,a2,b2,c2,d2,a3,b3,c3,d3,
    v.val[2] = _mm_unpackhi_epi16(sh0,sh1);         //a4,b4,c4,d4,a5,b5,c5,d5, a6,b6,c6,d6,a7,b7,c7,d7
    vst1q_u8(ptr,      v.val[0]);
    vst1q_u8((ptr + 16),  v.val[2]);
}
#define vst4_u8(ptr, val) vst4_u8_ptr(ptr, &val)

//void vst4_u16(__transfersize(16) uint16_t * ptr, uint16x4x4_t val)// VST4.16 {d0, d1, d2, d3}, [r0]
_NEON2SSE_INLINE void vst4_u16_ptr(__transfersize(16) uint16_t * ptr, uint16x4x4_t* val)
{
    uint16x4x4_t v;
    __m128i sh0, sh1;
    sh0 = _mm_unpacklo_epi16(val->val[0],val->val[1]);         //a0,a1,b0,b1,c0,c1,d0,d1,
    sh1 = _mm_unpacklo_epi16(val->val[2],val->val[3]);         //a2,a3,b2,b3,c2,c3,d2,d3
    v.val[0] = _mm_unpacklo_epi32(sh0,sh1);         // a0,a1,a2,a3,b0,b1,b2,b3
    v.val[2] = _mm_unpackhi_epi32(sh0,sh1);         // c0,c1,c2,c3,d0,d1,d2,d3
    vst1q_u16(ptr,      v.val[0]);         //store as 128 bit structure
    vst1q_u16((ptr + 8),  v.val[2]);
}
#define vst4_u16(ptr, val) vst4_u16_ptr(ptr, &val)

//void vst4_u32(__transfersize(8) uint32_t * ptr, uint32x2x4_t val)// VST4.32 {d0, d1, d2, d3}, [r0]
_NEON2SSE_INLINE void vst4_u32_ptr(__transfersize(8) uint32_t * ptr, uint32x2x4_t* val)
{         //0,4,   1,5,  2,6,  3,7
    uint32x2x4_t v;
    __m128i sh0, sh1;
    sh0 = _mm_unpacklo_epi32(val->val[0], val->val[1]);         //0,1,4,5
    sh1 = _mm_unpacklo_epi32(val->val[2], val->val[3]);         //2,3,6,7
    v.val[0] = _mm_unpacklo_epi64(sh0,sh1);         //
    v.val[1] = _mm_unpackhi_epi64(sh0,sh1);         //
    vst1q_u32(ptr,     v.val[0]);         //store as 128 bit structure
    vst1q_u32((ptr + 4),  v.val[1]);
}
#define vst4_u32(ptr, val) vst4_u32_ptr(ptr, &val)

//void vst4_u64(__transfersize(4) uint64_t * ptr, uint64x1x4_t val)// VST1.64 {d0, d1, d2, d3}, [r0]
_NEON2SSE_INLINE void vst4_u64_ptr(__transfersize(4) uint64_t * ptr, uint64x1x4_t* val)
{
    vst1q_u64(ptr,    val->val[0]);
    vst1q_u64((ptr + 2), val->val[2]);
}
#define vst4_u64(ptr, val) vst4_u64_ptr(ptr, &val)

//void vst4_s8(__transfersize(32) int8_t * ptr, int8x8x4_t val)  //VST4.8 {d0, d1, d2, d3}, [r0]
#define vst4_s8(ptr, val) vst4_u8((uint8_t*)ptr, val)

//void vst4_s16(__transfersize(16) int16_t * ptr, int16x4x4_t val)  // VST4.16 {d0, d1, d2, d3}, [r0]
#define vst4_s16(ptr, val) vst4_u16((uint16_t*)ptr, val)

//void vst4_s32(__transfersize(8) int32_t * ptr, int32x2x4_t val) // VST4.32 {d0, d1, d2, d3}, [r0]
#define vst4_s32(ptr, val) vst4_u32((uint32_t*)ptr, val)

//void vst4_s64(__transfersize(4) int64_t * ptr, int64x1x4_t val); // VST1.64 {d0, d1, d2, d3}, [r0]
void vst4_s64_ptr(__transfersize(4) int64_t * ptr, int64x1x4_t * val);
#define vst4_s64(ptr, val) vst4_u64((uint64_t*)ptr, val)

//void vst4_f16(__transfersize(16) __fp16 * ptr, float16x4x4_t val);// VST4.16 {d0, d1, d2, d3}, [r0]
void vst4_f16_ptr(__transfersize(16) __fp16 * ptr, float16x4x4_t * val);
// IA32 SIMD doesn't work with 16bit floats currently, so need to go to 32 bit and then work with two 128bit registers. See vld1q_f16 for example

//void vst4_f32(__transfersize(8) float32_t * ptr, float32x2x4_t val)// VST4.32 {d0, d1, d2, d3}, [r0]
_NEON2SSE_INLINE void vst4_f32_ptr(__transfersize(8) float32_t * ptr, float32x2x4_t* val)
{         //a0,a1,  b0,b1, c0,c1, d0,d1 -> a0,c0, a1,c1, b0,d0, b1,d1
    float32x2x4_t v;
    v.val[0] = _mm_unpacklo_ps(val->val[0],val->val[1]);
    v.val[2] = _mm_unpacklo_ps(val->val[2],val->val[3]);
    v.val[1] = _mm_movelh_ps (v.val[0], v.val[2]);         //a0, c0, a1,c1,
    v.val[3] = _mm_movehl_ps (v.val[2],v.val[0]);         //b0,d0, b1, d1
    vst1q_f32(ptr,     v.val[1]);         //store as 128 bit structure
    vst1q_f32((ptr + 4),  v.val[3]);
}
#define vst4_f32(ptr, val) vst4_f32_ptr(ptr, &val)

//void vst4_p8(__transfersize(32) poly8_t * ptr, poly8x8x4_t val);// VST4.8 {d0, d1, d2, d3}, [r0]
void vst4_p8_ptr(__transfersize(32) poly8_t * ptr, poly8x8x4_t * val);
#define vst4_p8 vst4_u8

//void vst4_p16(__transfersize(16) poly16_t * ptr, poly16x4x4_t val);// VST4.16 {d0, d1, d2, d3}, [r0]
void vst4_p16_ptr(__transfersize(16) poly16_t * ptr, poly16x4x4_t * val);
#define vst4_p16 vst4_u16

//*********** Store a lane of a vector into memory (extract given lane) for a couple of vectors  *********************
//********************************************************************************************************************
//void vst2q_lane_u16(__transfersize(2) uint16_t * ptr, uint16x8x2_t val, __constrange(0,7) int lane)// VST2.16 {d0[0], d2[0]}, [r0]
_NEON2SSE_INLINE void vst2q_lane_u16_ptr(__transfersize(2) uint16_t * ptr, uint16x8x2_t* val, __constrange(0,7) int lane)
{
    vst1q_lane_s16(ptr, val->val[0], lane);
    vst1q_lane_s16((ptr + 1), val->val[1], lane);
}
#define vst2q_lane_u16(ptr, val, lane) vst2q_lane_u16_ptr(ptr, &val, lane)

//void vst2q_lane_u32(__transfersize(2) uint32_t * ptr, uint32x4x2_t val, __constrange(0,3) int lane)// VST2.32 {d0[0], d2[0]}, [r0]
_NEON2SSE_INLINE void vst2q_lane_u32_ptr(__transfersize(2) uint32_t* ptr, uint32x4x2_t* val, __constrange(0,3) int lane)
{
    vst1q_lane_u32(ptr, val->val[0], lane);
    vst1q_lane_u32((ptr + 1), val->val[1], lane);
}
#define vst2q_lane_u32(ptr, val, lane) vst2q_lane_u32_ptr(ptr, &val, lane)

//void vst2q_lane_s16(__transfersize(2) int16_t * ptr, int16x8x2_t val, __constrange(0,7) int lane);// VST2.16 {d0[0], d2[0]}, [r0]
void vst2q_lane_s16_ptr(__transfersize(2) int16_t * ptr, int16x8x2_t * val, __constrange(0,7) int lane);
#define vst2q_lane_s16(ptr, val, lane) vst2q_lane_u16((uint16_t*)ptr, val, lane)

//void vst2q_lane_s32(__transfersize(2) int32_t * ptr, int32x4x2_t val, __constrange(0,3) int lane);// VST2.32 {d0[0], d2[0]}, [r0]
void vst2q_lane_s32_ptr(__transfersize(2) int32_t * ptr, int32x4x2_t * val, __constrange(0,3) int lane);
#define vst2q_lane_s32(ptr, val, lane)  vst2q_lane_u32((uint32_t*)ptr, val, lane)

//void vst2q_lane_f16(__transfersize(2) __fp16 * ptr, float16x8x2_t val, __constrange(0,7) int lane);// VST2.16 {d0[0], d2[0]}, [r0]
void vst2q_lane_f16_ptr(__transfersize(2) __fp16 * ptr, float16x8x2_t * val, __constrange(0,7) int lane);
//current IA SIMD doesn't support float16

//void vst2q_lane_f32(__transfersize(2) float32_t * ptr, float32x4x2_t val, __constrange(0,3) int lane)// VST2.32 {d0[0], d2[0]}, [r0]
_NEON2SSE_INLINE void vst2q_lane_f32_ptr(__transfersize(2) float32_t* ptr, float32x4x2_t* val, __constrange(0,3) int lane)
{
    vst1q_lane_f32(ptr, val->val[0], lane);
    vst1q_lane_f32((ptr + 1), val->val[1], lane);
}
#define vst2q_lane_f32(ptr, val, lane) vst2q_lane_f32_ptr(ptr, &val, lane)

//void vst2q_lane_p16(__transfersize(2) poly16_t * ptr, poly16x8x2_t val, __constrange(0,7) int lane);// VST2.16 {d0[0], d2[0]}, [r0]
void vst2q_lane_p16_ptr(__transfersize(2) poly16_t * ptr, poly16x8x2_t * val, __constrange(0,7) int lane);
#define vst2q_lane_p16 vst2q_lane_s16

//void vst2_lane_u16(__transfersize(2) uint16_t * ptr, uint16x4x2_t val, __constrange(0,3) int lane);// VST2.16 {d0[0], d1[0]}, [r0]
void vst2_lane_u16_ptr(__transfersize(2) uint16_t * ptr, uint16x4x2_t * val, __constrange(0,3) int lane);         // VST2.16 {d0[0], d1[0]}, [r0]
#define vst2_lane_u16 vst2q_lane_u16

//void vst2_lane_u32(__transfersize(2) uint32_t * ptr, uint32x2x2_t val, __constrange(0,1) int lane);// VST2.32 {d0[0], d1[0]}, [r0]
void vst2_lane_u32_ptr(__transfersize(2) uint32_t * ptr, uint32x2x2_t * val, __constrange(0,1) int lane);         // VST2.32 {d0[0], d1[0]}, [r0]
#define vst2_lane_u32 vst2q_lane_u32

//void vst2_lane_s8(__transfersize(2) int8_t * ptr, int8x8x2_t val, __constrange(0,7) int lane);// VST2.8 {d0[0], d1[0]}, [r0]
void vst2_lane_s8_ptr(__transfersize(2) int8_t * ptr, int8x8x2_t * val, __constrange(0,7) int lane);
#define vst2_lane_s8(ptr, val, lane)  vst2_lane_u8((uint8_t*)ptr, val, lane)

//void vst2_lane_s16(__transfersize(2) int16_t * ptr, int16x4x2_t val, __constrange(0,3) int lane);// VST2.16 {d0[0], d1[0]}, [r0]
void vst2_lane_s16_ptr(__transfersize(2) int16_t * ptr, int16x4x2_t * val, __constrange(0,3) int lane);
#define vst2_lane_s16 vst2q_lane_s16

//void vst2_lane_s32(__transfersize(2) int32_t * ptr, int32x2x2_t val, __constrange(0,1) int lane);// VST2.32 {d0[0], d1[0]}, [r0]
void vst2_lane_s32_ptr(__transfersize(2) int32_t * ptr, int32x2x2_t * val, __constrange(0,1) int lane);
#define vst2_lane_s32 vst2q_lane_s32

//void vst2_lane_f16(__transfersize(2) __fp16 * ptr, float16x4x2_t val, __constrange(0,3) int lane); // VST2.16 {d0[0], d1[0]}, [r0]
//current IA SIMD doesn't support float16

void vst2_lane_f32_ptr(__transfersize(2) float32_t * ptr, float32x2x2_t * val, __constrange(0,1) int lane);         // VST2.32 {d0[0], d1[0]}, [r0]
#define vst2_lane_f32 vst2q_lane_f32

//void vst2_lane_p8(__transfersize(2) poly8_t * ptr, poly8x8x2_t val, __constrange(0,7) int lane);// VST2.8 {d0[0], d1[0]}, [r0]
#define vst2_lane_p8 vst2_lane_u8

//void vst2_lane_p16(__transfersize(2) poly16_t * ptr, poly16x4x2_t val, __constrange(0,3) int lane);// VST2.16 {d0[0], d1[0]}, [r0]
#define vst2_lane_p16 vst2_lane_u16

//************************* Triple lanes  stores *******************************************************
//*******************************************************************************************************
//void vst3q_lane_u16(__transfersize(3) uint16_t * ptr, uint16x8x3_t val, __constrange(0,7) int lane)// VST3.16 {d0[0], d2[0], d4[0]}, [r0]
_NEON2SSE_INLINE void vst3q_lane_u16_ptr(__transfersize(3) uint16_t * ptr, uint16x8x3_t* val, __constrange(0,7) int lane)
{
    vst2q_lane_u16_ptr(ptr, (uint16x8x2_t*)val, lane);
    vst1q_lane_u16((ptr + 2), val->val[2], lane);
}
#define vst3q_lane_u16(ptr, val, lane) vst3q_lane_u16_ptr(ptr, &val, lane)

//void vst3q_lane_u32(__transfersize(3) uint32_t * ptr, uint32x4x3_t val, __constrange(0,3) int lane)// VST3.32 {d0[0], d2[0], d4[0]}, [r0]
_NEON2SSE_INLINE void vst3q_lane_u32_ptr(__transfersize(3) uint32_t * ptr, uint32x4x3_t* val, __constrange(0,3) int lane)
{
    vst2q_lane_u32_ptr(ptr, (uint32x4x2_t*)val, lane);
    vst1q_lane_u32((ptr + 2), val->val[2], lane);
}
#define vst3q_lane_u32(ptr, val, lane) vst3q_lane_u32_ptr(ptr, &val, lane)

//void vst3q_lane_s16(__transfersize(3) int16_t * ptr, int16x8x3_t val, __constrange(0,7) int lane);// VST3.16 {d0[0], d2[0], d4[0]}, [r0]
void vst3q_lane_s16_ptr(__transfersize(3) int16_t * ptr, int16x8x3_t * val, __constrange(0,7) int lane);
#define vst3q_lane_s16(ptr, val, lane) vst3q_lane_u16((uint16_t *)ptr, val, lane)

//void vst3q_lane_s32(__transfersize(3) int32_t * ptr, int32x4x3_t val, __constrange(0,3) int lane);// VST3.32 {d0[0], d2[0], d4[0]}, [r0]
void vst3q_lane_s32_ptr(__transfersize(3) int32_t * ptr, int32x4x3_t * val, __constrange(0,3) int lane);
#define vst3q_lane_s32(ptr, val, lane) vst3q_lane_u32((uint32_t *)ptr, val, lane)

//void vst3q_lane_f16(__transfersize(3) __fp16 * ptr, float16x8x3_t val, __constrange(0,7) int lane);// VST3.16 {d0[0], d2[0], d4[0]}, [r0]
void vst3q_lane_f16_ptr(__transfersize(3) __fp16 * ptr, float16x8x3_t * val, __constrange(0,7) int lane);
//current IA SIMD doesn't support float16

//void vst3q_lane_f32(__transfersize(3) float32_t * ptr, float32x4x3_t val, __constrange(0,3) int lane)// VST3.32 {d0[0], d2[0], d4[0]}, [r0]
_NEON2SSE_INLINE void vst3q_lane_f32_ptr(__transfersize(3) float32_t * ptr, float32x4x3_t* val, __constrange(0,3) int lane)
{
    vst1q_lane_f32(ptr,   val->val[0], lane);
    vst1q_lane_f32((ptr + 1),   val->val[1], lane);
    vst1q_lane_f32((ptr + 2), val->val[2], lane);
}
#define vst3q_lane_f32(ptr, val, lane) vst3q_lane_f32_ptr(ptr, &val, lane)

//void vst3_lane_s8(__transfersize(3) int8_t * ptr, int8x8x3_t val, __constrange(0,7) int lane);// VST3.8 {d0[0], d1[0], d2[0]}, [r0]
void vst3_lane_s8_ptr(__transfersize(3) int8_t * ptr, int8x8x3_t * val, __constrange(0,7) int lane);
#define  vst3_lane_s8(ptr, val, lane) vst3_lane_u8((uint8_t *)ptr, val, lane)

//void vst3_lane_s16(__transfersize(3) int16_t * ptr, int16x4x3_t val, __constrange(0,3) int lane);// VST3.16 {d0[0], d1[0], d2[0]}, [r0]
void vst3_lane_s16_ptr(__transfersize(3) int16_t * ptr, int16x4x3_t * val, __constrange(0,3) int lane);
#define vst3_lane_s16(ptr, val, lane) vst3_lane_u16((uint16_t *)ptr, val, lane)

//void vst3_lane_s32(__transfersize(3) int32_t * ptr, int32x2x3_t val, __constrange(0,1) int lane);// VST3.32 {d0[0], d1[0], d2[0]}, [r0]
void vst3_lane_s32_ptr(__transfersize(3) int32_t * ptr, int32x2x3_t * val, __constrange(0,1) int lane);
#define vst3_lane_s32(ptr, val, lane) vst3_lane_u32((uint32_t *)ptr, val, lane)

//void vst3_lane_f16(__transfersize(3) __fp16 * ptr, float16x4x3_t val, __constrange(0,3) int lane);// VST3.16 {d0[0], d1[0], d2[0]}, [r0]
void vst3_lane_f16_ptr(__transfersize(3) __fp16 * ptr, float16x4x3_t * val, __constrange(0,3) int lane);
//current IA SIMD doesn't support float16

//void vst3_lane_f32(__transfersize(3) float32_t * ptr, float32x2x3_t val, __constrange(0,1) int lane)// VST3.32 {d0[0], d1[0], d2[0]}, [r0]
void vst3_lane_f32_ptr(__transfersize(3) float32_t * ptr, float32x2x3_t * val, __constrange(0,1) int lane);
#define vst3_lane_f32 vst3q_lane_f32

//void vst3_lane_p8(__transfersize(3) poly8_t * ptr, poly8x8x3_t val, __constrange(0,7) int lane);// VST3.8 {d0[0], d1[0], d2[0]}, [r0]
void vst3_lane_p8_ptr(__transfersize(3) poly8_t * ptr, poly8x8x3_t * val, __constrange(0,7) int lane);
#define vst3_lane_p8 vst3_lane_u8

//void vst3_lane_p16(__transfersize(3) poly16_t * ptr, poly16x4x3_t val, __constrange(0,3) int lane);// VST3.16 {d0[0], d1[0], d2[0]}, [r0]
void vst3_lane_p16_ptr(__transfersize(3) poly16_t * ptr, poly16x4x3_t * val, __constrange(0,3) int lane);
#define vst3_lane_p16 vst3_lane_s16

//******************************** Quadruple lanes stores ***********************************************
//*******************************************************************************************************
//void vst4q_lane_u16(__transfersize(4) uint16_t * ptr, uint16x8x4_t val, __constrange(0,7) int lane)// VST4.16 {d0[0], d2[0], d4[0], d6[0]}, [r0]
_NEON2SSE_INLINE void vst4q_lane_u16_ptr(__transfersize(4) uint16_t * ptr, uint16x8x4_t* val4, __constrange(0,7) int lane)
{
    vst2q_lane_u16_ptr(ptr,    (uint16x8x2_t*)val4->val, lane);
    vst2q_lane_u16_ptr((ptr + 2),((uint16x8x2_t*)val4->val + 1), lane);
}
#define vst4q_lane_u16(ptr, val, lane) vst4q_lane_u16_ptr(ptr, &val, lane)

//void vst4q_lane_u32(__transfersize(4) uint32_t * ptr, uint32x4x4_t val, __constrange(0,3) int lane)// VST4.32 {d0[0], d2[0], d4[0], d6[0]}, [r0]
_NEON2SSE_INLINE void vst4q_lane_u32_ptr(__transfersize(4) uint32_t * ptr, uint32x4x4_t* val4, __constrange(0,3) int lane)
{
    vst2q_lane_u32_ptr(ptr,     (uint32x4x2_t*)val4->val, lane);
    vst2q_lane_u32_ptr((ptr + 2), ((uint32x4x2_t*)val4->val + 1), lane);
}
#define vst4q_lane_u32(ptr, val, lane) vst4q_lane_u32_ptr(ptr, &val, lane)

//void vst4q_lane_s16(__transfersize(4) int16_t * ptr, int16x8x4_t val, __constrange(0,7) int lane);// VST4.16 {d0[0], d2[0], d4[0], d6[0]}, [r0]
void vst4q_lane_s16_ptr(__transfersize(4) int16_t * ptr, int16x8x4_t * val, __constrange(0,7) int lane);
#define vst4q_lane_s16(ptr,val,lane) vst4q_lane_u16((uint16_t *)ptr,val,lane)

//void vst4q_lane_s32(__transfersize(4) int32_t * ptr, int32x4x4_t val, __constrange(0,3) int lane);// VST4.32 {d0[0], d2[0], d4[0], d6[0]}, [r0]
void vst4q_lane_s32_ptr(__transfersize(4) int32_t * ptr, int32x4x4_t * val, __constrange(0,3) int lane);
#define vst4q_lane_s32(ptr,val,lane) vst4q_lane_u32((uint32_t *)ptr,val,lane)

//void vst4q_lane_f16(__transfersize(4) __fp16 * ptr, float16x8x4_t val, __constrange(0,7) int lane);// VST4.16 {d0[0], d2[0], d4[0], d6[0]}, [r0]
void vst4q_lane_f16_ptr(__transfersize(4) __fp16 * ptr, float16x8x4_t * val, __constrange(0,7) int lane);
//current IA SIMD doesn't support float16

//void vst4q_lane_f32(__transfersize(4) float32_t * ptr, float32x4x4_t val, __constrange(0,3) int lane)// VST4.32 {d0[0], d2[0], d4[0], d6[0]}, [r0]
_NEON2SSE_INLINE void vst4q_lane_f32_ptr(__transfersize(4) float32_t * ptr, float32x4x4_t* val, __constrange(0,3) int lane)
{
    vst1q_lane_f32(ptr,   val->val[0], lane);
    vst1q_lane_f32((ptr + 1), val->val[1], lane);
    vst1q_lane_f32((ptr + 2), val->val[2], lane);
    vst1q_lane_f32((ptr + 3), val->val[3], lane);
}
#define vst4q_lane_f32(ptr, val, lane) vst4q_lane_f32_ptr(ptr, &val, lane)

//void vst4q_lane_p16(__transfersize(4) poly16_t * ptr, poly16x8x4_t val, __constrange(0,7) int lane);// VST4.16 {d0[0], d2[0], d4[0], d6[0]}, [r0]
void vst4q_lane_p16_ptr(__transfersize(4) poly16_t * ptr, poly16x8x4_t * val, __constrange(0,7) int lane);
#define vst4q_lane_p16 vst4q_lane_u16

//void vst4_lane_u8(__transfersize(4) uint8_t * ptr, uint8x8x4_t val, __constrange(0,7) int lane)// VST4.8 {d0[0], d1[0], d2[0], d3[0]}, [r0]
_NEON2SSE_INLINE void vst4_lane_u8_ptr(__transfersize(4) uint8_t * ptr, uint8x8x4_t* val, __constrange(0,7) int lane)
{
    vst1q_lane_u8(ptr,   val->val[0], lane);
    vst1q_lane_u8((ptr + 1),  val->val[1], lane);
    vst1q_lane_u8((ptr + 2), val->val[2], lane);
    vst1q_lane_u8((ptr + 3), val->val[3], lane);
}
#define vst4_lane_u8(ptr, val, lane) vst4_lane_u8_ptr(ptr, &val, lane)

//void vst4_lane_u16(__transfersize(4) uint16_t * ptr, uint16x4x4_t val, __constrange(0,3) int lane)// VST4.16 {d0[0], d1[0], d2[0], d3[0]}, [r0]
_NEON2SSE_INLINE void vst4_lane_u16_ptr(__transfersize(4) uint16_t * ptr, uint16x4x4_t* val, __constrange(0,3) int lane)
{
    vst1q_lane_u16(ptr,   val->val[0], lane);
    vst1q_lane_u16((ptr + 1),val->val[1], lane);
    vst1q_lane_u16((ptr + 2), val->val[2], lane);
    vst1q_lane_u16((ptr + 3), val->val[3], lane);
}
#define vst4_lane_u16(ptr, val, lane) vst4_lane_u16_ptr(ptr, &val, lane)

//void vst4_lane_u32(__transfersize(4) uint32_t * ptr, uint32x2x4_t val, __constrange(0,1) int lane)// VST4.32 {d0[0], d1[0], d2[0], d3[0]}, [r0]
_NEON2SSE_INLINE void vst4_lane_u32_ptr(__transfersize(4) uint32_t * ptr, uint32x2x4_t* val, __constrange(0,1) int lane)
{
    vst1q_lane_u32(ptr,   val->val[0], lane);
    vst1q_lane_u32((ptr + 1), val->val[1], lane);
    vst1q_lane_u32((ptr + 2), val->val[2], lane);
    vst1q_lane_u32((ptr + 3), val->val[3], lane);

}
#define vst4_lane_u32(ptr, val, lane) vst4_lane_u32_ptr(ptr, &val, lane)

//void vst4_lane_s8(__transfersize(4) int8_t * ptr, int8x8x4_t val, __constrange(0,7) int lane)// VST4.8 {d0[0], d1[0], d2[0], d3[0]}, [r0]
#define vst4_lane_s8(ptr, val, lane) vst4_lane_u8((uint8_t*)ptr, val, lane)

//void vst4_lane_s16(__transfersize(4) int16_t * ptr, int16x4x4_t val, __constrange(0,3) int lane)// VST4.16 {d0[0], d1[0], d2[0], d3[0]}, [r0]
#define vst4_lane_s16(ptr, val, lane) vst4_lane_u16((uint16_t*)ptr, val, lane)

//void vst4_lane_s32(__transfersize(4) int32_t * ptr, int32x2x4_t val, __constrange(0,1) int lane)// VST4.32 {d0[0], d1[0], d2[0], d3[0]}, [r0]
#define vst4_lane_s32(ptr, val, lane) vst4_lane_u32((uint32_t*)ptr, val, lane)

//void vst4_lane_f16(__transfersize(4) __fp16 * ptr, float16x4x4_t val, __constrange(0,3) int lane);// VST4.16 {d0[0], d1[0], d2[0], d3[0]}, [r0]
void vst4_lane_f16_ptr(__transfersize(4) __fp16 * ptr, float16x4x4_t * val, __constrange(0,3) int lane);
//current IA SIMD doesn't support float16

//void vst4_lane_f32(__transfersize(4) float32_t * ptr, float32x2x4_t val, __constrange(0,1) int lane)// VST4.32 {d0[0], d1[0], d2[0], d3[0]}, [r0]
#define vst4_lane_f32 vst4q_lane_f32

//void vst4_lane_p8(__transfersize(4) poly8_t * ptr, poly8x8x4_t val, __constrange(0,7) int lane);// VST4.8 {d0[0], d1[0], d2[0], d3[0]}, [r0]
void vst4_lane_p8_ptr(__transfersize(4) poly8_t * ptr, poly8x8x4_t * val, __constrange(0,7) int lane);
#define vst4_lane_p8 vst4_lane_u8

//void vst4_lane_p16(__transfersize(4) poly16_t * ptr, poly16x4x4_t val, __constrange(0,3) int lane);// VST4.16 {d0[0], d1[0], d2[0], d3[0]}, [r0]
void vst4_lane_p16_ptr(__transfersize(4) poly16_t * ptr, poly16x4x4_t * val, __constrange(0,3) int lane);
#define vst4_lane_p16 vst4_lane_u16

//**************************************************************************************************
//************************ Extract lanes from a vector ********************************************
//**************************************************************************************************
//These intrinsics extract a single lane (element) from a vector.

uint8_t vgetq_lane_u8(uint8x16_t vec, __constrange(0,15) int lane);         // VMOV.U8 r0, d0[0]
#define vgetq_lane_u8 _MM_EXTRACT_EPI8

uint16_t vgetq_lane_u16(uint16x8_t vec, __constrange(0,7) int lane);         // VMOV.s16 r0, d0[0]
#define  vgetq_lane_u16 _MM_EXTRACT_EPI16

uint32_t vgetq_lane_u32(uint32x4_t vec, __constrange(0,3) int lane);         // VMOV.32 r0, d0[0]
#define vgetq_lane_u32 _MM_EXTRACT_EPI32

int8_t vgetq_lane_s8(int8x16_t vec, __constrange(0,15) int lane);         // VMOV.S8 r0, d0[0]
#define vgetq_lane_s8 vgetq_lane_u8

int16_t vgetq_lane_s16(int16x8_t vec, __constrange(0,7) int lane);         // VMOV.S16 r0, d0[0]
#define vgetq_lane_s16 vgetq_lane_u16

int32_t vgetq_lane_s32(int32x4_t vec, __constrange(0,3) int lane);         // VMOV.32 r0, d0[0]
#define vgetq_lane_s32 vgetq_lane_u32

poly8_t vgetq_lane_p8(poly8x16_t vec, __constrange(0,15) int lane);         // VMOV.U8 r0, d0[0]
#define vgetq_lane_p8 vgetq_lane_u8

poly16_t vgetq_lane_p16(poly16x8_t vec, __constrange(0,7) int lane);         // VMOV.s16 r0, d0[0]
#define vgetq_lane_p16 vgetq_lane_u16

float32_t vgetq_lane_f32(float32x4_t vec, __constrange(0,3) int lane);         // VMOV.32 r0, d0[0]
_NEON2SSE_INLINE float32_t vgetq_lane_f32(float32x4_t vec, __constrange(0,3) int lane)
{
    int32_t ilane;
    ilane = _MM_EXTRACT_PS(vec,lane);
    return *(float*)&ilane;
}

int64_t vgetq_lane_s64(int64x2_t vec, __constrange(0,1) int lane);         // VMOV r0,r0,d0
#define vgetq_lane_s64 (int64_t) vgetq_lane_u64

uint64_t vgetq_lane_u64(uint64x2_t vec, __constrange(0,1) int lane);         // VMOV r0,r0,d0
#define vgetq_lane_u64 _MM_EXTRACT_EPI64

// ***************** Set lanes within a vector ********************************************
// **************************************************************************************
//These intrinsics set a single lane (element) within a vector.
//same functions as vld1_lane_xx ones, but take the value to be set directly.

uint8x16_t vsetq_lane_u8(uint8_t value, uint8x16_t vec, __constrange(0,15) int lane);         // VMOV.8 d0[0],r0
_NEON2SSE_INLINE uint8x16_t vsetq_lane_u8(uint8_t value, uint8x16_t vec, __constrange(0,15) int lane)
{
    uint8_t val;
    val = value;
    return vld1q_lane_u8(&val, vec,  lane);
}

uint16x8_t vsetq_lane_u16(uint16_t value, uint16x8_t vec, __constrange(0,7) int lane);         // VMOV.16 d0[0],r0
_NEON2SSE_INLINE uint16x8_t vsetq_lane_u16(uint16_t value, uint16x8_t vec, __constrange(0,7) int lane)
{
    uint16_t val;
    val = value;
    return vld1q_lane_u16(&val, vec,  lane);
}

uint32x4_t vsetq_lane_u32(uint32_t value, uint32x4_t vec, __constrange(0,3) int lane);         // VMOV.32 d0[0],r0
_NEON2SSE_INLINE uint32x4_t vsetq_lane_u32(uint32_t value, uint32x4_t vec, __constrange(0,3) int lane)
{
    uint32_t val;
    val = value;
    return vld1q_lane_u32(&val, vec,  lane);
}

int8x16_t vsetq_lane_s8(int8_t value, int8x16_t vec, __constrange(0,15) int lane);         // VMOV.8 d0[0],r0
_NEON2SSE_INLINE int8x16_t vsetq_lane_s8(int8_t value, int8x16_t vec, __constrange(0,15) int lane)
{
    int8_t val;
    val = value;
    return vld1q_lane_s8(&val, vec,  lane);
}

int16x8_t vsetq_lane_s16(int16_t value, int16x8_t vec, __constrange(0,7) int lane);         // VMOV.16 d0[0],r0
_NEON2SSE_INLINE int16x8_t vsetq_lane_s16(int16_t value, int16x8_t vec, __constrange(0,7) int lane)
{
    int16_t val;
    val = value;
    return vld1q_lane_s16(&val, vec,  lane);
}

int32x4_t vsetq_lane_s32(int32_t value, int32x4_t vec, __constrange(0,3) int lane);         // VMOV.32 d0[0],r0
_NEON2SSE_INLINE int32x4_t vsetq_lane_s32(int32_t value, int32x4_t vec, __constrange(0,3) int lane)
{
    int32_t val;
    val = value;
    return vld1q_lane_s32(&val, vec,  lane);
}

poly8x16_t vsetq_lane_p8(poly8_t value, poly8x16_t vec, __constrange(0,15) int lane);         // VMOV.8 d0[0],r0
#define vsetq_lane_p8 vsetq_lane_u8

poly16x8_t vsetq_lane_p16(poly16_t value, poly16x8_t vec, __constrange(0,7) int lane);         // VMOV.16 d0[0],r0
#define vsetq_lane_p16 vsetq_lane_u16

float32x4_t vsetq_lane_f32(float32_t value, float32x4_t vec, __constrange(0,3) int lane);         // VMOV.32 d0[0],r0
_NEON2SSE_INLINE float32x4_t vsetq_lane_f32(float32_t value, float32x4_t vec, __constrange(0,3) int lane)
{
    float32_t val;
    val = value;
}

int64x2_t vsetq_lane_s64(int64_t value, int64x2_t vec, __constrange(0,1) int lane);         // VMOV d0,r0,r0
_NEON2SSE_INLINE int64x2_t vsetq_lane_s64(int64_t value, int64x2_t vec, __constrange(0,1) int lane)
{
    uint64_t val;
    val = value;
    return vld1q_lane_s64(&val, vec,  lane);
}

uint64x2_t vsetq_lane_u64(uint64_t value, uint64x2_t vec, __constrange(0,1) int lane);         // VMOV d0,r0,r0
#define vsetq_lane_u64 vsetq_lane_s64

// *******************************************************************************
// **************** Initialize a vector from bit pattern ***************************
// *******************************************************************************
//These intrinsics create a vector from a literal bit pattern.

//no IA32 SIMD avalilable

//********************* Set all lanes to same value ********************************
//*********************************************************************************
//These intrinsics set all lanes to the same value.

uint8x16_t   vdupq_n_u8(uint8_t value);         // VDUP.8 q0,r0
#define vdupq_n_u8(value) _mm_set1_epi8((uint8_t) (value))

uint16x8_t   vdupq_n_u16(uint16_t value);         // VDUP.16 q0,r0
#define vdupq_n_u16(value) _mm_set1_epi16((uint16_t) (value))

uint32x4_t   vdupq_n_u32(uint32_t value);         // VDUP.32 q0,r0
#define vdupq_n_u32(value) _mm_set1_epi32((uint32_t) (value))

int8x16_t   vdupq_n_s8(int8_t value);         // VDUP.8 q0,r0
#define vdupq_n_s8 _mm_set1_epi8

int16x8_t   vdupq_n_s16(int16_t value);         // VDUP.16 q0,r0
#define vdupq_n_s16 _mm_set1_epi16

int32x4_t   vdupq_n_s32(int32_t value);         // VDUP.32 q0,r0
#define vdupq_n_s32 _mm_set1_epi32

poly8x16_t vdupq_n_p8(poly8_t value);         // VDUP.8 q0,r0
#define  vdupq_n_p8 vdupq_n_u8

poly16x8_t vdupq_n_p16(poly16_t value);         // VDUP.16 q0,r0
#define  vdupq_n_p16 vdupq_n_u16

float32x4_t vdupq_n_f32(float32_t value);         // VDUP.32 q0,r0
#define vdupq_n_f32 _mm_set1_ps

int64x2_t   vdupq_n_s64(int64_t value);         // VMOV d0,r0,r0
_NEON2SSE_INLINE int64x2_t   vdupq_n_s64(int64_t value)
{
    _NEON2SSE_ALIGN_16 int64_t value2[2] = {value, value};         //value may be an immediate
    return LOAD_SI128(value2);
}

uint64x2_t   vdupq_n_u64(uint64_t value);         // VMOV d0,r0,r0
_NEON2SSE_INLINE uint64x2_t   vdupq_n_u64(uint64_t value)
{
    _NEON2SSE_ALIGN_16 uint64_t val[2] = {value, value};         //value may be an immediate
    return LOAD_SI128(val);
}

//****  Set all lanes to same value  ************************
//Same functions as above - just aliaces.********************
//Probably they reflect the fact that 128-bit functions versions use VMOV instruction **********

uint8x16_t vmovq_n_u8(uint8_t value);         // VDUP.8 q0,r0
#define vmovq_n_u8 vdupq_n_u8

uint16x8_t vmovq_n_u16(uint16_t value);         // VDUP.16 q0,r0
#define vmovq_n_u16 vdupq_n_s16

uint32x4_t vmovq_n_u32(uint32_t value);         // VDUP.32 q0,r0
#define vmovq_n_u32 vdupq_n_u32

int8x16_t vmovq_n_s8(int8_t value);         // VDUP.8 q0,r0
#define vmovq_n_s8 vdupq_n_s8

int16x8_t vmovq_n_s16(int16_t value);         // VDUP.16 q0,r0
#define vmovq_n_s16 vdupq_n_s16

int32x4_t vmovq_n_s32(int32_t value);         // VDUP.32 q0,r0
#define vmovq_n_s32 vdupq_n_s32

poly8x16_t vmovq_n_p8(poly8_t value);         // VDUP.8 q0,r0
#define vmovq_n_p8 vdupq_n_u8

poly16x8_t vmovq_n_p16(poly16_t value);         // VDUP.16 q0,r0
#define vmovq_n_p16 vdupq_n_s16

float32x4_t vmovq_n_f32(float32_t value);         // VDUP.32 q0,r0
#define vmovq_n_f32 vdupq_n_f32

int64x2_t vmovq_n_s64(int64_t value);         // VMOV d0,r0,r0
#define vmovq_n_s64 vdupq_n_s64

uint64x2_t vmovq_n_u64(uint64_t value);         // VMOV d0,r0,r0
#define vmovq_n_u64 vdupq_n_u64

//**************Set all lanes to the value of one lane of a vector *************
//****************************************************************************
//here shuffle is better solution than lane extraction followed by set1 function

// ********************************************************************
// ********************  Combining vectors *****************************
// ********************************************************************
//These intrinsics join two 64 bit vectors into a single 128bit vector.

//current IA SIMD doesn't support float16

//**********************************************************************
//************************* Splitting vectors **************************
//**********************************************************************
//**************** Get high part ******************************************
//These intrinsics split a 128 bit vector into 2 component 64 bit vectors

// IA32 SIMD doesn't work with 16bit floats currently

//********************** Get low part **********************
//**********************************************************

// IA32 SIMD doesn't work with 16bit floats currently

//**************************************************************************
//************************ Converting vectors **********************************
//**************************************************************************
//************* Convert from float ***************************************
// need to set _MM_SET_ROUNDING_MODE ( x) accordingly

int32x4_t   vcvtq_s32_f32(float32x4_t a);         // VCVT.S32.F32 q0, q0
#define vcvtq_s32_f32 _mm_cvtps_epi32

uint32x4_t vcvtq_u32_f32(float32x4_t a);         // VCVT.U32.F32 q0, q0
_NEON2SSE_INLINE uint32x4_t vcvtq_u32_f32(float32x4_t a)         // VCVT.U32.F32 q0, q0
{         //No single instruction SSE solution  but we could implement it as following:
    __m128i resi;
    __m128 zero,  mask, a_pos, mask_f_max_si, res;
    _NEON2SSE_ALIGN_16 int32_t c7fffffff[4] = {0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff};
    zero = _mm_setzero_ps();
    mask = _mm_cmpgt_ps(a, zero);
    a_pos = _mm_and_ps(a, mask);
    mask_f_max_si = _mm_cmpgt_ps(a_pos,*(__m128*)c7fffffff);
    res =  _mm_sub_ps(a_pos, mask_f_max_si);         //if the input fits to signed we don't subtract anything
    resi = _mm_cvtps_epi32(res);
    return _mm_add_epi32(resi, *(__m128i*)&mask_f_max_si);
}

// ***** Convert to the fixed point  with   the number of fraction bits specified by b ***********
//*************************************************************************************************
//Intel SIMD doesn't support fixed point

int32x4_t vcvtq_n_s32_f32(float32x4_t a, __constrange(1,32) int b);         // VCVT.S32.F32 q0, q0, #32
uint32x4_t vcvtq_n_u32_f32(float32x4_t a, __constrange(1,32) int b);         // VCVT.U32.F32 q0, q0, #32

//***************** Convert to float *************************
//*************************************************************

float32x4_t vcvtq_f32_s32(int32x4_t a);         // VCVT.F32.S32 q0, q0
#define vcvtq_f32_s32(a) _mm_cvtepi32_ps(a)

float32x4_t vcvtq_f32_u32(uint32x4_t a);         // VCVT.F32.U32 q0, q0
_NEON2SSE_INLINE float32x4_t vcvtq_f32_u32(uint32x4_t a)         // VCVT.F32.U32 q0, q0
{         //solution may be not optimal
    __m128 two16, fHi, fLo;
    __m128i hi, lo;
    two16 = _mm_set1_ps((float)0x10000);         //2^16
    // Avoid double rounding by doing two exact conversions
    // of high and low 16-bit segments
    hi = _mm_srli_epi32(a, 16);
    lo = _mm_srli_epi32(_mm_slli_epi32(a, 16), 16);
    fHi = _mm_mul_ps(_mm_cvtepi32_ps(hi), two16);
    fLo = _mm_cvtepi32_ps(lo);
    // do single rounding according to current rounding mode
    return _mm_add_ps(fHi, fLo);
}

//**************Convert between floats ***********************
//************************************************************

//Intel SIMD doesn't support 16bits floats curently

//Intel SIMD doesn't support 16bits floats curently, the only solution is to store 16bit floats and load as 32 bits

//************Vector narrow integer conversion (truncation) ******************
//****************************************************************************

//**************** Vector long move   ***********************
//***********************************************************

//*************Vector saturating narrow integer*****************
//**************************************************************

//************* Vector saturating narrow integer signed->unsigned **************
//*****************************************************************************

// ********************************************************
// **************** Table look up **************************
// ********************************************************
//VTBL (Vector Table Lookup) uses byte indexes in a control vector to look up byte values
//in a table and generate a new vector. Indexes out of range return 0.
//for Intel SIMD we need to set the MSB to 1 for zero return

//Special trick to avoid __declspec(align('8')) won't be aligned" error

//Special trick to avoid __declspec(align('16')) won't be aligned" error

//****************** Extended table look up intrinsics ***************************
//**********************************************************************************
//VTBX (Vector Table Extension) works in the same way as VTBL do,
// except that indexes out of range leave the destination element unchanged.

//Special trick to avoid __declspec(align('8')) won't be aligned" error

//*************************************************************************************************
// *************************** Operations with a scalar value *********************************
//*************************************************************************************************

//******* Vector multiply accumulate by scalar *************************************************
//**********************************************************************************************

//***************** Vector widening multiply accumulate by scalar **********************
//***************************************************************************************

// ******** Vector widening saturating doubling multiply accumulate by scalar *******************************
// ************************************************************************************************

// ****** Vector multiply subtract by scalar *****************
// *************************************************************

// **** Vector widening multiply subtract by scalar ****
// ****************************************************

//********* Vector widening saturating doubling multiply subtract by scalar **************************
//******************************************************************************************************

//********** Vector multiply with scalar *****************************

int16x8_t vmulq_n_s16(int16x8_t a, int16_t b);         // VMUL.I16 q0,q0,d0[0]
_NEON2SSE_INLINE int16x8_t vmulq_n_s16(int16x8_t a, int16_t b)         // VMUL.I16 q0,q0,d0[0]
{
    int16x8_t b16x8;
    b16x8 = vdupq_n_s16(b);
    return vmulq_s16(a, b16x8);
}

int32x4_t vmulq_n_s32(int32x4_t a, int32_t b);         // VMUL.I32 q0,q0,d0[0]
_NEON2SSE_INLINE int32x4_t vmulq_n_s32(int32x4_t a, int32_t b)         // VMUL.I32 q0,q0,d0[0]
{
    int32x4_t b32x4;
    b32x4 = vdupq_n_s32(b);
    return vmulq_s32(a, b32x4);
}

float32x4_t vmulq_n_f32(float32x4_t a, float32_t b);         // VMUL.F32 q0,q0,d0[0]
_NEON2SSE_INLINE float32x4_t vmulq_n_f32(float32x4_t a, float32_t b)         // VMUL.F32 q0,q0,d0[0]
{
    float32x4_t b32x4;
    b32x4 = vdupq_n_f32(b);
    return vmulq_f32(a, b32x4);
}

uint16x8_t vmulq_n_u16(uint16x8_t a, uint16_t b);         // VMUL.I16 q0,q0,d0[0]
_NEON2SSE_INLINE uint16x8_t vmulq_n_u16(uint16x8_t a, uint16_t b)         // VMUL.I16 q0,q0,d0[0]
{
    uint16x8_t b16x8;
    b16x8 = vdupq_n_s16(b);
    return vmulq_s16(a, b16x8);
}

uint32x4_t vmulq_n_u32(uint32x4_t a, uint32_t b);         // VMUL.I32 q0,q0,d0[0]
_NEON2SSE_INLINE uint32x4_t vmulq_n_u32(uint32x4_t a, uint32_t b)         // VMUL.I32 q0,q0,d0[0]
{
    uint32x4_t b32x4;
    b32x4 = vdupq_n_u32(b);
    return vmulq_u32(a, b32x4);
}

//**** Vector long multiply with scalar ************

//**** Vector long multiply by scalar ****

//********* Vector saturating doubling long multiply with scalar  *******************

//************* Vector saturating doubling long multiply by scalar ***********************************************

// *****Vector saturating doubling multiply high with scalar *****

int16x8_t vqdmulhq_n_s16(int16x8_t vec1, int16_t val2);         //  VQDMULH.S16 q0,q0,d0[0]
_NEON2SSE_INLINE int16x8_t vqdmulhq_n_s16(int16x8_t vec1, int16_t val2)         //  VQDMULH.S16 q0,q0,d0[0]
{         //solution may be not optimal
    int16x8_t scalar;
    scalar = vdupq_n_s16(val2);
    return vqdmulhq_s16(vec1, scalar);
}

int32x4_t vqdmulhq_n_s32(int32x4_t vec1, int32_t val2);         //  VQDMULH.S32 q0,q0,d0[0]
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int32x4_t vqdmulhq_n_s32(int32x4_t vec1, int32_t val2), _NEON2SSE_REASON_SLOW_UNEFFECTIVE)
{
    int32x4_t scalar;
    scalar = vdupq_n_s32(val2);
    return vqdmulhq_s32(vec1, scalar);
}

//***** Vector saturating doubling multiply high by scalar ****************

//******** Vector saturating rounding doubling multiply high with scalar ***

#if defined(USE_SSSE3)
int16x8_t vqrdmulhq_n_s16(int16x8_t vec1, int16_t val2);         // VQRDMULH.S16 q0,q0,d0[0]
_NEON2SSE_INLINE int16x8_t vqrdmulhq_n_s16(int16x8_t vec1, int16_t val2)         // VQRDMULH.S16 q0,q0,d0[0]
{         //solution may be not optimal
    int16x8_t scalar;
    scalar = vdupq_n_s16(val2);
    return vqrdmulhq_s16(vec1, scalar);
}
#endif

#if defined(USE_SSSE3)
int32x4_t vqrdmulhq_n_s32(int32x4_t vec1, int32_t val2);         // VQRDMULH.S32 q0,q0,d0[0]
_NEON2SSE_INLINE _NEON2SSE_PERFORMANCE_WARNING(int32x4_t vqrdmulhq_n_s32(int32x4_t vec1, int32_t val2), _NEON2SSE_REASON_SLOW_UNEFFECTIVE)
{
    int32x4_t scalar;
    scalar = vdupq_n_s32(val2);
    return vqrdmulhq_s32(vec1, scalar);
}
#endif

//********* Vector rounding saturating doubling multiply high by scalar  ****

//**************Vector multiply accumulate with scalar *******************

int16x8_t vmlaq_n_s16(int16x8_t a, int16x8_t b, int16_t c);         // VMLA.I16 q0, q0, d0[0]
_NEON2SSE_INLINE int16x8_t vmlaq_n_s16(int16x8_t a, int16x8_t b, int16_t c)         // VMLA.I16 q0, q0, d0[0]
{
    int16x8_t scalar;
    scalar = vdupq_n_s16(c);
    return vmlaq_s16(a,b,scalar);
}

int32x4_t vmlaq_n_s32(int32x4_t a, int32x4_t b, int32_t c);         // VMLA.I32 q0, q0, d0[0]
_NEON2SSE_INLINE int32x4_t vmlaq_n_s32(int32x4_t a, int32x4_t b, int32_t c)         // VMLA.I32 q0, q0, d0[0]
{
    int32x4_t scalar;
    scalar = vdupq_n_s32(c);
    return vmlaq_s32(a,b,scalar);
}

uint16x8_t vmlaq_n_u16(uint16x8_t a, uint16x8_t b, uint16_t c);         // VMLA.I16 q0, q0, d0[0]
#define vmlaq_n_u16 vmlaq_n_s16

uint32x4_t vmlaq_n_u32(uint32x4_t a, uint32x4_t b, uint32_t c);         // VMLA.I32 q0, q0, d0[0]
#define vmlaq_n_u32 vmlaq_n_s32

float32x4_t vmlaq_n_f32(float32x4_t a, float32x4_t b, float32_t c);         // VMLA.F32 q0, q0, d0[0]
_NEON2SSE_INLINE float32x4_t vmlaq_n_f32(float32x4_t a, float32x4_t b, float32_t c)         // VMLA.F32 q0, q0, d0[0]
{
    float32x4_t scalar;
    scalar = vdupq_n_f32(c);
    return vmlaq_f32(a,b,scalar);
}

//************Vector widening multiply accumulate with scalar****************************

//************ Vector widening saturating doubling multiply accumulate with scalar **************

//******** Vector multiply subtract with scalar **************

int16x8_t vmlsq_n_s16(int16x8_t a, int16x8_t b, int16_t c);         // VMLS.I16 q0, q0, d0[0]
_NEON2SSE_INLINE int16x8_t vmlsq_n_s16(int16x8_t a, int16x8_t b, int16_t c)         // VMLS.I16 q0, q0, d0[0]
{
    int16x8_t vc;
    vc = vdupq_n_s16(c);
    return vmlsq_s16(a, b,vc);
}

int32x4_t vmlsq_n_s32(int32x4_t a, int32x4_t b, int32_t c);         // VMLS.I32 q0, q0, d0[0]
_NEON2SSE_INLINE int32x4_t vmlsq_n_s32(int32x4_t a, int32x4_t b, int32_t c)         // VMLS.I32 q0, q0, d0[0]
{
    int32x4_t vc;
    vc = vdupq_n_s32(c);
    return vmlsq_s32(a,b,vc);
}

uint16x8_t vmlsq_n_u16(uint16x8_t a, uint16x8_t b, uint16_t c);         // VMLS.I16 q0, q0, d0[0]
_NEON2SSE_INLINE uint16x8_t vmlsq_n_u16(uint16x8_t a, uint16x8_t b, uint16_t c)         // VMLS.I16 q0, q0, d0[0]
{
    uint32x4_t vc;
    vc = vdupq_n_u32(c);
    return vmlsq_u32(a,b,vc);
}

uint32x4_t vmlsq_n_u32(uint32x4_t a, uint32x4_t b, uint32_t c);         // VMLS.I32 q0, q0, d0[0]
_NEON2SSE_INLINE uint32x4_t vmlsq_n_u32(uint32x4_t a, uint32x4_t b, uint32_t c)         // VMLS.I32 q0, q0, d0[0]
{
    uint32x4_t vc;
    vc = vdupq_n_u32(c);
    return vmlsq_u32(a,b,vc);
}

float32x4_t vmlsq_n_f32(float32x4_t a, float32x4_t b, float32_t c);         // VMLS.F32 q0, q0, d0[0]
_NEON2SSE_INLINE float32x4_t vmlsq_n_f32(float32x4_t a, float32x4_t b, float32_t c)
{
    float32x4_t vc;
    vc = vdupq_n_f32(c);
    return vmlsq_f32(a,b,vc);
}

//**** Vector widening multiply subtract with scalar ******

//***** Vector widening saturating doubling multiply subtract with scalar *********
//**********************************************************************************

//*******************  Vector extract ***********************************************
//*************************************************************************************
//VEXT (Vector Extract) extracts  elements from the bottom end of the second operand
//vector and the top end of the first, concatenates them, and places the result in the destination vector
//c elements from the bottom end of the second operand and (8-c) from the top end of the first

#if defined(USE_SSSE3)
//same result tested

#endif

#if defined(USE_SSSE3)
int8x16_t vextq_s8(int8x16_t a, int8x16_t b, __constrange(0,15) int c);         // VEXT.8 q0,q0,q0,#0
#define vextq_s8(a,b,c) _MM_ALIGNR_EPI8 (b,a,c)

uint8x16_t vextq_u8(uint8x16_t a, uint8x16_t b, __constrange(0,15) int c);         // VEXT.8 q0,q0,q0,#0
#define vextq_u8(a,b,c) _MM_ALIGNR_EPI8 (b,a,c)

poly8x16_t vextq_p8(poly8x16_t a, poly8x16_t b, __constrange(0,15) int c);         // VEXT.8 q0,q0,q0,#0
#define vextq_p8 vextq_s8

int16x8_t vextq_s16(int16x8_t a, int16x8_t b, __constrange(0,7) int c);         // VEXT.16 q0,q0,q0,#0
#define vextq_s16(a,b,c) _MM_ALIGNR_EPI8 (b,a,c * 2)

uint16x8_t vextq_u16(uint16x8_t a, uint16x8_t b, __constrange(0,7) int c);         // VEXT.16 q0,q0,q0,#0
#define vextq_u16(a,b,c) _MM_ALIGNR_EPI8 (b,a,c * 2)

poly16x8_t vextq_p16(poly16x8_t a, poly16x8_t b, __constrange(0,7) int c);         // VEXT.16 q0,q0,q0,#0
#define vextq_p16 vextq_s16
#endif

#if defined(USE_SSSE3)
int32x4_t vextq_s32(int32x4_t a, int32x4_t b, __constrange(0,3) int c);         // VEXT.32 q0,q0,q0,#0
#define vextq_s32(a,b,c) _MM_ALIGNR_EPI8 (b,a,c * 4)

uint32x4_t vextq_u32(uint32x4_t a, uint32x4_t b, __constrange(0,3) int c);         // VEXT.32 q0,q0,q0,#0
#define vextq_u32(a,b,c) _MM_ALIGNR_EPI8 (b,a,c * 4)

int64x2_t vextq_s64(int64x2_t a, int64x2_t b, __constrange(0,1) int c);         // VEXT.64 q0,q0,q0,#0
#define vextq_s64(a,b,c) _MM_ALIGNR_EPI8(b,a,c * 8)

uint64x2_t vextq_u64(uint64x2_t a, uint64x2_t b, __constrange(0,1) int c);         // VEXT.64 q0,q0,q0,#0
#define vextq_u64(a,b,c) _MM_ALIGNR_EPI8(b,a,c * 8)
#endif

//************ Reverse vector elements (swap endianness)*****************
//*************************************************************************
//VREVn.m reverses the order of the m-bit lanes within a set that is n bits wide.

#if defined(USE_SSSE3)
int8x16_t vrev64q_s8(int8x16_t vec);         // VREV64.8 q0,q0
_NEON2SSE_INLINE int8x16_t vrev64q_s8(int8x16_t vec)         // VREV64.8 q0,q0
{
    _NEON2SSE_ALIGN_16 int8_t mask_rev_e8[16] = {7,6,5,4,3,2,1,0, 15,14,13,12,11,10,9, 8};
    return _mm_shuffle_epi8 (vec, *(__m128i*)  mask_rev_e8);
}
#endif

#if defined(USE_SSSE3)
int16x8_t vrev64q_s16(int16x8_t vec);         // VREV64.16 q0,q0
_NEON2SSE_INLINE int16x8_t vrev64q_s16(int16x8_t vec)         // VREV64.16 q0,q0
{         //no _mm_shuffle_epi16, _mm_shuffle_epi8 to be used with the corresponding mask
    _NEON2SSE_ALIGN_16 int8_t mask_rev_e16[16] = {6,7, 4,5,2,3,0,1,14,15,12,13,10,11,8,9};
    return _mm_shuffle_epi8 (vec, *(__m128i*)mask_rev_e16);
}
#endif

int32x4_t vrev64q_s32(int32x4_t vec);         // VREV64.32 q0,q0
_NEON2SSE_INLINE int32x4_t vrev64q_s32(int32x4_t vec)         // VREV64.32 q0,q0
{
    return _mm_shuffle_epi32 (vec, 1 | (0 << 2) | (3 << 4) | (2 << 6) );
}

#if defined(USE_SSSE3)
uint8x16_t vrev64q_u8(uint8x16_t vec);         // VREV64.8 q0,q0
#define vrev64q_u8 vrev64q_s8

uint16x8_t vrev64q_u16(uint16x8_t vec);         // VREV64.16 q0,q0
#define vrev64q_u16 vrev64q_s16
#endif

uint32x4_t vrev64q_u32(uint32x4_t vec);         // VREV64.32 q0,q0
#define vrev64q_u32 vrev64q_s32

#if defined(USE_SSSE3)
poly8x16_t vrev64q_p8(poly8x16_t vec);         // VREV64.8 q0,q0
#define vrev64q_p8 vrev64q_u8

poly16x8_t vrev64q_p16(poly16x8_t vec);         // VREV64.16 q0,q0
#define vrev64q_p16 vrev64q_s16
#endif

float32x4_t vrev64q_f32(float32x4_t vec);         // VREV64.32 q0,q0
#define vrev64q_f32(vec) _mm_shuffle_ps (vec,  vec, _MM_SHUFFLE(2,3, 0,1))

//********************  32 bit shuffles **********************
//************************************************************

#if defined(USE_SSSE3)
int8x16_t vrev32q_s8(int8x16_t vec);         // VREV32.8 q0,q0
_NEON2SSE_INLINE int8x16_t vrev32q_s8(int8x16_t vec)         // VREV32.8 q0,q0
{
    _NEON2SSE_ALIGN_16 int8_t mask_rev_e8[16] = {3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12};
    return _mm_shuffle_epi8 (vec, *(__m128i*)  mask_rev_e8);
}
#endif

#if defined(USE_SSSE3)
int16x8_t vrev32q_s16(int16x8_t vec);         // VREV32.16 q0,q0
_NEON2SSE_INLINE int16x8_t vrev32q_s16(int16x8_t vec)         // VREV32.16 q0,q0
{
    _NEON2SSE_ALIGN_16 int8_t mask_rev_e8[16] = {2,3,0,1, 6,7, 4,5, 10,11, 8,9, 14,15,12,13};
    return _mm_shuffle_epi8 (vec, *(__m128i*)  mask_rev_e8);
}
#endif

#if defined(USE_SSSE3)
uint8x16_t vrev32q_u8(uint8x16_t vec);         // VREV32.8 q0,q0
#define vrev32q_u8 vrev32q_s8

uint16x8_t vrev32q_u16(uint16x8_t vec);         // VREV32.16 q0,q0
#define vrev32q_u16 vrev32q_s16

poly8x16_t vrev32q_p8(poly8x16_t vec);         // VREV32.8 q0,q0
#define vrev32q_p8 vrev32q_u8
#endif

//*************  16 bit shuffles **********************
//******************************************************

#if defined(USE_SSSE3)
int8x16_t vrev16q_s8(int8x16_t vec);         // VREV16.8 q0,q0
_NEON2SSE_INLINE int8x16_t vrev16q_s8(int8x16_t vec)         // VREV16.8 q0,q0
{
    _NEON2SSE_ALIGN_16 int8_t mask_rev8[16] = {1,0, 3,2, 5,4, 7,6, 9,8, 11, 10, 13, 12, 15, 14};
    return _mm_shuffle_epi8 (vec, *(__m128i*)  mask_rev8);
}
#endif

#if defined(USE_SSSE3)
uint8x16_t vrev16q_u8(uint8x16_t vec);         // VREV16.8 q0,q0
#define vrev16q_u8 vrev16q_s8

poly8x16_t vrev16q_p8(poly8x16_t vec);         // VREV16.8 q0,q0
#define vrev16q_p8 vrev16q_u8
#endif

//*********************************************************************
//**************** Other single operand arithmetic *******************
//*********************************************************************

//*********** Absolute: Vd[i] = |Va[i]| **********************************
//************************************************************************

int8x16_t   vabsq_s8(int8x16_t a);         // VABS.S8 q0,q0
#define vabsq_s8 _mm_abs_epi8

int16x8_t   vabsq_s16(int16x8_t a);         // VABS.S16 q0,q0
#define vabsq_s16 _mm_abs_epi16

int32x4_t   vabsq_s32(int32x4_t a);         // VABS.S32 q0,q0
#define vabsq_s32 _mm_abs_epi32

float32x4_t vabsq_f32(float32x4_t a);         // VABS.F32 q0,q0
_NEON2SSE_INLINE float32x4_t vabsq_f32(float32x4_t a)         // VABS.F32 q0,q0
{
    _NEON2SSE_ALIGN_16 int32_t c7fffffff[4] = {0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff};
    return _mm_and_ps (a, *(__m128*)c7fffffff);
}

//****** Saturating absolute: Vd[i] = sat(|Va[i]|) *********************
//**********************************************************************
//For signed-integer data types, the absolute value of the most negative value is not representable by the data type, saturation takes place

#if defined(USE_SSSE3)
int8x16_t vqabsq_s8(int8x16_t a);         // VQABS.S8 q0,q0
_NEON2SSE_INLINE int8x16_t vqabsq_s8(int8x16_t a)         // VQABS.S8 q0,q0
{
    __m128i c_128, abs, abs_cmp;
    c_128 = _mm_set1_epi8 (0x80);         //-128
    abs = _mm_abs_epi8 (a);
    abs_cmp = _mm_cmpeq_epi8 (abs, c_128);
    return _mm_xor_si128 (abs,  abs_cmp);
}
#endif

#if defined(USE_SSSE3)
int16x8_t vqabsq_s16(int16x8_t a);         // VQABS.S16 q0,q0
_NEON2SSE_INLINE int16x8_t vqabsq_s16(int16x8_t a)         // VQABS.S16 q0,q0
{
    __m128i c_32768, abs, abs_cmp;
    c_32768 = _mm_set1_epi16 (0x8000);         //-32768
    abs = _mm_abs_epi16 (a);
    abs_cmp = _mm_cmpeq_epi16 (abs, c_32768);
    return _mm_xor_si128 (abs,  abs_cmp);
}
#endif

#if defined(USE_SSSE3)
int32x4_t vqabsq_s32(int32x4_t a);         // VQABS.S32 q0,q0
_NEON2SSE_INLINE int32x4_t vqabsq_s32(int32x4_t a)         // VQABS.S32 q0,q0
{
    __m128i c80000000, abs, abs_cmp;
    c80000000 = _mm_set1_epi32 (0x80000000);         //most negative value
    abs = _mm_abs_epi32 (a);
    abs_cmp = _mm_cmpeq_epi32 (abs, c80000000);
    return _mm_xor_si128 (abs,  abs_cmp);
}
#endif

//*************** Negate: Vd[i] = - Va[i] *************************************
//*****************************************************************************
//several Negate implementations possible for SIMD.
//e.//function _mm_sign function(a, negative numbers vector), but the following one gives good performance:

int8x16_t vnegq_s8(int8x16_t a);         // VNE//q0,q0
_NEON2SSE_INLINE int8x16_t vnegq_s8(int8x16_t a)         // VNE//q0,q0
{
    __m128i zero;
    zero = _mm_setzero_si128 ();
    return _mm_sub_epi8 (zero, a);
}         //or _mm_sign_epi8 (a, negative numbers vector)

int16x8_t vnegq_s16(int16x8_t a);         // VNE//q0,q0
_NEON2SSE_INLINE int16x8_t vnegq_s16(int16x8_t a)         // VNE//q0,q0
{
    __m128i zero;
    zero = _mm_setzero_si128 ();
    return _mm_sub_epi16 (zero, a);
}         //or _mm_sign_epi16 (a, negative numbers vector)

int32x4_t vnegq_s32(int32x4_t a);         // VNE//q0,q0
_NEON2SSE_INLINE int32x4_t vnegq_s32(int32x4_t a)         // VNE//q0,q0
{
    __m128i zero;
    zero = _mm_setzero_si128 ();
    return _mm_sub_epi32 (zero, a);
}         //or _mm_sign_epi32 (a, negative numbers vector)

float32x4_t vnegq_f32(float32x4_t a);         // VNE//q0,q0
_NEON2SSE_INLINE float32x4_t vnegq_f32(float32x4_t a)         // VNE//q0,q0
{
    _NEON2SSE_ALIGN_16 int32_t c80000000[4] = {0x80000000, 0x80000000, 0x80000000, 0x80000000};
    return _mm_xor_ps (a, *(__m128*) c80000000);
}

//************** Saturating Negate: sat(Vd[i] = - Va[i]) **************************
//***************************************************************************************
//For signed-integer data types, the negation of the most negative value can't be produced without saturation, while with saturation it is max positive

int8x16_t vqnegq_s8(int8x16_t a);         // VQNE//q0,q0
_NEON2SSE_INLINE int8x16_t vqnegq_s8(int8x16_t a)         // VQNE//q0,q0
{
    __m128i zero;
    zero = _mm_setzero_si128 ();
    return _mm_subs_epi8 (zero, a);         //saturating substraction
}

int16x8_t vqnegq_s16(int16x8_t a);         // VQNE//q0,q0
_NEON2SSE_INLINE int16x8_t vqnegq_s16(int16x8_t a)         // VQNE//q0,q0
{
    __m128i zero;
    zero = _mm_setzero_si128 ();
    return _mm_subs_epi16 (zero, a);         //saturating substraction
}

int32x4_t vqnegq_s32(int32x4_t a);         // VQNE//q0,q0
_NEON2SSE_INLINE int32x4_t vqnegq_s32(int32x4_t a)         // VQNE//q0,q0
{         //solution may be not optimal compared with a serial
    __m128i c80000000, zero, sub, cmp;
    c80000000 = _mm_set1_epi32 (0x80000000);         //most negative value
    zero = _mm_setzero_si128 ();
    sub =  _mm_sub_epi32 (zero, a);         //substraction
    cmp = _mm_cmpeq_epi32 (a, c80000000);
    return _mm_xor_si128 (sub,  cmp);
}

//****************** Count leading zeros ********************************
//**************************************************************************
//no corresponding vector intrinsics in IA32, need to implement it.  While the implementation is effective for 8 bits, it may be not for 16 and 32 bits

#if defined(USE_SSSE3)
int8x16_t vclzq_s8(int8x16_t a);         // VCLZ.I8 q0,q0
_NEON2SSE_INLINE int8x16_t vclzq_s8(int8x16_t a)
{
    _NEON2SSE_ALIGN_16 int8_t mask_CLZ[16] = { /* 0 */ 4,/* 1 */ 3,/* 2 */ 2,/* 3 */ 2,
                                       /* 4 */ 1,/* 5 */ 1,/* 6 */ 1,/* 7 */ 1,
                                       /* 8 */ 0,/* 9 */ 0,/* a */ 0,/* b */ 0,
                                       /* c */ 0,/* d */ 0,/* e */ 0,/* f */ 0};
    __m128i maskLOW, c4, lowclz, mask, hiclz;
    maskLOW = _mm_set1_epi8(0x0f);         //low 4 bits, don't need masking low to avoid zero if MSB is set - it happens automatically
    c4 = _mm_set1_epi8(4);
    lowclz = _mm_shuffle_epi8( *(__m128i*)mask_CLZ, a);         //uses low 4 bits anyway
    mask =  _mm_srli_epi16(a, 4);         //get high 4 bits as low bits
    mask = _mm_and_si128(mask, maskLOW);         //low 4 bits, need masking to avoid zero if MSB is set
    hiclz = _mm_shuffle_epi8( *(__m128i*) mask_CLZ, mask);         //uses low 4 bits anyway
    mask = _mm_cmpeq_epi8(hiclz, c4);         // shows the need to add lowclz zeros
    lowclz = _mm_and_si128(lowclz,mask);
    return _mm_add_epi8(lowclz, hiclz);
}
#endif

#if defined(USE_SSSE3)
int16x8_t vclzq_s16(int16x8_t a);         // VCLZ.I16 q0,q0
_NEON2SSE_INLINE int16x8_t vclzq_s16(int16x8_t a)
{
    __m128i c7, res8x16, res8x16_swap;
    _NEON2SSE_ALIGN_16 int8_t mask8_sab[16] = { 1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14};
    _NEON2SSE_ALIGN_16 uint16_t mask8bit[8] = {0x00ff, 0x00ff, 0x00ff, 0x00ff,0x00ff, 0x00ff, 0x00ff, 0x00ff};
    c7 = _mm_srli_epi16(*(__m128i*)mask8bit, 5);         //7
    res8x16 = vclzq_s8(a);
    res8x16_swap = _mm_shuffle_epi8 (res8x16, *(__m128i*) mask8_sab);         //horisontal pairs swap
    res8x16 = _mm_and_si128(res8x16, *(__m128i*)mask8bit);         //lowclz
    res8x16_swap = _mm_and_si128(res8x16_swap, *(__m128i*)mask8bit);         //hiclz
    c7 = _mm_cmpgt_epi16(res8x16_swap, c7);         // shows the need to add lowclz zeros
    res8x16 = _mm_and_si128(res8x16, c7);         //lowclz
    return _mm_add_epi16(res8x16_swap, res8x16);
}
#endif

int32x4_t vclzq_s32(int32x4_t a);         // VCLZ.I32 q0,q0
_NEON2SSE_INLINE int32x4_t vclzq_s32(int32x4_t a)
{
    __m128i c55555555, c33333333, c0f0f0f0f, c3f, c32, tmp, tmp1, res;
    c55555555 = _mm_set1_epi32(0x55555555);
    c33333333 = _mm_set1_epi32(0x33333333);
    c0f0f0f0f = _mm_set1_epi32(0x0f0f0f0f);
    c3f = _mm_set1_epi32(0x3f);
    c32 = _mm_set1_epi32(32);
    tmp = _mm_srli_epi32(a, 1);
    res = _mm_or_si128(tmp, a);         //atmp[i] |= (atmp[i] >> 1);
    tmp = _mm_srli_epi32(res, 2);
    res = _mm_or_si128(tmp, res);         //atmp[i] |= (atmp[i] >> 2);
    tmp = _mm_srli_epi32(res, 4);
    res = _mm_or_si128(tmp, res);         //atmp[i] |= (atmp[i] >> 4);
    tmp = _mm_srli_epi32(res, 8);
    res = _mm_or_si128(tmp, res);         //atmp[i] |= (atmp[i] >> 8);
    tmp = _mm_srli_epi32(res, 16);
    res = _mm_or_si128(tmp, res);         //atmp[i] |= (atmp[i] >> 16);

    tmp = _mm_srli_epi32(res, 1);
    tmp = _mm_and_si128(tmp, c55555555);
    res = _mm_sub_epi32(res, tmp);         //atmp[i] -= ((atmp[i] >> 1) & 0x55555555);

    tmp = _mm_srli_epi32(res, 2);
    tmp = _mm_and_si128(tmp, c33333333);
    tmp1 = _mm_and_si128(res, c33333333);
    res = _mm_add_epi32(tmp, tmp1);         //atmp[i] = (((atmp[i] >> 2) & 0x33333333) + (atmp[i] & 0x33333333));

    tmp = _mm_srli_epi32(res, 4);
    tmp = _mm_add_epi32(tmp, res);
    res = _mm_and_si128(tmp, c0f0f0f0f);         //atmp[i] = (((atmp[i] >> 4) + atmp[i]) & 0x0f0f0f0f);

    tmp = _mm_srli_epi32(res, 8);
    res = _mm_add_epi32(tmp, res);         //atmp[i] += (atmp[i] >> 8);

    tmp = _mm_srli_epi32(res, 16);
    res = _mm_add_epi32(tmp, res);         //atmp[i] += (atmp[i] >> 16);

    res = _mm_and_si128(res, c3f);         //atmp[i] = atmp[i] & 0x0000003f;

    return _mm_sub_epi32(c32, res);         //res[i] = 32 - atmp[i];
}

#if defined(USE_SSSE3)
uint8x16_t vclzq_u8(uint8x16_t a);         // VCLZ.I8 q0,q0
#define vclzq_u8 vclzq_s8

uint16x8_t vclzq_u16(uint16x8_t a);         // VCLZ.I16 q0,q0
#define vclzq_u16 vclzq_s16
#endif

uint32x4_t vclzq_u32(uint32x4_t a);         // VCLZ.I32 q0,q0
#define vclzq_u32 vclzq_s32

//************** Count leading sign bits **************************
//********************************************************************
//VCLS (Vector Count Leading Sign bits) counts the number of consecutive bits following
// the topmost bit, that are the same as the topmost bit, in each element in a vector
//No corresponding vector intrinsics in IA32, need to implement it.
//While the implementation is effective for 8 bits, it may be not for 16 and 32 bits

#if defined(USE_SSSE3)
int8x16_t vclsq_s8(int8x16_t a);         // VCLS.S8 q0,q0
_NEON2SSE_INLINE int8x16_t vclsq_s8(int8x16_t a)
{
    __m128i cff, c80, c1, a_mask, a_neg, a_pos, a_comb;
    cff = _mm_cmpeq_epi8 (a,a);         //0xff
    c80 = _mm_set1_epi8(0x80);
    c1 = _mm_set1_epi8(1);
    a_mask = _mm_and_si128(a, c80);
    a_mask = _mm_cmpeq_epi8(a_mask, c80);         //0xff if negative input and 0 if positive
    a_neg = _mm_xor_si128(a, cff);
    a_neg = _mm_and_si128(a_mask, a_neg);
    a_pos = _mm_andnot_si128(a_mask, a);
    a_comb = _mm_or_si128(a_pos, a_neg);
    a_comb = vclzq_s8(a_comb);
    return _mm_sub_epi8(a_comb, c1);
}
#endif

#if defined(USE_SSSE3)
int16x8_t vclsq_s16(int16x8_t a);         // VCLS.S16 q0,q0
_NEON2SSE_INLINE int16x8_t vclsq_s16(int16x8_t a)
{
    __m128i cffff, c8000, c1, a_mask, a_neg, a_pos, a_comb;
    cffff = _mm_cmpeq_epi16(a,a);
    c8000 =  _mm_slli_epi16(cffff, 15);         //0x8000
    c1 = _mm_srli_epi16(cffff,15);         //0x1
    a_mask = _mm_and_si128(a, c8000);
    a_mask = _mm_cmpeq_epi16(a_mask, c8000);         //0xffff if negative input and 0 if positive
    a_neg = _mm_xor_si128(a, cffff);
    a_neg = _mm_and_si128(a_mask, a_neg);
    a_pos = _mm_andnot_si128(a_mask, a);
    a_comb = _mm_or_si128(a_pos, a_neg);
    a_comb = vclzq_s16(a_comb);
    return _mm_sub_epi16(a_comb, c1);
}
#endif

int32x4_t vclsq_s32(int32x4_t a);         // VCLS.S32 q0,q0
_NEON2SSE_INLINE int32x4_t vclsq_s32(int32x4_t a)
{
    __m128i cffffffff, c80000000, c1, a_mask, a_neg, a_pos, a_comb;
    cffffffff = _mm_cmpeq_epi32(a,a);
    c80000000 =  _mm_slli_epi32(cffffffff, 31);         //0x80000000
    c1 = _mm_srli_epi32(cffffffff,31);         //0x1
    a_mask = _mm_and_si128(a, c80000000);
    a_mask = _mm_cmpeq_epi32(a_mask, c80000000);         //0xffffffff if negative input and 0 if positive
    a_neg = _mm_xor_si128(a, cffffffff);
    a_neg = _mm_and_si128(a_mask, a_neg);
    a_pos = _mm_andnot_si128(a_mask, a);
    a_comb = _mm_or_si128(a_pos, a_neg);
    a_comb = vclzq_s32(a_comb);
    return _mm_sub_epi32(a_comb, c1);
}

//************************* Count number of set bits   ********************************
//*************************************************************************************
//No corresponding SIMD solution. One option is to get a elements, convert it to 32 bits and then use SSE4.2  _mm_popcnt__u32 (unsigned int v) for each element
//another option is to do the following algorithm:

#if defined(USE_SSSE3)
uint8x16_t vcntq_u8(uint8x16_t a);         // VCNT.8 q0,q0
_NEON2SSE_INLINE uint8x16_t vcntq_u8(uint8x16_t a)
{
    _NEON2SSE_ALIGN_16 int8_t mask_POPCOUNT[16] = { /* 0 */ 0,/* 1 */ 1,/* 2 */ 1,/* 3 */ 2,
                                            /* 4 */ 1,/* 5 */ 2,/* 6 */ 2,/* 7 */ 3,
                                            /* 8 */ 1,/* 9 */ 2,/* a */ 2,/* b */ 3,
                                            /* c */ 2,/* d */ 3,/* e */ 3,/* f */ 4};
    __m128i maskLOW, mask, lowpopcnt, hipopcnt;
    maskLOW = _mm_set1_epi8(0x0f);         //low 4 bits, need masking to avoid zero if MSB is set
    mask = _mm_and_si128(a, maskLOW);
    lowpopcnt = _mm_shuffle_epi8( *(__m128i*)mask_POPCOUNT, mask);         //uses low 4 bits anyway
    mask =  _mm_srli_epi16(a, 4);         //get high 4 bits as low bits
    mask = _mm_and_si128(mask, maskLOW);         //low 4 bits, need masking to avoid zero if MSB is set
    hipopcnt = _mm_shuffle_epi8( *(__m128i*) mask_POPCOUNT, mask);         //uses low 4 bits anyway
    return _mm_add_epi8(lowpopcnt, hipopcnt);
}
#endif

#if defined(USE_SSSE3)
int8x16_t vcntq_s8(int8x16_t a);         // VCNT.8 q0,q0
#define vcntq_s8 vcntq_u8

poly8x16_t vcntq_p8(poly8x16_t a);         // VCNT.8 q0,q0
#define vcntq_p8 vcntq_u8
#endif

//**************************************************************************************
//*********************** Logical operations ****************************************
//**************************************************************************************
//************************** Bitwise not ***********************************
//several Bitwise not implementations possible for SIMD. Eg "xor" with all ones, but the following one gives good performance

int8x16_t vmvnq_s8(int8x16_t a);         // VMVN q0,q0
_NEON2SSE_INLINE int8x16_t vmvnq_s8(int8x16_t a)         // VMVN q0,q0
{
    __m128i c1;
    c1 = _mm_cmpeq_epi8 (a,a);         //0xff
    return _mm_andnot_si128 (a, c1);
}

int16x8_t vmvnq_s16(int16x8_t a);         // VMVN q0,q0
_NEON2SSE_INLINE int16x8_t vmvnq_s16(int16x8_t a)         // VMVN q0,q0
{
    __m128i c1;
    c1 = _mm_cmpeq_epi16 (a,a);         //0xffff
    return _mm_andnot_si128 (a, c1);
}

int32x4_t vmvnq_s32(int32x4_t a);         // VMVN q0,q0
_NEON2SSE_INLINE int32x4_t vmvnq_s32(int32x4_t a)         // VMVN q0,q0
{
    __m128i c1;
    c1 = _mm_cmpeq_epi32 (a,a);         //0xffffffff
    return _mm_andnot_si128 (a, c1);
}

uint8x16_t vmvnq_u8(uint8x16_t a);         // VMVN q0,q0
#define vmvnq_u8 vmvnq_s8

uint16x8_t vmvnq_u16(uint16x8_t a);         // VMVN q0,q0
#define vmvnq_u16 vmvnq_s16

uint32x4_t vmvnq_u32(uint32x4_t a);         // VMVN q0,q0
#define vmvnq_u32 vmvnq_s32

poly8x16_t vmvnq_p8(poly8x16_t a);         // VMVN q0,q0
#define vmvnq_p8 vmvnq_u8

//****************** Bitwise and ***********************
//******************************************************

int8x16_t   vandq_s8(int8x16_t a, int8x16_t b);         // VAND q0,q0,q0
#define vandq_s8 _mm_and_si128

int16x8_t   vandq_s16(int16x8_t a, int16x8_t b);         // VAND q0,q0,q0
#define vandq_s16 _mm_and_si128

int32x4_t   vandq_s32(int32x4_t a, int32x4_t b);         // VAND q0,q0,q0
#define vandq_s32 _mm_and_si128

int64x2_t   vandq_s64(int64x2_t a, int64x2_t b);         // VAND q0,q0,q0
#define vandq_s64 _mm_and_si128

uint8x16_t   vandq_u8(uint8x16_t a, uint8x16_t b);         // VAND q0,q0,q0
#define vandq_u8 _mm_and_si128

uint16x8_t   vandq_u16(uint16x8_t a, uint16x8_t b);         // VAND q0,q0,q0
#define vandq_u16 _mm_and_si128

uint32x4_t   vandq_u32(uint32x4_t a, uint32x4_t b);         // VAND q0,q0,q0
#define vandq_u32 _mm_and_si128

uint64x2_t   vandq_u64(uint64x2_t a, uint64x2_t b);         // VAND q0,q0,q0
#define vandq_u64 _mm_and_si128

//******************** Bitwise or *********************************
//******************************************************************

int8x16_t   vorrq_s8(int8x16_t a, int8x16_t b);         // VORR q0,q0,q0
#define vorrq_s8 _mm_or_si128

int16x8_t   vorrq_s16(int16x8_t a, int16x8_t b);         // VORR q0,q0,q0
#define vorrq_s16 _mm_or_si128

int32x4_t   vorrq_s32(int32x4_t a, int32x4_t b);         // VORR q0,q0,q0
#define vorrq_s32 _mm_or_si128

int64x2_t   vorrq_s64(int64x2_t a, int64x2_t b);         // VORR q0,q0,q0
#define vorrq_s64 _mm_or_si128

uint8x16_t   vorrq_u8(uint8x16_t a, uint8x16_t b);         // VORR q0,q0,q0
#define vorrq_u8 _mm_or_si128

uint16x8_t   vorrq_u16(uint16x8_t a, uint16x8_t b);         // VORR q0,q0,q0
#define vorrq_u16 _mm_or_si128

uint32x4_t   vorrq_u32(uint32x4_t a, uint32x4_t b);         // VORR q0,q0,q0
#define vorrq_u32 _mm_or_si128

uint64x2_t   vorrq_u64(uint64x2_t a, uint64x2_t b);         // VORR q0,q0,q0
#define vorrq_u64 _mm_or_si128

//************* Bitwise exclusive or (EOR or XOR) ******************
//*******************************************************************

int8x16_t   veorq_s8(int8x16_t a, int8x16_t b);         // VEOR q0,q0,q0
#define veorq_s8 _mm_xor_si128

int16x8_t   veorq_s16(int16x8_t a, int16x8_t b);         // VEOR q0,q0,q0
#define veorq_s16 _mm_xor_si128

int32x4_t   veorq_s32(int32x4_t a, int32x4_t b);         // VEOR q0,q0,q0
#define veorq_s32 _mm_xor_si128

int64x2_t   veorq_s64(int64x2_t a, int64x2_t b);         // VEOR q0,q0,q0
#define veorq_s64 _mm_xor_si128

uint8x16_t   veorq_u8(uint8x16_t a, uint8x16_t b);         // VEOR q0,q0,q0
#define veorq_u8 _mm_xor_si128

uint16x8_t   veorq_u16(uint16x8_t a, uint16x8_t b);         // VEOR q0,q0,q0
#define veorq_u16 _mm_xor_si128

uint32x4_t   veorq_u32(uint32x4_t a, uint32x4_t b);         // VEOR q0,q0,q0
#define veorq_u32 _mm_xor_si128

uint64x2_t   veorq_u64(uint64x2_t a, uint64x2_t b);         // VEOR q0,q0,q0
#define veorq_u64 _mm_xor_si128

//********************** Bit Clear **********************************
//*******************************************************************
//Logical AND complement (AND negation or AND NOT)

//notice arguments "swap"

//notice arguments "swap"

//notice arguments "swap"

//notice arguments "swap"

//notice arguments "swap"

//notice arguments "swap"

//notice arguments "swap"

//notice arguments "swap"

int8x16_t   vbicq_s8(int8x16_t a, int8x16_t b);         // VBIC q0,q0,q0
#define vbicq_s8(a,b) _mm_andnot_si128 (b,a)         //notice arguments "swap"

int16x8_t   vbicq_s16(int16x8_t a, int16x8_t b);         // VBIC q0,q0,q0
#define vbicq_s16(a,b) _mm_andnot_si128 (b,a)         //notice arguments "swap"

int32x4_t   vbicq_s32(int32x4_t a, int32x4_t b);         // VBIC q0,q0,q0
#define vbicq_s32(a,b) _mm_andnot_si128 (b,a)         //notice arguments "swap"

int64x2_t   vbicq_s64(int64x2_t a, int64x2_t b);         // VBIC q0,q0,q0
#define vbicq_s64(a,b) _mm_andnot_si128 (b,a)         //notice arguments "swap"

uint8x16_t   vbicq_u8(uint8x16_t a, uint8x16_t b);         // VBIC q0,q0,q0
#define vbicq_u8(a,b) _mm_andnot_si128 (b,a)         //notice arguments "swap"

uint16x8_t   vbicq_u16(uint16x8_t a, uint16x8_t b);         // VBIC q0,q0,q0
#define vbicq_u16(a,b) _mm_andnot_si128 (b,a)         //notice arguments "swap"

uint32x4_t   vbicq_u32(uint32x4_t a, uint32x4_t b);         // VBIC q0,q0,q0
#define vbicq_u32(a,b) _mm_andnot_si128 (b,a)         //notice arguments "swap"

uint64x2_t   vbicq_u64(uint64x2_t a, uint64x2_t b);         // VBIC q0,q0,q0
#define vbicq_u64(a,b) _mm_andnot_si128 (b,a)         //notice arguments "swap"

//**************** Bitwise OR complement ********************************
//**************************************** ********************************
//no exact IA 32 match, need to implement it as following

int8x16_t vornq_s8(int8x16_t a, int8x16_t b);         // VORN q0,q0,q0
_NEON2SSE_INLINE int8x16_t vornq_s8(int8x16_t a, int8x16_t b)         // VORN q0,q0,q0
{
    __m128i b1;
    b1 = vmvnq_s8( b);         //bitwise not for b
    return _mm_or_si128 (a, b1);
}

int16x8_t vornq_s16(int16x8_t a, int16x8_t b);         // VORN q0,q0,q0
_NEON2SSE_INLINE int16x8_t vornq_s16(int16x8_t a, int16x8_t b)         // VORN q0,q0,q0
{
    __m128i b1;
    b1 = vmvnq_s16( b);         //bitwise not for b
    return _mm_or_si128 (a, b1);
}

int32x4_t vornq_s32(int32x4_t a, int32x4_t b);         // VORN q0,q0,q0
_NEON2SSE_INLINE int32x4_t vornq_s32(int32x4_t a, int32x4_t b)         // VORN q0,q0,q0
{
    __m128i b1;
    b1 = vmvnq_s32( b);         //bitwise not for b
    return _mm_or_si128 (a, b1);
}

int64x2_t vornq_s64(int64x2_t a, int64x2_t b);         // VORN q0,q0,q0
_NEON2SSE_INLINE int64x2_t vornq_s64(int64x2_t a, int64x2_t b)
{
    __m128i c1, b1;
    c1 = _mm_cmpeq_epi8 (a, a);         //all ones 0xfffffff...fffff
    b1 = _mm_andnot_si128 (b, c1);
    return _mm_or_si128 (a, b1);
}

uint8x16_t vornq_u8(uint8x16_t a, uint8x16_t b);         // VORN q0,q0,q0
_NEON2SSE_INLINE uint8x16_t vornq_u8(uint8x16_t a, uint8x16_t b)         // VORN q0,q0,q0
{
    __m128i b1;
    b1 = vmvnq_u8( b);         //bitwise not for b
    return _mm_or_si128 (a, b1);
}

uint16x8_t vornq_u16(uint16x8_t a, uint16x8_t b);         // VORN q0,q0,q0
_NEON2SSE_INLINE uint16x8_t vornq_u16(uint16x8_t a, uint16x8_t b)         // VORN q0,q0,q0
{
    __m128i b1;
    b1 = vmvnq_s16( b);         //bitwise not for b
    return _mm_or_si128 (a, b1);
}

uint32x4_t vornq_u32(uint32x4_t a, uint32x4_t b);         // VORN q0,q0,q0
_NEON2SSE_INLINE uint32x4_t vornq_u32(uint32x4_t a, uint32x4_t b)         // VORN q0,q0,q0
{
    __m128i b1;
    b1 = vmvnq_u32( b);         //bitwise not for b
    return _mm_or_si128 (a, b1);
}
uint64x2_t vornq_u64(uint64x2_t a, uint64x2_t b);         // VORN q0,q0,q0
#define vornq_u64 vornq_s64

//********************* Bitwise Select *****************************
//******************************************************************
//Note This intrinsic can compile to any of VBSL/VBIF/VBIT depending on register allocation.(?????????)

//VBSL (Bitwise Select) selects each bit for the destination from the first operand if the
//corresponding bit of the destination is 1, or from the second operand if the corresponding bit of the destination is 0.

//VBIF (Bitwise Insert if False) inserts each bit from the first operand into the destination
//if the corresponding bit of the second operand is 0, otherwise leaves the destination bit unchanged

//VBIT (Bitwise Insert if True) inserts each bit from the first operand into the destination
//if the corresponding bit of the second operand is 1, otherwise leaves the destination bit unchanged.

//VBSL only is implemented for SIMD

int8x16_t vbslq_s8(uint8x16_t a, int8x16_t b, int8x16_t c);         // VBSL q0,q0,q0
_NEON2SSE_INLINE int8x16_t vbslq_s8(uint8x16_t a, int8x16_t b, int8x16_t c)         // VBSL q0,q0,q0
{
    __m128i sel1, sel2;
    sel1 = _mm_and_si128   (a, b);
    sel2 = _mm_andnot_si128 (a, c);
    return _mm_or_si128 (sel1, sel2);
}

int16x8_t vbslq_s16(uint16x8_t a, int16x8_t b, int16x8_t c);         // VBSL q0,q0,q0
#define vbslq_s16 vbslq_s8

int32x4_t vbslq_s32(uint32x4_t a, int32x4_t b, int32x4_t c);         // VBSL q0,q0,q0
#define vbslq_s32 vbslq_s8

int64x2_t vbslq_s64(uint64x2_t a, int64x2_t b, int64x2_t c);         // VBSL q0,q0,q0
#define vbslq_s64 vbslq_s8

uint8x16_t vbslq_u8(uint8x16_t a, uint8x16_t b, uint8x16_t c);         // VBSL q0,q0,q0
#define vbslq_u8 vbslq_s8

uint16x8_t vbslq_u16(uint16x8_t a, uint16x8_t b, uint16x8_t c);         // VBSL q0,q0,q0
#define vbslq_u16 vbslq_s8

uint32x4_t vbslq_u32(uint32x4_t a, uint32x4_t b, uint32x4_t c);         // VBSL q0,q0,q0
#define vbslq_u32 vbslq_s8

uint64x2_t vbslq_u64(uint64x2_t a, uint64x2_t b, uint64x2_t c);         // VBSL q0,q0,q0
#define vbslq_u64 vbslq_s8

float32x4_t vbslq_f32(uint32x4_t a, float32x4_t b, float32x4_t c);         // VBSL q0,q0,q0
_NEON2SSE_INLINE float32x4_t vbslq_f32(uint32x4_t a, float32x4_t b, float32x4_t c)         // VBSL q0,q0,q0
{
    __m128 sel1, sel2;
    sel1 = _mm_and_ps   (*(__m128*)&a, b);
    sel2 = _mm_andnot_ps (*(__m128*)&a, c);
    return _mm_or_ps (sel1, sel2);
}

poly8x16_t vbslq_p8(uint8x16_t a, poly8x16_t b, poly8x16_t c);         // VBSL q0,q0,q0
#define vbslq_p8 vbslq_u8

poly16x8_t vbslq_p16(uint16x8_t a, poly16x8_t b, poly16x8_t c);         // VBSL q0,q0,q0
#define vbslq_p16 vbslq_s8

//************************************************************************************
//**************** Transposition operations ****************************************
//************************************************************************************
//*****************  Vector Transpose ************************************************
//************************************************************************************
//VTRN (Vector Transpose) treats the elements of its operand vectors as elements of 2 x 2 matrices, and transposes the matrices.
// making the result look as (a0, b0, a2, b2, a4, b4,....) (a1, b1, a3, b3, a5, b5,.....)

#if defined(USE_SSSE3)
//int8x16x2_t vtrnq_s8(int8x16_t a, int8x16_t b); // VTRN.8 q0,q0
_NEON2SSE_INLINE int8x16x2_t vtrnq_s8(int8x16_t a, int8x16_t b)         // VTRN.8 q0,q0
{
    int8x16x2_t r8x16;
    __m128i a_sh, b_sh;
    _NEON2SSE_ALIGN_16 int8_t mask8_even_odd[16] = { 0, 2, 4, 6, 8, 10, 12, 14, 1, 3,5, 7, 9, 11, 13, 15};
    a_sh = _mm_shuffle_epi8 (a, *(__m128i*)mask8_even_odd);         //a0, a2, a4, a6, a8, a10, a12, a14, a1, a3, a5, a7, a9, a11, a13, a15
    b_sh = _mm_shuffle_epi8 (b, *(__m128i*)mask8_even_odd);         //b0, b2, b4, b6, b8, b10, b12, b14, b1, b3, b5, b7, b9, b11, b13, b15

    r8x16.val[0] =  _mm_unpacklo_epi8(a_sh, b_sh);         //(a0, b0, a2, b2, a4, b4, a6, b6, a8,b8, a10,b10, a12,b12, a14,b14)
    r8x16.val[1] =  _mm_unpackhi_epi8(a_sh, b_sh);         // (a1, b1, a3, b3, a5, b5, a7, b7, a9,b9, a11,b11, a13,b13, a15,b15)
    return r8x16;
}
#endif

#if defined(USE_SSSE3)
int16x8x2_t vtrnq_s16(int16x8_t a, int16x8_t b);         // VTRN.16 q0,q0
_NEON2SSE_INLINE int16x8x2_t vtrnq_s16(int16x8_t a, int16x8_t b)         // VTRN.16 q0,q0
{
    int16x8x2_t v16x8;
    __m128i a_sh, b_sh;
    _NEON2SSE_ALIGN_16 int8_t mask16_even_odd[16] = { 0,1, 4,5, 8,9, 12,13, 2,3, 6,7, 10,11, 14,15};
    a_sh = _mm_shuffle_epi8 (a, *(__m128i*)mask16_even_odd);         //a0, a2, a4, a6,  a1, a3, a5, a7
    b_sh = _mm_shuffle_epi8 (b, *(__m128i*)mask16_even_odd);         //b0, b2, b4, b6,  b1, b3, b5, b7
    v16x8.val[0] = _mm_unpacklo_epi16(a_sh, b_sh);         //a0, b0, a2, b2, a4, b4, a6, b6
    v16x8.val[1] = _mm_unpackhi_epi16(a_sh, b_sh);         //a1, b1, a3, b3, a5, b5, a7, b7
    return v16x8;
}
#endif

int32x4x2_t vtrnq_s32(int32x4_t a, int32x4_t b);         // VTRN.32 q0,q0
_NEON2SSE_INLINE int32x4x2_t vtrnq_s32(int32x4_t a, int32x4_t b)         // VTRN.32 q0,q0
{         //may be not optimal solution compared with serial
    int32x4x2_t v32x4;
    __m128i a_sh, b_sh;
    a_sh = _mm_shuffle_epi32 (a, 216);         //a0, a2, a1, a3
    b_sh = _mm_shuffle_epi32 (b, 216);         //b0, b2, b1, b3

    v32x4.val[0] = _mm_unpacklo_epi32(a_sh, b_sh);         //a0, b0, a2, b2
    v32x4.val[1] = _mm_unpackhi_epi32(a_sh, b_sh);         //a1, b1, a3,  b3
    return v32x4;
}

#if defined(USE_SSSE3)
uint8x16x2_t vtrnq_u8(uint8x16_t a, uint8x16_t b);         // VTRN.8 q0,q0
#define vtrnq_u8 vtrnq_s8

uint16x8x2_t vtrnq_u16(uint16x8_t a, uint16x8_t b);         // VTRN.16 q0,q0
#define vtrnq_u16 vtrnq_s16
#endif

uint32x4x2_t vtrnq_u32(uint32x4_t a, uint32x4_t b);         // VTRN.32 q0,q0
#define vtrnq_u32 vtrnq_s32

float32x4x2_t vtrnq_f32(float32x4_t a, float32x4_t b);         // VTRN.32 q0,q0
_NEON2SSE_INLINE float32x4x2_t vtrnq_f32(float32x4_t a, float32x4_t b)         // VTRN.32 q0,q0
{         //may be not optimal solution compared with serial
    float32x4x2_t f32x4;
    __m128 a_sh, b_sh;
    a_sh = _mm_shuffle_ps (a, a, _MM_SHUFFLE(3,1, 2, 0));         //a0, a2, a1, a3, need to check endiness
    b_sh = _mm_shuffle_ps (b, b, _MM_SHUFFLE(3,1, 2, 0));         //b0, b2, b1, b3, need to check endiness

    f32x4.val[0] = _mm_unpacklo_ps(a_sh, b_sh);         //a0, b0, a2, b2
    f32x4.val[1] = _mm_unpackhi_ps(a_sh, b_sh);         //a1, b1, a3,  b3
    return f32x4;
}

#if defined(USE_SSSE3)
poly8x16x2_t vtrnq_p8(poly8x16_t a, poly8x16_t b);         // VTRN.8 q0,q0
#define vtrnq_p8 vtrnq_s8

poly16x8x2_t vtrnq_p16(poly16x8_t a, poly16x8_t b);         // VTRN.16 q0,q0
#define vtrnq_p16 vtrnq_s16
#endif

//***************** Interleave elements ***************************
//*****************************************************************
//output has (a0,b0,a1,b1, a2,b2,.....)

int8x16x2_t vzipq_s8(int8x16_t a, int8x16_t b);         // VZIP.8 q0,q0
_NEON2SSE_INLINE int8x16x2_t vzipq_s8(int8x16_t a, int8x16_t b)         // VZIP.8 q0,q0
{
    int8x16x2_t r8x16;
    r8x16.val[0] =  _mm_unpacklo_epi8(a, b);
    r8x16.val[1] =  _mm_unpackhi_epi8(a, b);
    return r8x16;
}

int16x8x2_t vzipq_s16(int16x8_t a, int16x8_t b);         // VZIP.16 q0,q0
_NEON2SSE_INLINE int16x8x2_t vzipq_s16(int16x8_t a, int16x8_t b)         // VZIP.16 q0,q0
{
    int16x8x2_t r16x8;
    r16x8.val[0] =  _mm_unpacklo_epi16(a, b);
    r16x8.val[1] =  _mm_unpackhi_epi16(a, b);
    return r16x8;
}

int32x4x2_t vzipq_s32(int32x4_t a, int32x4_t b);         // VZIP.32 q0,q0
_NEON2SSE_INLINE int32x4x2_t vzipq_s32(int32x4_t a, int32x4_t b)         // VZIP.32 q0,q0
{
    int32x4x2_t r32x4;
    r32x4.val[0] =  _mm_unpacklo_epi32(a, b);
    r32x4.val[1] =  _mm_unpackhi_epi32(a, b);
    return r32x4;
}

uint8x16x2_t vzipq_u8(uint8x16_t a, uint8x16_t b);         // VZIP.8 q0,q0
#define vzipq_u8 vzipq_s8

uint16x8x2_t vzipq_u16(uint16x8_t a, uint16x8_t b);         // VZIP.16 q0,q0
#define vzipq_u16 vzipq_s16

uint32x4x2_t vzipq_u32(uint32x4_t a, uint32x4_t b);         // VZIP.32 q0,q0
#define vzipq_u32 vzipq_s32

float32x4x2_t vzipq_f32(float32x4_t a, float32x4_t b);         // VZIP.32 q0,q0
_NEON2SSE_INLINE float32x4x2_t vzipq_f32(float32x4_t a, float32x4_t b)         // VZIP.32 q0,q0
{
    float32x4x2_t f32x4;
    f32x4.val[0] =   _mm_unpacklo_ps ( a,  b);
    f32x4.val[1] =   _mm_unpackhi_ps ( a,  b);
    return f32x4;
}

poly8x16x2_t vzipq_p8(poly8x16_t a, poly8x16_t b);         // VZIP.8 q0,q0
#define vzipq_p8 vzipq_u8

poly16x8x2_t vzipq_p16(poly16x8_t a, poly16x8_t b);         // VZIP.16 q0,q0
#define vzipq_p16 vzipq_u16

//*********************** De-Interleave elements *************************
//*************************************************************************
//As the result of these functions first val  contains (a0,a2,a4,....,b0,b2, b4,...) and the second val (a1,a3,a5,....b1,b3,b5...)
//no such functions in IA32 SIMD, shuffle is required

#if defined(USE_SSSE3)
int8x16x2_t vuzpq_s8(int8x16_t a, int8x16_t b);         // VUZP.8 q0,q0
_NEON2SSE_INLINE int8x16x2_t vuzpq_s8(int8x16_t a, int8x16_t b)         // VUZP.8 q0,q0
{
    int8x16x2_t v8x16;
    __m128i a_sh, b_sh;
    _NEON2SSE_ALIGN_16 int8_t mask8_even_odd[16] = { 0, 2, 4, 6, 8, 10, 12, 14, 1, 3,5, 7, 9, 11, 13, 15};
    a_sh = _mm_shuffle_epi8 (a, *(__m128i*)mask8_even_odd);         //a0, a2, a4, a6, a8, a10, a12, a14, a1, a3, a5, a7, a9, a11, a13, a15
    b_sh = _mm_shuffle_epi8 (b, *(__m128i*)mask8_even_odd);         //b0, b2, b4, b6, b8, b10, b12, b14, b1, b3, b5, b7, b9, b11, b13, b15
    //we need unpack64 to combine lower (upper) 64 bits from a with lower (upper) 64 bits from b
    v8x16.val[0] = _mm_unpacklo_epi64(a_sh, b_sh);         ///a0, a2, a4, a6, a8, a10, a12, a14,  b0, b2, b4, b6, b8, b10, b12, b14,
    v8x16.val[1] = _mm_unpackhi_epi64(a_sh, b_sh);         //a1, a3, a5, a7, a9, a11, a13, a15,  b1, b3, b5, b7, b9, b11, b13, b15
    return v8x16;
}
#endif

#if defined(USE_SSSE3)
int16x8x2_t vuzpq_s16(int16x8_t a, int16x8_t b);         // VUZP.16 q0,q0
_NEON2SSE_INLINE int16x8x2_t vuzpq_s16(int16x8_t a, int16x8_t b)         // VUZP.16 q0,q0
{
    int16x8x2_t v16x8;
    __m128i a_sh, b_sh;
    _NEON2SSE_ALIGN_16 int8_t mask16_even_odd[16] = { 0,1, 4,5, 8,9, 12,13, 2,3, 6,7, 10,11, 14,15};
    a_sh = _mm_shuffle_epi8 (a, *(__m128i*)mask16_even_odd);         //a0, a2, a4, a6,  a1, a3, a5, a7
    b_sh = _mm_shuffle_epi8 (b, *(__m128i*)mask16_even_odd);         //b0, b2, b4, b6,  b1, b3, b5, b7
    v16x8.val[0] = _mm_unpacklo_epi64(a_sh, b_sh);         //a0, a2, a4, a6, b0, b2, b4, b6
    v16x8.val[1] = _mm_unpackhi_epi64(a_sh, b_sh);         //a1, a3, a5, a7, b1, b3, b5, b7
    return v16x8;
}
#endif

int32x4x2_t vuzpq_s32(int32x4_t a, int32x4_t b);         // VUZP.32 q0,q0
_NEON2SSE_INLINE int32x4x2_t vuzpq_s32(int32x4_t a, int32x4_t b)         // VUZP.32 q0,q0
{         //may be not optimal solution compared with serial
    int32x4x2_t v32x4;
    __m128i a_sh, b_sh;
    a_sh = _mm_shuffle_epi32 (a, 216);         //a0, a2, a1, a3
    b_sh = _mm_shuffle_epi32 (b, 216);         //b0, b2, b1, b3

    v32x4.val[0] = _mm_unpacklo_epi64(a_sh, b_sh);         //a0, a2, b0, b2
    v32x4.val[1] = _mm_unpackhi_epi64(a_sh, b_sh);         //a1, a3, b1, b3
    return v32x4;
}

#if defined(USE_SSSE3)
uint8x16x2_t vuzpq_u8(uint8x16_t a, uint8x16_t b);         // VUZP.8 q0,q0
#define vuzpq_u8 vuzpq_s8

uint16x8x2_t vuzpq_u16(uint16x8_t a, uint16x8_t b);         // VUZP.16 q0,q0
#define vuzpq_u16 vuzpq_s16
#endif

uint32x4x2_t vuzpq_u32(uint32x4_t a, uint32x4_t b);         // VUZP.32 q0,q0
#define vuzpq_u32 vuzpq_s32

float32x4x2_t vuzpq_f32(float32x4_t a, float32x4_t b);         // VUZP.32 q0,q0
_NEON2SSE_INLINE float32x4x2_t vuzpq_f32(float32x4_t a, float32x4_t b)         // VUZP.32 q0,q0
{
    float32x4x2_t v32x4;
    v32x4.val[0] = _mm_shuffle_ps(a, b, _MM_SHUFFLE(2,0, 2, 0));         //a0, a2, b0, b2 , need to check endianess however
    v32x4.val[1] = _mm_shuffle_ps(a, b, _MM_SHUFFLE(3,1, 3, 1));         //a1, a3, b1, b3, need to check endianess however
    return v32x4;
}

#if defined(USE_SSSE3)
poly8x16x2_t vuzpq_p8(poly8x16_t a, poly8x16_t b);         // VUZP.8 q0,q0
#define vuzpq_p8 vuzpq_u8

poly16x8x2_t vuzpq_p16(poly16x8_t a, poly16x8_t b);         // VUZP.16 q0,q0
#define vuzpq_p16 vuzpq_u16
#endif

//##############################################################################################
//*********************** Reinterpret cast intrinsics.******************************************
//##############################################################################################
// Not a part of oficial NEON instruction set but available in gcc compiler *********************

poly8x16_t vreinterpretq_p8_u32 (uint32x4_t t);
#define vreinterpretq_p8_u32

poly8x16_t vreinterpretq_p8_u16 (uint16x8_t t);
#define vreinterpretq_p8_u16

poly8x16_t vreinterpretq_p8_u8 (uint8x16_t t);
#define vreinterpretq_p8_u8

poly8x16_t vreinterpretq_p8_s32 (int32x4_t t);
#define vreinterpretq_p8_s32

poly8x16_t vreinterpretq_p8_s16 (int16x8_t t);
#define vreinterpretq_p8_s16

poly8x16_t vreinterpretq_p8_s8 (int8x16_t t);
#define vreinterpretq_p8_s8

poly8x16_t vreinterpretq_p8_u64 (uint64x2_t t);
#define vreinterpretq_p8_u64

poly8x16_t vreinterpretq_p8_s64 (int64x2_t t);
#define vreinterpretq_p8_s64

poly8x16_t vreinterpretq_p8_f32 (float32x4_t t);
#define vreinterpretq_p8_f32(t) _M128i(t)

poly8x16_t vreinterpretq_p8_p16 (poly16x8_t t);
#define vreinterpretq_p8_p16

poly16x8_t vreinterpretq_p16_u32 (uint32x4_t t);
#define vreinterpretq_p16_u32

poly16x8_t vreinterpretq_p16_u16 (uint16x8_t t);
#define vreinterpretq_p16_u16

poly16x8_t vreinterpretq_p16_s32 (int32x4_t t);
#define vreinterpretq_p16_s32

poly16x8_t vreinterpretq_p16_s16 (int16x8_t t);
#define vreinterpretq_p16_s16

poly16x8_t vreinterpretq_p16_s8 (int8x16_t t);
#define vreinterpretq_p16_s8

poly16x8_t vreinterpretq_p16_u64 (uint64x2_t t);
#define vreinterpretq_p16_u64

poly16x8_t vreinterpretq_p16_s64 (int64x2_t t);
#define vreinterpretq_p16_s64

poly16x8_t vreinterpretq_p16_f32 (float32x4_t t);
#define vreinterpretq_p16_f32(t) _M128i(t)

poly16x8_t vreinterpretq_p16_p8 (poly8x16_t t);
#define vreinterpretq_p16_p8  vreinterpretq_s16_p8

//****  Integer to float  ******

float32x4_t vreinterpretq_f32_u32 (uint32x4_t t);
#define  vreinterpretq_f32_u32(t) *(__m128*)&(t)

float32x4_t vreinterpretq_f32_u16 (uint16x8_t t);
#define vreinterpretq_f32_u16 vreinterpretq_f32_u32

float32x4_t vreinterpretq_f32_u8 (uint8x16_t t);
#define vreinterpretq_f32_u8 vreinterpretq_f32_u32

float32x4_t vreinterpretq_f32_s32 (int32x4_t t);
#define vreinterpretq_f32_s32 vreinterpretq_f32_u32

float32x4_t vreinterpretq_f32_s16 (int16x8_t t);
#define vreinterpretq_f32_s16 vreinterpretq_f32_u32

float32x4_t vreinterpretq_f32_s8 (int8x16_t t);
#define vreinterpretq_f32_s8 vreinterpretq_f32_u32

float32x4_t vreinterpretq_f32_u64 (uint64x2_t t);
#define vreinterpretq_f32_u64 vreinterpretq_f32_u32

float32x4_t vreinterpretq_f32_s64 (int64x2_t t);
#define vreinterpretq_f32_s64 vreinterpretq_f32_u32

float32x4_t vreinterpretq_f32_p16 (poly16x8_t t);
#define vreinterpretq_f32_p16 vreinterpretq_f32_u32

float32x4_t vreinterpretq_f32_p8 (poly8x16_t t);
#define vreinterpretq_f32_p8 vreinterpretq_f32_u32

//*** Integer type conversions ******************
//no conversion necessary for the following functions because it is same data type

int64x2_t vreinterpretq_s64_u32 (uint32x4_t t);
#define vreinterpretq_s64_u32

int64x2_t vreinterpretq_s64_s16 (uint16x8_t t);
#define vreinterpretq_s64_s16

int64x2_t vreinterpretq_s64_u8 (uint8x16_t t);
#define vreinterpretq_s64_u8

int64x2_t vreinterpretq_s64_s32 (int32x4_t t);
#define vreinterpretq_s64_s32

int64x2_t vreinterpretq_s64_u16 (int16x8_t t);
#define vreinterpretq_s64_u16

int64x2_t vreinterpretq_s64_s8 (int8x16_t t);
#define vreinterpretq_s64_s8

int64x2_t vreinterpretq_s64_u64 (uint64x2_t t);
#define vreinterpretq_s64_u64

int64x2_t vreinterpretq_s64_f32 (float32x4_t t);
#define vreinterpretq_s64_f32(t) _M128i(t)

int64x2_t vreinterpretq_s64_p16 (poly16x8_t t);
#define vreinterpretq_s64_p16

int64x2_t vreinterpretq_s64_p8 (poly8x16_t t);
#define vreinterpretq_s64_p8

uint64x2_t vreinterpretq_u64_u32 (uint32x4_t t);
#define vreinterpretq_u64_u32

uint64x2_t vreinterpretq_u64_u16 (uint16x8_t t);
#define vreinterpretq_u64_u16

uint64x2_t vreinterpretq_u64_u8 (uint8x16_t t);
#define vreinterpretq_u64_u8

uint64x2_t vreinterpretq_u64_s32 (int32x4_t t);
#define vreinterpretq_u64_s32

uint64x2_t vreinterpretq_u64_s16 (int16x8_t t);
#define vreinterpretq_u64_s16

uint64x2_t vreinterpretq_u64_s8 (int8x16_t t);
#define vreinterpretq_u64_s8

uint64x2_t vreinterpretq_u64_s64 (int64x2_t t);
#define vreinterpretq_u64_s64

uint64x2_t vreinterpretq_u64_f32 (float32x4_t t);
#define vreinterpretq_u64_f32(t) _M128i(t)

uint64x2_t vreinterpretq_u64_p16 (poly16x8_t t);
#define vreinterpretq_u64_p16

uint64x2_t vreinterpretq_u64_p8 (poly8x16_t t);
#define vreinterpretq_u64_p8

int8x16_t vreinterpretq_s8_u32 (uint32x4_t t);
#define vreinterpretq_s8_u32

int8x16_t vreinterpretq_s8_u16 (uint16x8_t t);
#define vreinterpretq_s8_u16

int8x16_t vreinterpretq_s8_u8 (uint8x16_t t);
#define vreinterpretq_s8_u8

int8x16_t vreinterpretq_s8_s32 (int32x4_t t);
#define vreinterpretq_s8_s32

int8x16_t vreinterpretq_s8_s16 (int16x8_t t);
#define vreinterpretq_s8_s16

int8x16_t vreinterpretq_s8_u64 (uint64x2_t t);
#define vreinterpretq_s8_u64

int8x16_t vreinterpretq_s8_s64 (int64x2_t t);
#define vreinterpretq_s8_s64

int8x16_t vreinterpretq_s8_f32 (float32x4_t t);
#define vreinterpretq_s8_f32(t) _M128i(t)

int8x16_t vreinterpretq_s8_p16 (poly16x8_t t);
#define vreinterpretq_s8_p16

int8x16_t vreinterpretq_s8_p8 (poly8x16_t t);
#define vreinterpretq_s8_p8

int16x8_t vreinterpretq_s16_u32 (uint32x4_t t);
#define vreinterpretq_s16_u32

int16x8_t vreinterpretq_s16_u16 (uint16x8_t t);
#define vreinterpretq_s16_u16

int16x8_t vreinterpretq_s16_u8 (uint8x16_t t);
#define vreinterpretq_s16_u8

int16x8_t vreinterpretq_s16_s32 (int32x4_t t);
#define vreinterpretq_s16_s32

int16x8_t vreinterpretq_s16_s8 (int8x16_t t);
#define vreinterpretq_s16_s8

int16x8_t vreinterpretq_s16_u64 (uint64x2_t t);
#define vreinterpretq_s16_u64

int16x8_t vreinterpretq_s16_s64 (int64x2_t t);
#define vreinterpretq_s16_s64

int16x8_t vreinterpretq_s16_f32 (float32x4_t t);
#define vreinterpretq_s16_f32(t) _M128i(t)

int16x8_t vreinterpretq_s16_p16 (poly16x8_t t);
#define vreinterpretq_s16_p16

int16x8_t vreinterpretq_s16_p8 (poly8x16_t t);
#define vreinterpretq_s16_p8

int32x4_t vreinterpretq_s32_u32 (uint32x4_t t);
#define vreinterpretq_s32_u32

int32x4_t vreinterpretq_s32_u16 (uint16x8_t t);
#define vreinterpretq_s32_u16

int32x4_t vreinterpretq_s32_u8 (uint8x16_t t);
#define vreinterpretq_s32_u8

int32x4_t vreinterpretq_s32_s16 (int16x8_t t);
#define vreinterpretq_s32_s16

int32x4_t vreinterpretq_s32_s8 (int8x16_t t);
#define vreinterpretq_s32_s8

int32x4_t vreinterpretq_s32_u64 (uint64x2_t t);
#define vreinterpretq_s32_u64

int32x4_t vreinterpretq_s32_s64 (int64x2_t t);
#define vreinterpretq_s32_s64

int32x4_t vreinterpretq_s32_f32 (float32x4_t t);
#define vreinterpretq_s32_f32(t)  _mm_castps_si128(t)         //(*(__m128i*)&(t))

int32x4_t vreinterpretq_s32_p16 (poly16x8_t t);
#define vreinterpretq_s32_p16

int32x4_t vreinterpretq_s32_p8 (poly8x16_t t);
#define vreinterpretq_s32_p8

uint8x16_t vreinterpretq_u8_u32 (uint32x4_t t);
#define vreinterpretq_u8_u32

uint8x16_t vreinterpretq_u8_u16 (uint16x8_t t);
#define vreinterpretq_u8_u16

uint8x16_t vreinterpretq_u8_s32 (int32x4_t t);
#define vreinterpretq_u8_s32

uint8x16_t vreinterpretq_u8_s16 (int16x8_t t);
#define vreinterpretq_u8_s16

uint8x16_t vreinterpretq_u8_s8 (int8x16_t t);
#define vreinterpretq_u8_s8

uint8x16_t vreinterpretq_u8_u64 (uint64x2_t t);
#define vreinterpretq_u8_u64

uint8x16_t vreinterpretq_u8_s64 (int64x2_t t);
#define vreinterpretq_u8_s64

uint8x16_t vreinterpretq_u8_f32 (float32x4_t t);
#define vreinterpretq_u8_f32(t) _M128i(t)

uint8x16_t vreinterpretq_u8_p16 (poly16x8_t t);
#define vreinterpretq_u8_p16

uint8x16_t vreinterpretq_u8_p8 (poly8x16_t t);
#define vreinterpretq_u8_p8

uint16x8_t vreinterpretq_u16_u32 (uint32x4_t t);
#define vreinterpretq_u16_u32

uint16x8_t vreinterpretq_u16_u8 (uint8x16_t t);
#define vreinterpretq_u16_u8

uint16x8_t vreinterpretq_u16_s32 (int32x4_t t);
#define vreinterpretq_u16_s32

uint16x8_t vreinterpretq_u16_s16 (int16x8_t t);
#define vreinterpretq_u16_s16

uint16x8_t vreinterpretq_u16_s8 (int8x16_t t);
#define vreinterpretq_u16_s8

uint16x8_t vreinterpretq_u16_u64 (uint64x2_t t);
#define vreinterpretq_u16_u64

uint16x8_t vreinterpretq_u16_s64 (int64x2_t t);
#define vreinterpretq_u16_s64

uint16x8_t vreinterpretq_u16_f32 (float32x4_t t);
#define vreinterpretq_u16_f32(t) _M128i(t)

uint16x8_t vreinterpretq_u16_p16 (poly16x8_t t);
#define vreinterpretq_u16_p16

uint16x8_t vreinterpretq_u16_p8 (poly8x16_t t);
#define vreinterpretq_u16_p8

uint32x4_t vreinterpretq_u32_u16 (uint16x8_t t);
#define vreinterpretq_u32_u16

uint32x4_t vreinterpretq_u32_u8 (uint8x16_t t);
#define vreinterpretq_u32_u8

uint32x4_t vreinterpretq_u32_s32 (int32x4_t t);
#define vreinterpretq_u32_s32

uint32x4_t vreinterpretq_u32_s16 (int16x8_t t);
#define vreinterpretq_u32_s16

uint32x4_t vreinterpretq_u32_s8 (int8x16_t t);
#define vreinterpretq_u32_s8

uint32x4_t vreinterpretq_u32_u64 (uint64x2_t t);
#define vreinterpretq_u32_u64

uint32x4_t vreinterpretq_u32_s64 (int64x2_t t);
#define vreinterpretq_u32_s64

uint32x4_t vreinterpretq_u32_f32 (float32x4_t t);
#define  vreinterpretq_u32_f32(t) _M128i(t)

uint32x4_t vreinterpretq_u32_p16 (poly16x8_t t);
#define vreinterpretq_u32_p16

uint32x4_t vreinterpretq_u32_p8 (poly8x16_t t);
#define vreinterpretq_u32_p8

#endif /* NEON2SSE_H */
