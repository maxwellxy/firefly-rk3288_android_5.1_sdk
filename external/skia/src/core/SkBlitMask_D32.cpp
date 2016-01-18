/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkBlitMask.h"
#include "SkColor.h"
#include "SkColorPriv.h"

#if defined(__ARM_HAVE_NEON) && defined(SK_CPU_LENDIAN)
    #define SK_USE_NEON
    #include <arm_neon.h>
#endif

static void D32_A8_Color(void* SK_RESTRICT dst, size_t dstRB,
                         const void* SK_RESTRICT maskPtr, size_t maskRB,
                         SkColor color, int width, int height) {
    SkPMColor pmc = SkPreMultiplyColor(color);
    size_t dstOffset = dstRB - (width << 2);
    size_t maskOffset = maskRB - width;
    SkPMColor* SK_RESTRICT device = (SkPMColor *)dst;
    const uint8_t* SK_RESTRICT mask = (const uint8_t*)maskPtr;

    do {
        int w = width;
        do {
            unsigned aa = *mask++;
            *device = SkBlendARGB32(pmc, *device, aa);
            device += 1;
        } while (--w != 0);
        device = (uint32_t*)((char*)device + dstOffset);
        mask += maskOffset;
    } while (--height != 0);
}

static void D32_A8_Opaque(void* SK_RESTRICT dst, size_t dstRB,
                          const void* SK_RESTRICT maskPtr, size_t maskRB,
                          SkColor color, int width, int height) {
    SkPMColor pmc = SkPreMultiplyColor(color);
    SkPMColor* SK_RESTRICT device = (SkPMColor*)dst;
    const uint8_t* SK_RESTRICT mask = (const uint8_t*)maskPtr;

    maskRB -= width;
    dstRB -= (width << 2);

#ifdef SK_USE_NEON
#define UNROLL 8
#define UNROLL_BY_2 4
#define UNROLL_BY_4 2
    do {
        int w = width;
        int count = 0;
        //cache line is 64 bytes
        __builtin_prefetch(mask);
        __builtin_prefetch(device);

        if( w>= UNROLL){
            /*mask or alpha*/
            uint16x8_t alpha_full;
            uint32x4_t alpha_lo, alpha_hi;
            uint32x4_t alpha_slo, alpha_shi; /*Saturated and scaled */
            uint32x4_t rb,ag;    /*For device premultiply */
            uint32x4_t rbp,agp;  /*For pmc premultiply */
            uint32x4_t dev_lo, dev_hi;
            uint32x4_t pmc_dup = vdupq_n_u32(pmc);
            uint32x4_t pmc_lo, pmc_hi;

            do{
                if( (int) count % 64 == 0){
                   __builtin_prefetch(mask);
                }
                if( (int) count % 16 == 0){
                   __builtin_prefetch(device);
                }

                alpha_full = vmovl_u8(vld1_u8(mask));
                alpha_lo = vmovl_u16(vget_low_u16(alpha_full));
                alpha_hi = vmovl_u16(vget_high_u16(alpha_full));

                dev_lo = vld1q_u32(device);
                dev_hi = vld1q_u32(device + 4);

                /*SkAlpha255To256(255-aa)*/
                //alpha_slo = vaddq_u32(vsubq_u32( vdupq_n_u32(0x000000FF), alpha_lo), vdupq_n_u32(0x00000001));
                alpha_slo = vsubq_u32( vdupq_n_u32(0x00000100), alpha_lo);
                //alpha_shi = vaddq_u32(vsubq_u32( vdupq_n_u32(0x000000FF), alpha_hi), vdupq_n_u32(0x00000001));
                alpha_shi = vsubq_u32( vdupq_n_u32(0x00000100), alpha_hi);
                /*SkAlpha255To256(aa)*/
                alpha_lo = vaddq_u32(alpha_lo, vdupq_n_u32(0x00000001));
                alpha_hi = vaddq_u32(alpha_hi, vdupq_n_u32(0x00000001));

                rb = vshrq_n_u32( vmulq_u32( vandq_u32( dev_lo, vdupq_n_u32(0x00FF00FF)), alpha_slo), 8);
                ag = vmulq_u32( vandq_u32( vshrq_n_u32(dev_lo, 8), vdupq_n_u32(0x00FF00FF)), alpha_slo);
                dev_lo = vorrq_u32( vandq_u32(rb,  vdupq_n_u32(0x00FF00FF)), vandq_u32(ag, vdupq_n_u32(0xFF00FF00)));
                rbp = vshrq_n_u32( vmulq_u32( vandq_u32( pmc_dup, vdupq_n_u32(0x00FF00FF)), alpha_lo), 8);
                agp = vmulq_u32( vandq_u32( vshrq_n_u32(pmc_dup, 8), vdupq_n_u32(0x00FF00FF)), alpha_lo);
                pmc_lo = vorrq_u32( vandq_u32(rbp,  vdupq_n_u32(0x00FF00FF)), vandq_u32(agp, vdupq_n_u32(0xFF00FF00)));

                dev_lo = vaddq_u32 ( pmc_lo, dev_lo);

                rb = vshrq_n_u32( vmulq_u32( vandq_u32( dev_hi, vdupq_n_u32(0x00FF00FF)), alpha_shi), 8);
                ag = vmulq_u32( vandq_u32( vshrq_n_u32(dev_hi, 8), vdupq_n_u32(0x00FF00FF)), alpha_shi);
                dev_hi = vorrq_u32( vandq_u32(rb,	vdupq_n_u32(0x00FF00FF)), vandq_u32(ag, vdupq_n_u32(0xFF00FF00)));
                rbp = vshrq_n_u32( vmulq_u32( vandq_u32( pmc_dup, vdupq_n_u32(0x00FF00FF)), alpha_hi), 8);
                agp = vmulq_u32( vandq_u32( vshrq_n_u32(pmc_dup, 8), vdupq_n_u32(0x00FF00FF)), alpha_hi);
                pmc_hi = vorrq_u32( vandq_u32(rbp,  vdupq_n_u32(0x00FF00FF)), vandq_u32(agp, vdupq_n_u32(0xFF00FF00)));

                dev_hi = vaddq_u32 ( pmc_hi, dev_hi);

                vst1q_u32(device, dev_lo);
                vst1q_u32(device + 4, dev_hi);

                device += UNROLL;
                mask += UNROLL;
                count += UNROLL;
                w -= UNROLL;
               }while (w >= UNROLL);
            }            else if(w >= UNROLL_BY_2){
               /*mask or alpha*/
               uint16x8_t alpha_full = vmovl_u8(vld1_u8(mask));
               uint32x4_t alpha_lo;
               uint32x4_t alpha_slo; /*Saturated and scaled */
               uint32x4_t rb,ag;     /*For device premultiply */
               uint32x4_t rbp,agp;   /*For pmc premultiply */
               uint32x4_t dev_lo;
               uint32x4_t pmc_lo;
               uint32x4_t pmc_dup = vdupq_n_u32(pmc);

               dev_lo = vld1q_u32(device);
               alpha_lo = vmovl_u16(vget_low_u16(alpha_full));

               /*SkAlpha255To256(255-aa)*/
               //alpha_slo = vaddq_u32(vsubq_u32( vdupq_n_u32(0x000000FF), alpha_lo), vdupq_n_u32(0x00000001));
               alpha_slo = vsubq_u32( vdupq_n_u32(0x00000100), alpha_lo);
               /*SkAlpha255To256(aa)*/
               alpha_lo = vaddq_u32(alpha_lo, vdupq_n_u32(0x00000001));

               rb = vshrq_n_u32( vmulq_u32( vandq_u32( dev_lo, vdupq_n_u32(0x00FF00FF)), alpha_slo), 8);
               ag = vmulq_u32( vandq_u32( vshrq_n_u32(dev_lo, 8), vdupq_n_u32(0x00FF00FF)), alpha_slo);
               dev_lo = vorrq_u32( vandq_u32(rb,  vdupq_n_u32(0x00FF00FF)), vandq_u32(ag, vdupq_n_u32(0xFF00FF00)));
               rbp = vshrq_n_u32( vmulq_u32( vandq_u32( pmc_dup, vdupq_n_u32(0x00FF00FF)), alpha_lo), 8);
               agp = vmulq_u32( vandq_u32( vshrq_n_u32(pmc_dup, 8), vdupq_n_u32(0x00FF00FF)), alpha_lo);
               pmc_lo = vorrq_u32( vandq_u32(rbp,  vdupq_n_u32(0x00FF00FF)), vandq_u32(agp, vdupq_n_u32(0xFF00FF00)));

               dev_lo = vaddq_u32 ( pmc_lo, dev_lo);

               vst1q_u32(device, dev_lo);

               device += UNROLL_BY_2;
               mask += UNROLL_BY_2;
               w -= UNROLL_BY_2;

               if (w >= UNROLL_BY_4){
                   /*mask or alpha*/
                   uint32x2_t alpha_lo;
                   uint32x2_t alpha_slo; /*Saturated and scaled */
                   uint32x2_t rb,ag;
                   uint32x2_t rbp,agp;   /*For pmc premultiply */
                   uint32x2_t dev_lo;
                   uint32x2_t pmc_lo;
                   uint32x2_t pmc_dup = vdup_n_u32(pmc);

                   dev_lo = vld1_u32(device);
                   alpha_lo = vget_low_u32(vmovl_u16(vget_high_u16(alpha_full)));

                   /*SkAlpha255To256(255-aa)*/
                   //alpha_slo = vadd_u32(vsub_u32( vdup_n_u32(0x000000FF), alpha_lo), vdup_n_u32(0x00000001));
                   alpha_slo = vsub_u32( vdup_n_u32(0x00000100), alpha_lo);
                   /*SkAlpha255To256(aa)*/
                   alpha_lo = vadd_u32(alpha_lo, vdup_n_u32(0x00000001));

                   rb = vshr_n_u32( vmul_u32( vand_u32( dev_lo, vdup_n_u32(0x00FF00FF)), alpha_slo), 8);
                   ag = vmul_u32( vand_u32( vshr_n_u32(dev_lo, 8), vdup_n_u32(0x00FF00FF)), alpha_slo);
                   dev_lo = vorr_u32( vand_u32(rb,  vdup_n_u32(0x00FF00FF)), vand_u32(ag, vdup_n_u32(0xFF00FF00)));
                   rbp = vshr_n_u32( vmul_u32( vand_u32( pmc_dup, vdup_n_u32(0x00FF00FF)), alpha_lo), 8);
                   agp = vmul_u32( vand_u32( vshr_n_u32(pmc_dup, 8), vdup_n_u32(0x00FF00FF)), alpha_lo);
                   pmc_lo = vorr_u32( vand_u32(rbp,  vdup_n_u32(0x00FF00FF)), vand_u32(agp, vdup_n_u32(0xFF00FF00)));

                   dev_lo = vadd_u32 ( pmc_lo, dev_lo);

                   vst1_u32(device, dev_lo);

                   device += UNROLL_BY_4;
                   mask += UNROLL_BY_4;
                   w -= UNROLL_BY_4;
               }

        } else if(w >= UNROLL_BY_4){
                 /*mask or alpha*/
                uint16x8_t alpha_full = vmovl_u8(vld1_u8(mask));
                uint32x2_t alpha_lo;
                uint32x2_t alpha_slo; /*Saturated and scaled */
                uint32x2_t rb,ag;
                uint32x2_t rbp,agp;   /*For pmc premultiply */
                uint32x2_t dev_lo;
                uint32x2_t pmc_lo;
                uint32x2_t pmc_dup = vdup_n_u32(pmc);

                dev_lo = vld1_u32(device);
                alpha_lo = vget_low_u32(vmovl_u16(vget_low_u16(alpha_full)));

                /*SkAlpha255To256(255-aa)*/
                //alpha_slo = vadd_u32(vsub_u32( vdup_n_u32(0x000000FF), alpha_lo), vdup_n_u32(0x00000001));
                alpha_slo = vsub_u32( vdup_n_u32(0x00000100), alpha_lo);
                /*SkAlpha255To256(aa)*/
                alpha_lo = vadd_u32(alpha_lo, vdup_n_u32(0x00000001));

                rb = vshr_n_u32( vmul_u32( vand_u32( dev_lo, vdup_n_u32(0x00FF00FF)), alpha_slo), 8);
                ag = vmul_u32( vand_u32( vshr_n_u32(dev_lo, 8), vdup_n_u32(0x00FF00FF)), alpha_slo);
                dev_lo = vorr_u32( vand_u32(rb,  vdup_n_u32(0x00FF00FF)), vand_u32(ag, vdup_n_u32(0xFF00FF00)));
                rbp = vshr_n_u32( vmul_u32( vand_u32( pmc_dup, vdup_n_u32(0x00FF00FF)), alpha_lo), 8);
                agp = vmul_u32( vand_u32( vshr_n_u32(pmc_dup, 8), vdup_n_u32(0x00FF00FF)), alpha_lo);
                pmc_lo = vorr_u32( vand_u32(rbp,  vdup_n_u32(0x00FF00FF)), vand_u32(agp, vdup_n_u32(0xFF00FF00)));

                dev_lo = vadd_u32 ( pmc_lo, dev_lo);

                vst1_u32(device, dev_lo);

                device += UNROLL_BY_4;
                mask += UNROLL_BY_4;
                w -= UNROLL_BY_4;
        }

        /*residuals (which is everything that cannot be handled by neon) */
        while( w > 0){
            unsigned aa = *mask++;
            if( (aa != 0) || (aa != 255)){
                *device = SkAlphaMulQ(pmc, SkAlpha255To256(aa)) + SkAlphaMulQ(*device, SkAlpha255To256(255 - aa));
            }
            device += 1;
            --w;
        }
        device = (uint32_t*)((char*)device + dstRB);
        mask += maskRB;
    } while (--height != 0);
#else
    do{
        int w = width;
        do {
            unsigned aa = *mask++;
            *device = SkAlphaMulQ(pmc, SkAlpha255To256(aa)) + SkAlphaMulQ(*device, SkAlpha255To256(255 - aa));
            device += 1;
        } while (--w != 0);
        device = (uint32_t*)((char*)device + dstRB);
        mask += maskRB;
    } while (--height != 0);
#endif
}

static void D32_A8_Black(void* SK_RESTRICT dst, size_t dstRB,
                         const void* SK_RESTRICT maskPtr, size_t maskRB,
                         SkColor, int width, int height) {
    SkPMColor* SK_RESTRICT device = (SkPMColor*)dst;
    const uint8_t* SK_RESTRICT mask = (const uint8_t*)maskPtr;

    maskRB -= width;
    dstRB -= (width << 2);

#ifdef SK_USE_NEON
#define UNROLL 8
#define UNROLL_BY_2 4
#define UNROLL_BY_4 2
    do {
        int w = width;
        int count = 0;
        //cache line is 64 bytes
        __builtin_prefetch(mask);
        __builtin_prefetch(device);

        if( w>= UNROLL){
            /*mask or alpha*/
            uint16x8_t alpha_full;
            uint32x4_t alpha_lo, alpha_hi;
            uint32x4_t alpha_slo, alpha_shi; /*Saturated and scaled */
            uint32x4_t rb,ag;
            uint32x4_t dev_lo, dev_hi;

            do{
                if( (int) count % 64 == 0){
                   __builtin_prefetch(mask);
                }
                if( (int) count % 16 == 0){
                   __builtin_prefetch(device);
                }

                alpha_full = vmovl_u8(vld1_u8(mask));
                alpha_lo = vmovl_u16(vget_low_u16(alpha_full));
                alpha_hi = vmovl_u16(vget_high_u16(alpha_full));

                dev_lo = vld1q_u32(device);
                dev_hi = vld1q_u32(device + 4);

                /*SkAlpha255To256(255-aa)*/
                //alpha_slo = vaddq_u32(vsubq_u32( vdupq_n_u32(0x000000FF), alpha_lo), vdupq_n_u32(0x00000001));
                alpha_slo = vsubq_u32( vdupq_n_u32(0x00000100), alpha_lo);
                //alpha_shi = vaddq_u32(vsubq_u32( vdupq_n_u32(0x000000FF), alpha_hi), vdupq_n_u32(0x00000001));
                alpha_shi = vsubq_u32( vdupq_n_u32(0x00000100), alpha_hi);

                rb = vshrq_n_u32( vmulq_u32( vandq_u32( dev_lo, vdupq_n_u32(0x00FF00FF)), alpha_slo), 8);
                ag = vmulq_u32( vandq_u32( vshrq_n_u32(dev_lo, 8), vdupq_n_u32(0x00FF00FF)), alpha_slo);
                dev_lo = vorrq_u32( vandq_u32(rb,  vdupq_n_u32(0x00FF00FF)), vandq_u32(ag, vdupq_n_u32(0xFF00FF00)));
                dev_lo = vaddq_u32 ( vshlq_n_u32(alpha_lo, SK_A32_SHIFT), dev_lo);

                rb = vshrq_n_u32( vmulq_u32( vandq_u32( dev_hi, vdupq_n_u32(0x00FF00FF)), alpha_shi), 8);
                ag = vmulq_u32( vandq_u32( vshrq_n_u32(dev_hi, 8), vdupq_n_u32(0x00FF00FF)), alpha_shi);
                dev_hi = vorrq_u32( vandq_u32(rb,	vdupq_n_u32(0x00FF00FF)), vandq_u32(ag, vdupq_n_u32(0xFF00FF00)));
                dev_hi = vaddq_u32( vshlq_n_u32(alpha_hi, SK_A32_SHIFT), dev_hi);

                vst1q_u32(device, dev_lo);
                vst1q_u32(device + 4, dev_hi);

                device += UNROLL;
                mask += UNROLL;
                count += UNROLL;
                w -= UNROLL;
               }while (w >= UNROLL);
            }            else if(w >= UNROLL_BY_2){
               /*mask or alpha*/
               uint16x8_t alpha_full = vmovl_u8(vld1_u8(mask));
               uint32x4_t alpha_lo;
               uint32x4_t alpha_slo; /*Saturated and scaled */
               uint32x4_t rb,ag;
               uint32x4_t dev_lo;

               dev_lo = vld1q_u32(device);
               alpha_lo = vmovl_u16(vget_low_u16(alpha_full));

               /*SkAlpha255To256(255-aa)*/
               //alpha_slo = vaddq_u32(vsubq_u32( vdupq_n_u32(0x000000FF), alpha_lo), vdupq_n_u32(0x00000001));
               alpha_slo = vsubq_u32( vdupq_n_u32(0x00000100), alpha_lo);

               rb = vshrq_n_u32( vmulq_u32( vandq_u32( dev_lo, vdupq_n_u32(0x00FF00FF)), alpha_slo), 8);
               ag = vmulq_u32( vandq_u32( vshrq_n_u32(dev_lo, 8), vdupq_n_u32(0x00FF00FF)), alpha_slo);
               dev_lo = vorrq_u32( vandq_u32(rb,  vdupq_n_u32(0x00FF00FF)), vandq_u32(ag, vdupq_n_u32(0xFF00FF00)));
               dev_lo = vaddq_u32( vshlq_n_u32(alpha_lo, SK_A32_SHIFT), dev_lo);

               vst1q_u32(device, dev_lo);

               device += UNROLL_BY_2;
               mask += UNROLL_BY_2;
               w -= UNROLL_BY_2;

               if (w >= UNROLL_BY_4){
                   /*mask or alpha*/
                   uint32x2_t alpha_lo;
                   uint32x2_t alpha_slo; /*Saturated and scaled */
                   uint32x2_t rb,ag;
                   uint32x2_t dev_lo;

                   dev_lo = vld1_u32(device);
                   alpha_lo = vget_low_u32(vmovl_u16(vget_high_u16(alpha_full)));

                   /*SkAlpha255To256(255-aa)*/
                   //alpha_slo = vadd_u32(vsubq_u32( vdup_n_u32(0x000000FF), alpha_lo), vdup_n_u32(0x00000001));
                   alpha_slo = vsub_u32( vdup_n_u32(0x00000100), alpha_lo);

                   rb = vshr_n_u32( vmul_u32( vand_u32( dev_lo, vdup_n_u32(0x00FF00FF)), alpha_slo), 8);
                   ag = vmul_u32( vand_u32( vshr_n_u32(dev_lo, 8), vdup_n_u32(0x00FF00FF)), alpha_slo);
                   dev_lo = vorr_u32( vand_u32(rb,  vdup_n_u32(0x00FF00FF)), vand_u32(ag, vdup_n_u32(0xFF00FF00)));
                   dev_lo = vadd_u32 ( vshl_n_u32(alpha_lo, SK_A32_SHIFT), dev_lo);

                   vst1_u32(device, dev_lo);

                   device += UNROLL_BY_4;
                   mask += UNROLL_BY_4;
                   w -= UNROLL_BY_4;
               }
        }else if(w >= UNROLL_BY_4){
                /*mask or alpha*/
                uint16x8_t alpha_full = vmovl_u8(vld1_u8(mask));
                uint32x2_t alpha_lo;
                uint32x2_t alpha_slo; /*Saturated and scaled */
                uint32x2_t rb,ag;
                uint32x2_t dev_lo;

                dev_lo = vld1_u32(device);
                alpha_lo = vget_low_u32(vmovl_u16(vget_low_u16(alpha_full)));

                /*SkAlpha255To256(255-aa)*/
                //alpha_slo = vadd_u32(vsub_u32( vdup_n_u32(0x000000FF), alpha_lo), vdup_n_u32(0x00000001));
                alpha_slo = vsub_u32( vdup_n_u32(0x00000100), alpha_lo);

                rb = vshr_n_u32( vmul_u32( vand_u32( dev_lo, vdup_n_u32(0x00FF00FF)), alpha_slo), 8);
                ag = vmul_u32( vand_u32( vshr_n_u32(dev_lo, 8), vdup_n_u32(0x00FF00FF)), alpha_slo);
                dev_lo = vorr_u32( vand_u32(rb,  vdup_n_u32(0x00FF00FF)), vand_u32(ag, vdup_n_u32(0xFF00FF00)));
                dev_lo = vadd_u32 ( vshl_n_u32(alpha_lo, SK_A32_SHIFT), dev_lo);

                vst1_u32(device, dev_lo);

                device += UNROLL_BY_4;
                mask += UNROLL_BY_4;
                w -= UNROLL_BY_4;
        }

        /*residuals (which is everything that cannot be handled by neon) */
        while( w > 0){
            unsigned aa = *mask++;
            if(aa == 255){
              *device =  0xFF000000;
            }else if (aa != 0){
              *device = (aa << SK_A32_SHIFT) + SkAlphaMulQ(*device, SkAlpha255To256(255 - aa));
            }
            device += 1;
            --w;
        }
        device = (uint32_t*)((char*)device + dstRB);
        mask += maskRB;
    } while (--height != 0);
#else
    do{
        int w = width;
        do {
            unsigned aa = *mask++;
            *device = (aa << SK_A32_SHIFT) + SkAlphaMulQ(*device, SkAlpha255To256(255 - aa));
            device += 1;
        } while (--w != 0);
        device = (uint32_t*)((char*)device + dstRB);
        mask += maskRB;
    } while (--height != 0);
#endif
}

SkBlitMask::BlitLCD16RowProc SkBlitMask::BlitLCD16RowFactory(bool isOpaque) {
    BlitLCD16RowProc proc = PlatformBlitRowProcs16(isOpaque);
    if (proc) {
        return proc;
    }

    if (isOpaque) {
        return  SkBlitLCD16OpaqueRow;
    } else {
        return  SkBlitLCD16Row;
    }
}

static void D32_LCD16_Proc(void* SK_RESTRICT dst, size_t dstRB,
                           const void* SK_RESTRICT mask, size_t maskRB,
                           SkColor color, int width, int height) {

    SkPMColor*        dstRow = (SkPMColor*)dst;
    const uint16_t* srcRow = (const uint16_t*)mask;
    SkPMColor       opaqueDst;

    SkBlitMask::BlitLCD16RowProc proc = NULL;
    bool isOpaque = (0xFF == SkColorGetA(color));
    proc = SkBlitMask::BlitLCD16RowFactory(isOpaque);
    SkASSERT(proc != NULL);

    if (isOpaque) {
        opaqueDst = SkPreMultiplyColor(color);
    } else {
        opaqueDst = 0;  // ignored
    }

    do {
        proc(dstRow, srcRow, color, width, opaqueDst);
        dstRow = (SkPMColor*)((char*)dstRow + dstRB);
        srcRow = (const uint16_t*)((const char*)srcRow + maskRB);
    } while (--height != 0);
}

///////////////////////////////////////////////////////////////////////////////

static void blit_lcd32_opaque_row(SkPMColor* SK_RESTRICT dst,
                                  const SkPMColor* SK_RESTRICT src,
                                  SkColor color, int width) {
    int srcR = SkColorGetR(color);
    int srcG = SkColorGetG(color);
    int srcB = SkColorGetB(color);

    for (int i = 0; i < width; i++) {
        SkPMColor mask = src[i];
        if (0 == mask) {
            continue;
        }

        SkPMColor d = dst[i];

        int maskR = SkGetPackedR32(mask);
        int maskG = SkGetPackedG32(mask);
        int maskB = SkGetPackedB32(mask);

        // Now upscale them to 0..256, so we can use SkAlphaBlend
        maskR = SkAlpha255To256(maskR);
        maskG = SkAlpha255To256(maskG);
        maskB = SkAlpha255To256(maskB);

        int dstR = SkGetPackedR32(d);
        int dstG = SkGetPackedG32(d);
        int dstB = SkGetPackedB32(d);

        // LCD blitting is only supported if the dst is known/required
        // to be opaque
        dst[i] = SkPackARGB32(0xFF,
                              SkAlphaBlend(srcR, dstR, maskR),
                              SkAlphaBlend(srcG, dstG, maskG),
                              SkAlphaBlend(srcB, dstB, maskB));
    }
}

static void blit_lcd32_row(SkPMColor* SK_RESTRICT dst,
                           const SkPMColor* SK_RESTRICT src,
                           SkColor color, int width) {
    int srcA = SkColorGetA(color);
    int srcR = SkColorGetR(color);
    int srcG = SkColorGetG(color);
    int srcB = SkColorGetB(color);

    srcA = SkAlpha255To256(srcA);

    for (int i = 0; i < width; i++) {
        SkPMColor mask = src[i];
        if (0 == mask) {
            continue;
        }

        SkPMColor d = dst[i];

        int maskR = SkGetPackedR32(mask);
        int maskG = SkGetPackedG32(mask);
        int maskB = SkGetPackedB32(mask);

        // Now upscale them to 0..256, so we can use SkAlphaBlend
        maskR = SkAlpha255To256(maskR);
        maskG = SkAlpha255To256(maskG);
        maskB = SkAlpha255To256(maskB);

        maskR = maskR * srcA >> 8;
        maskG = maskG * srcA >> 8;
        maskB = maskB * srcA >> 8;

        int dstR = SkGetPackedR32(d);
        int dstG = SkGetPackedG32(d);
        int dstB = SkGetPackedB32(d);

        // LCD blitting is only supported if the dst is known/required
        // to be opaque
        dst[i] = SkPackARGB32(0xFF,
                              SkAlphaBlend(srcR, dstR, maskR),
                              SkAlphaBlend(srcG, dstG, maskG),
                              SkAlphaBlend(srcB, dstB, maskB));
    }
}

static void D32_LCD32_Blend(void* SK_RESTRICT dst, size_t dstRB,
                            const void* SK_RESTRICT mask, size_t maskRB,
                            SkColor color, int width, int height) {
    SkASSERT(height > 0);
    SkPMColor* SK_RESTRICT dstRow = (SkPMColor*)dst;
    const SkPMColor* SK_RESTRICT srcRow = (const SkPMColor*)mask;

    do {
        blit_lcd32_row(dstRow, srcRow, color, width);
        dstRow = (SkPMColor*)((char*)dstRow + dstRB);
        srcRow = (const SkPMColor*)((const char*)srcRow + maskRB);
    } while (--height != 0);
}

static void D32_LCD32_Opaque(void* SK_RESTRICT dst, size_t dstRB,
                             const void* SK_RESTRICT mask, size_t maskRB,
                             SkColor color, int width, int height) {
    SkASSERT(height > 0);
    SkPMColor* SK_RESTRICT dstRow = (SkPMColor*)dst;
    const SkPMColor* SK_RESTRICT srcRow = (const SkPMColor*)mask;

    do {
        blit_lcd32_opaque_row(dstRow, srcRow, color, width);
        dstRow = (SkPMColor*)((char*)dstRow + dstRB);
        srcRow = (const SkPMColor*)((const char*)srcRow + maskRB);
    } while (--height != 0);
}

///////////////////////////////////////////////////////////////////////////////

static SkBlitMask::ColorProc D32_A8_Factory(SkColor color) {
    if (SK_ColorBLACK == color) {
        return D32_A8_Black;
    } else if (0xFF == SkColorGetA(color)) {
        return D32_A8_Opaque;
    } else {
        return D32_A8_Color;
    }
}

static SkBlitMask::ColorProc D32_LCD32_Factory(SkColor color) {
    return (0xFF == SkColorGetA(color)) ? D32_LCD32_Opaque : D32_LCD32_Blend;
}

SkBlitMask::ColorProc SkBlitMask::ColorFactory(SkColorType ct,
                                               SkMask::Format format,
                                               SkColor color) {
    ColorProc proc = PlatformColorProcs(ct, format, color);
    if (proc) {
        return proc;
    }

    switch (ct) {
        case kN32_SkColorType:
            switch (format) {
                case SkMask::kA8_Format:
                    return D32_A8_Factory(color);
                case SkMask::kLCD16_Format:
                    return D32_LCD16_Proc;
                case SkMask::kLCD32_Format:
                    return D32_LCD32_Factory(color);
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return NULL;
}

bool SkBlitMask::BlitColor(const SkBitmap& device, const SkMask& mask,
                           const SkIRect& clip, SkColor color) {
    ColorProc proc = ColorFactory(device.colorType(), mask.fFormat, color);
    if (proc) {
        int x = clip.fLeft;
        int y = clip.fTop;
        proc(device.getAddr32(x, y), device.rowBytes(), mask.getAddr(x, y),
             mask.fRowBytes, color, clip.width(), clip.height());
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void BW_RowProc_Blend(SkPMColor* SK_RESTRICT dst,
                             const uint8_t* SK_RESTRICT mask,
                             const SkPMColor* SK_RESTRICT src, int count) {
    int i, octuple = (count + 7) >> 3;
    for (i = 0; i < octuple; ++i) {
        int m = *mask++;
        if (m & 0x80) { dst[0] = SkPMSrcOver(src[0], dst[0]); }
        if (m & 0x40) { dst[1] = SkPMSrcOver(src[1], dst[1]); }
        if (m & 0x20) { dst[2] = SkPMSrcOver(src[2], dst[2]); }
        if (m & 0x10) { dst[3] = SkPMSrcOver(src[3], dst[3]); }
        if (m & 0x08) { dst[4] = SkPMSrcOver(src[4], dst[4]); }
        if (m & 0x04) { dst[5] = SkPMSrcOver(src[5], dst[5]); }
        if (m & 0x02) { dst[6] = SkPMSrcOver(src[6], dst[6]); }
        if (m & 0x01) { dst[7] = SkPMSrcOver(src[7], dst[7]); }
        src += 8;
        dst += 8;
    }
    count &= 7;
    if (count > 0) {
        int m = *mask;
        do {
            if (m & 0x80) { dst[0] = SkPMSrcOver(src[0], dst[0]); }
            m <<= 1;
            src += 1;
            dst += 1;
        } while (--count > 0);
    }
}

static void BW_RowProc_Opaque(SkPMColor* SK_RESTRICT dst,
                              const uint8_t* SK_RESTRICT mask,
                              const SkPMColor* SK_RESTRICT src, int count) {
    int i, octuple = (count + 7) >> 3;
    for (i = 0; i < octuple; ++i) {
        int m = *mask++;
        if (m & 0x80) { dst[0] = src[0]; }
        if (m & 0x40) { dst[1] = src[1]; }
        if (m & 0x20) { dst[2] = src[2]; }
        if (m & 0x10) { dst[3] = src[3]; }
        if (m & 0x08) { dst[4] = src[4]; }
        if (m & 0x04) { dst[5] = src[5]; }
        if (m & 0x02) { dst[6] = src[6]; }
        if (m & 0x01) { dst[7] = src[7]; }
        src += 8;
        dst += 8;
    }
    count &= 7;
    if (count > 0) {
        int m = *mask;
        do {
            if (m & 0x80) { dst[0] = SkPMSrcOver(src[0], dst[0]); }
            m <<= 1;
            src += 1;
            dst += 1;
        } while (--count > 0);
    }
}

static void A8_RowProc_Blend(SkPMColor* SK_RESTRICT dst,
                             const uint8_t* SK_RESTRICT mask,
                             const SkPMColor* SK_RESTRICT src, int count) {
    for (int i = 0; i < count; ++i) {
        if (mask[i]) {
            dst[i] = SkBlendARGB32(src[i], dst[i], mask[i]);
        }
    }
}

// expand the steps that SkAlphaMulQ performs, but this way we can
//  exand.. add.. combine
// instead of
// expand..combine add expand..combine
//
#define EXPAND0(v, m, s)    ((v) & (m)) * (s)
#define EXPAND1(v, m, s)    (((v) >> 8) & (m)) * (s)
#define COMBINE(e0, e1, m)  ((((e0) >> 8) & (m)) | ((e1) & ~(m)))

static void A8_RowProc_Opaque(SkPMColor* SK_RESTRICT dst,
                              const uint8_t* SK_RESTRICT mask,
                              const SkPMColor* SK_RESTRICT src, int count) {
    for (int i = 0; i < count; ++i) {
        int m = mask[i];
        if (m) {
            m += (m >> 7);
#if 1
            // this is slightly slower than the expand/combine version, but it
            // is much closer to the old results, so we use it for now to reduce
            // rebaselining.
            dst[i] = SkAlphaMulQ(src[i], m) + SkAlphaMulQ(dst[i], 256 - m);
#else
            uint32_t v = src[i];
            uint32_t s0 = EXPAND0(v, rbmask, m);
            uint32_t s1 = EXPAND1(v, rbmask, m);
            v = dst[i];
            uint32_t d0 = EXPAND0(v, rbmask, m);
            uint32_t d1 = EXPAND1(v, rbmask, m);
            dst[i] = COMBINE(s0 + d0, s1 + d1, rbmask);
#endif
        }
    }
}

static int upscale31To255(int value) {
    value = (value << 3) | (value >> 2);
    return value;
}

static int src_alpha_blend(int src, int dst, int srcA, int mask) {

    return dst + SkAlphaMul(src - SkAlphaMul(srcA, dst), mask);
}

static void LCD16_RowProc_Blend(SkPMColor* SK_RESTRICT dst,
                                const uint16_t* SK_RESTRICT mask,
                                const SkPMColor* SK_RESTRICT src, int count) {
    for (int i = 0; i < count; ++i) {
        uint16_t m = mask[i];
        if (0 == m) {
            continue;
        }

        SkPMColor s = src[i];
        SkPMColor d = dst[i];

        int srcA = SkGetPackedA32(s);
        int srcR = SkGetPackedR32(s);
        int srcG = SkGetPackedG32(s);
        int srcB = SkGetPackedB32(s);

        srcA += srcA >> 7;

        /*  We want all of these in 5bits, hence the shifts in case one of them
         *  (green) is 6bits.
         */
        int maskR = SkGetPackedR16(m) >> (SK_R16_BITS - 5);
        int maskG = SkGetPackedG16(m) >> (SK_G16_BITS - 5);
        int maskB = SkGetPackedB16(m) >> (SK_B16_BITS - 5);

        maskR = upscale31To255(maskR);
        maskG = upscale31To255(maskG);
        maskB = upscale31To255(maskB);

        int dstR = SkGetPackedR32(d);
        int dstG = SkGetPackedG32(d);
        int dstB = SkGetPackedB32(d);

        // LCD blitting is only supported if the dst is known/required
        // to be opaque
        dst[i] = SkPackARGB32(0xFF,
                              src_alpha_blend(srcR, dstR, srcA, maskR),
                              src_alpha_blend(srcG, dstG, srcA, maskG),
                              src_alpha_blend(srcB, dstB, srcA, maskB));
    }
}

static void LCD16_RowProc_Opaque(SkPMColor* SK_RESTRICT dst,
                                 const uint16_t* SK_RESTRICT mask,
                                 const SkPMColor* SK_RESTRICT src, int count) {
    for (int i = 0; i < count; ++i) {
        uint16_t m = mask[i];
        if (0 == m) {
            continue;
        }

        SkPMColor s = src[i];
        SkPMColor d = dst[i];

        int srcR = SkGetPackedR32(s);
        int srcG = SkGetPackedG32(s);
        int srcB = SkGetPackedB32(s);

        /*  We want all of these in 5bits, hence the shifts in case one of them
         *  (green) is 6bits.
         */
        int maskR = SkGetPackedR16(m) >> (SK_R16_BITS - 5);
        int maskG = SkGetPackedG16(m) >> (SK_G16_BITS - 5);
        int maskB = SkGetPackedB16(m) >> (SK_B16_BITS - 5);

        // Now upscale them to 0..32, so we can use blend32
        maskR = SkUpscale31To32(maskR);
        maskG = SkUpscale31To32(maskG);
        maskB = SkUpscale31To32(maskB);

        int dstR = SkGetPackedR32(d);
        int dstG = SkGetPackedG32(d);
        int dstB = SkGetPackedB32(d);

        // LCD blitting is only supported if the dst is known/required
        // to be opaque
        dst[i] = SkPackARGB32(0xFF,
                              SkBlend32(srcR, dstR, maskR),
                              SkBlend32(srcG, dstG, maskG),
                              SkBlend32(srcB, dstB, maskB));
    }
}

static void LCD32_RowProc_Blend(SkPMColor* SK_RESTRICT dst,
                                const SkPMColor* SK_RESTRICT mask,
                                const SkPMColor* SK_RESTRICT src, int count) {
    for (int i = 0; i < count; ++i) {
        SkPMColor m = mask[i];
        if (0 == m) {
            continue;
        }

        SkPMColor s = src[i];
        int srcA = SkGetPackedA32(s);
        int srcR = SkGetPackedR32(s);
        int srcG = SkGetPackedG32(s);
        int srcB = SkGetPackedB32(s);

        srcA = SkAlpha255To256(srcA);

        SkPMColor d = dst[i];

        int maskR = SkGetPackedR32(m);
        int maskG = SkGetPackedG32(m);
        int maskB = SkGetPackedB32(m);

        // Now upscale them to 0..256
        maskR = SkAlpha255To256(maskR);
        maskG = SkAlpha255To256(maskG);
        maskB = SkAlpha255To256(maskB);

        int dstR = SkGetPackedR32(d);
        int dstG = SkGetPackedG32(d);
        int dstB = SkGetPackedB32(d);

        // LCD blitting is only supported if the dst is known/required
        // to be opaque
        dst[i] = SkPackARGB32(0xFF,
                              src_alpha_blend(srcR, dstR, srcA, maskR),
                              src_alpha_blend(srcG, dstG, srcA, maskG),
                              src_alpha_blend(srcB, dstB, srcA, maskB));
    }
}

static void LCD32_RowProc_Opaque(SkPMColor* SK_RESTRICT dst,
                                 const SkPMColor* SK_RESTRICT mask,
                                 const SkPMColor* SK_RESTRICT src, int count) {
    for (int i = 0; i < count; ++i) {
        SkPMColor m = mask[i];
        if (0 == m) {
            continue;
        }

        SkPMColor s = src[i];
        SkPMColor d = dst[i];

        int maskR = SkGetPackedR32(m);
        int maskG = SkGetPackedG32(m);
        int maskB = SkGetPackedB32(m);

        int srcR = SkGetPackedR32(s);
        int srcG = SkGetPackedG32(s);
        int srcB = SkGetPackedB32(s);

        int dstR = SkGetPackedR32(d);
        int dstG = SkGetPackedG32(d);
        int dstB = SkGetPackedB32(d);

        // Now upscale them to 0..256, so we can use SkAlphaBlend
        maskR = SkAlpha255To256(maskR);
        maskG = SkAlpha255To256(maskG);
        maskB = SkAlpha255To256(maskB);

        // LCD blitting is only supported if the dst is known/required
        // to be opaque
        dst[i] = SkPackARGB32(0xFF,
                              SkAlphaBlend(srcR, dstR, maskR),
                              SkAlphaBlend(srcG, dstG, maskG),
                              SkAlphaBlend(srcB, dstB, maskB));
    }
}

SkBlitMask::RowProc SkBlitMask::RowFactory(SkColorType ct,
                                           SkMask::Format format,
                                           RowFlags flags) {
// make this opt-in until chrome can rebaseline
    RowProc proc = PlatformRowProcs(ct, format, flags);
    if (proc) {
        return proc;
    }

    static const RowProc gProcs[] = {
        // need X coordinate to handle BW
        false ? (RowProc)BW_RowProc_Blend : NULL, // suppress unused warning
        false ? (RowProc)BW_RowProc_Opaque : NULL, // suppress unused warning
        (RowProc)A8_RowProc_Blend,      (RowProc)A8_RowProc_Opaque,
        (RowProc)LCD16_RowProc_Blend,   (RowProc)LCD16_RowProc_Opaque,
        (RowProc)LCD32_RowProc_Blend,   (RowProc)LCD32_RowProc_Opaque,
    };

    int index;
    switch (ct) {
        case kN32_SkColorType:
            switch (format) {
                case SkMask::kBW_Format:    index = 0; break;
                case SkMask::kA8_Format:    index = 2; break;
                case SkMask::kLCD16_Format: index = 4; break;
                case SkMask::kLCD32_Format: index = 6; break;
                default:
                    return NULL;
            }
            if (flags & kSrcIsOpaque_RowFlag) {
                index |= 1;
            }
            SkASSERT((size_t)index < SK_ARRAY_COUNT(gProcs));
            return gProcs[index];
        default:
            break;
    }
    return NULL;
}
