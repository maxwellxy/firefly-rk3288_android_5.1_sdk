/*
 * Copyright 2014 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#if defined(__clang__) || (defined(__GNUC__) && !defined(SK_BUILD_FOR_MAC))

#define EXTRACT_ALPHA(var1, var2) \
    movdqa      %var1, %var2;           /* Clone source pixels to extract alpha */\
    psrlw       $8, %var2;              /* Discard red and blue, leaving alpha and green */\
    pshufhw     $0xF5, %var2, %var2;    /* Repeat alpha for scaling (high) */\
    movdqa      %xmm6, %xmm4;           \
    pshuflw     $0xF5, %var2, %var2;    /* Repeat alpha for scaling (low) */\
    movdqa      %xmm5, %xmm3;           \
    psubw       %var2, %xmm4            /* Finalize alpha calculations */

#define SCALE_PIXELS \
    psllw       $8, %xmm5;              /* Filter out red and blue components */\
    pmulhuw     %xmm4, %xmm5;           /* Scale red and blue */\
    psrlw       $8, %xmm3;              /* Filter out alpha and green components */\
    pmullw      %xmm4, %xmm3            /* Scale alpha and green */


/*
 * void S32A_Opaque_BlitRow32_SSE4(SkPMColor* SK_RESTRICT dst,
 *                                 const SkPMColor* SK_RESTRICT src,
 *                                 int count, U8CPU alpha)
 *
 * This function is divided into six blocks: initialization, blit 4-15 pixels,
 * blit 0-3 pixels, align destination for 16+ pixel blits,
 * blit 16+ pixels with source unaligned, blit 16+ pixels with source aligned.
 * There are some code reuse between the blocks.
 *
 * The primary optimization comes from checking the source pixels' alpha value.
 * If the alpha is zero, the pixel can be skipped entirely.
 * If the alpha is fully opaque, the pixel can be copied directly to the destination.
 * According to collected statistics, these two cases are the most common.
 * The main loop(s) uses pre-loading and unrolling in an attempt to reduce the
 * memory latency worse-case.
 */

#ifdef __clang__
    .text
#else
    .section .text.sse4.2,"ax",@progbits
    .type S32A_Opaque_BlitRow32_SSE4_asm, @function
#endif
    .p2align 4
#if defined(SK_BUILD_FOR_MAC)
    .global _S32A_Opaque_BlitRow32_SSE4_asm
    .private_extern _S32A_Opaque_BlitRow32_SSE4_asm
_S32A_Opaque_BlitRow32_SSE4_asm:
#else
    .global S32A_Opaque_BlitRow32_SSE4_asm
    .hidden S32A_Opaque_BlitRow32_SSE4_asm
S32A_Opaque_BlitRow32_SSE4_asm:
#endif
    .cfi_startproc
    prefetcht0  (%rsi)
    movl        %edx, %ecx              // Pixel count
    movq        %rdi, %rdx              // Destination pointer
    movq        %rsi, %rax              // Source pointer

    // Setup SSE constants
    movdqa      .LAlphaCheckMask(%rip), %xmm7  // 0xFF000000 mask to check alpha
    movdqa      .LInverseAlphaCalc(%rip), %xmm6// 16-bit 256 to calculate inv. alpha
    movdqa      .LResultMergeMask(%rip), %xmm0 // 0x00FF00FF mask (Must be in xmm0 because of pblendvb)

    subl        $4, %ecx                // Check if we have only 0-3 pixels
    js          .LReallySmall
    cmpl        $11, %ecx               // Do we have enough pixels to run the main loop?
    ja          .LBigBlit

    // Handle small blits (4-15 pixels)
    ////////////////////////////////////////////////////////////////////////////////
    xorq        %rdi, %rdi              // Reset offset to zero

.LSmallLoop:
    lddqu       (%rax, %rdi), %xmm1     // Load four source pixels
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    ja          .LSmallAlphaNotOpaqueOrZero
    jz          .LSmallAlphaZero
    movdqu      %xmm1, (%rdx, %rdi)     // Store four destination pixels
.LSmallAlphaZero:
    addq        $16, %rdi
    subl        $4, %ecx                // Check if there are four additional pixels, at least
    jns         .LSmallLoop
    jmp         .LSmallRemaining

    // Handle mixed alphas (calculate and scale)
    .p2align 4
.LSmallAlphaNotOpaqueOrZero:
    lddqu       (%rdx, %rdi), %xmm5     // Load four destination pixels
    EXTRACT_ALPHA(xmm1, xmm2)           // Extract and clone alpha value
    SCALE_PIXELS                        // Scale pixels using alpha

    addq        $16, %rdi
    subl        $4, %ecx                // Check if there are four additional pixels, at least
    pblendvb    %xmm5, %xmm3            // Mask in %xmm0, implicitly
    paddb       %xmm3, %xmm1            // Add source and destination pixels together
    movdqu      %xmm1, -16(%rdx, %rdi)  // Store four destination pixels
    jns         .LSmallLoop

    // Handle the last 0-3 pixels (also used by the main loops)
.LSmallRemaining:
    cmpl        $-4, %ecx               // Check if we are done
    je          .LSmallExit
    sall        $2, %ecx                // Calculate offset for last pixels
    movslq      %ecx, %rcx
    addq        %rcx, %rdi

    lddqu       (%rax, %rdi), %xmm1     // Load last four source pixels (overlapping)
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    jc          .LSmallRemainingStoreAll// If all alphas are opaque, just store (overlapping)
    jz          .LSmallExit             // If all alphas are zero, skip the pixels completely

    // Handle mixed alphas (calculate and scale)
    lddqu       (%rdx, %rdi), %xmm5     // Load last four destination pixels (overlapping)
    EXTRACT_ALPHA(xmm1, xmm2)           // Extract and clone alpha value

    psllw       $8, %xmm3               // Filter out red and blue components
    pmulhuw     %xmm4, %xmm3            // Scale red and blue
    movdqa      %xmm5, %xmm2
    psrlw       $8, %xmm2               // Filter out alpha and green components
    pmullw      %xmm4, %xmm2            // Scale alpha and green

    cmpl        $-8, %ecx               // Check how many pixels should be written
    pblendvb    %xmm3, %xmm2            // Combine results (mask in %xmm0, implicitly)
    paddb       %xmm2, %xmm1            // Add source and destination pixels together
    jb          .LSmallPixelsLeft1
    ja          .LSmallPixelsLeft3      // To avoid double-blending the overlapping pixels...
    pblendw     $0xF0, %xmm1, %xmm5     // Merge only the final two pixels to the destination
    movdqu      %xmm5, (%rdx, %rdi)     // Store last two destination pixels
.LSmallExit:
    ret

.LSmallPixelsLeft1:
    pblendw     $0xC0, %xmm1, %xmm5     // Merge only the final pixel to the destination
    movdqu      %xmm5, (%rdx, %rdi)     // Store last destination pixel
    ret

.LSmallPixelsLeft3:
    pblendw     $0xFC, %xmm1, %xmm5     // Merge only the final three pixels to the destination
    movdqu      %xmm5, (%rdx, %rdi)     // Store last three destination pixels
    ret

.LSmallRemainingStoreAll:
    movdqu      %xmm1, (%rdx, %rdi)     // Store last destination pixels (overwrite)
    ret

    // Handle really small blits (0-3 pixels)
    ////////////////////////////////////////////////////////////////////////////////
.LReallySmall:
    addl        $4, %ecx
    jle         .LReallySmallExit
    pcmpeqd     %xmm1, %xmm1
    cmpl        $2, %ecx                // Check how many pixels should be read
    pinsrd      $0x0, (%rax), %xmm1     // Load one source pixel
    pinsrd      $0x0, (%rdx), %xmm5     // Load one destination pixel
    jb          .LReallySmallCalc
    pinsrd      $0x1, 4(%rax), %xmm1    // Load second source pixel
    pinsrd      $0x1, 4(%rdx), %xmm5    // Load second destination pixel
    je          .LReallySmallCalc
    pinsrd      $0x2, 8(%rax), %xmm1    // Load third source pixel
    pinsrd      $0x2, 8(%rdx), %xmm5    // Load third destination pixel

.LReallySmallCalc:
    ptest       %xmm7, %xmm1            // Check if all alphas are opaque
    jc          .LReallySmallStore      // If all alphas are opaque, just store

    // Handle mixed alphas (calculate and scale)
    EXTRACT_ALPHA(xmm1, xmm2)           // Extract and clone alpha value

    pand        %xmm0, %xmm5            // Filter out red and blue components
    pmullw      %xmm4, %xmm5            // Scale red and blue
    psrlw       $8, %xmm3               // Filter out alpha and green components
    pmullw      %xmm4, %xmm3            // Scale alpha and green

    psrlw       $8, %xmm5               // Combine results
    pblendvb    %xmm5, %xmm3            // Mask in %xmm0, implicitly
    paddb       %xmm3, %xmm1            // Add source and destination pixels together

.LReallySmallStore:
    cmpl        $2, %ecx                // Check how many pixels should be written
    pextrd      $0x0, %xmm1, (%rdx)     // Store one destination pixel
    jb          .LReallySmallExit
    pextrd      $0x1, %xmm1, 4(%rdx)    // Store second destination pixel
    je          .LReallySmallExit
    pextrd      $0x2, %xmm1, 8(%rdx)    // Store third destination pixel
.LReallySmallExit:
    ret

    // Handle bigger blit operations (16+ pixels)
    ////////////////////////////////////////////////////////////////////////////////
    .p2align 4
.LBigBlit:
    // Align destination?
    testl       $0xF, %edx
    lddqu       (%rax), %xmm1           // Pre-load four source pixels
    jz          .LAligned

    movq        %rdx, %rdi              // Calculate alignment of destination pointer
    negq        %rdi
    andl        $0xF, %edi

    // Handle 1-3 pixels to align destination
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    jz          .LAlignDone             // If all alphas are zero, just skip
    lddqu       (%rdx), %xmm5           // Load four destination pixels
    jc          .LAlignStore            // If all alphas are opaque, just store

    // Handle mixed alphas (calculate and scale)
    EXTRACT_ALPHA(xmm1, xmm2)           // Extract and clone alpha value

    psllw       $8, %xmm3               // Filter out red and blue components
    pmulhuw     %xmm4, %xmm3            // Scale red and blue
    movdqa      %xmm5, %xmm2
    psrlw       $8, %xmm2               // Filter out alpha and green components
    pmullw      %xmm4, %xmm2            // Scale alpha and green

    pblendvb    %xmm3, %xmm2            // Combine results (mask in %xmm0, implicitly)
    paddb       %xmm2, %xmm1            // Add source and destination pixels together

.LAlignStore:
    cmpl        $8, %edi                // Check how many pixels should be written
    jb          .LAlignPixelsLeft1
    ja          .LAlignPixelsLeft3
    pblendw     $0x0F, %xmm1, %xmm5     // Blend two pixels
    jmp .LAlignStorePixels

.LAlignPixelsLeft1:
    pblendw     $0x03, %xmm1, %xmm5     // Blend one pixel
    jmp .LAlignStorePixels

.LAlignPixelsLeft3:
    pblendw     $0x3F, %xmm1, %xmm5     // Blend three pixels

.LAlignStorePixels:
    movdqu      %xmm5, (%rdx)           // Store destination pixels

.LAlignDone:
    addq        %rdi, %rax              // Adjust pointers and pixel count
    addq        %rdi, %rdx
    shrq        $2, %rdi
    lddqu       (%rax), %xmm1           // Pre-load new source pixels (after alignment)
    subl        %edi, %ecx

.LAligned:                              // Destination is guaranteed to be 16 byte aligned
    xorq        %rdi, %rdi              // Reset offset to zero
    subl        $8, %ecx                // Decrease counter (Reserve four pixels for the cleanup)
    testl       $0xF, %eax              // Check alignment of source pointer
    jz          .LAlignedLoop

    // Source not aligned to destination
    ////////////////////////////////////////////////////////////////////////////////
    .p2align 4
.LUnalignedLoop:                        // Main loop for unaligned, handles eight pixels per iteration
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    ja          .LAlphaNotOpaqueOrZero00
    lddqu       16(%rax, %rdi), %xmm2   // Pre-load four source pixels
    jz          .LAlphaZero00
    movdqa      %xmm1, (%rdx, %rdi)     // Store four destination pixels

.LAlphaZero00:
    ptest       %xmm7, %xmm2            // Check if all alphas are zero or opaque
    ja          .LAlphaNotOpaqueOrZero01
    lddqu       32(%rax, %rdi), %xmm1   // Pre-load four source pixels
    jz          .LAlphaZero01
    movdqa      %xmm2, 16(%rdx, %rdi)   // Store four destination pixels

.LAlphaZero01:
    addq        $32, %rdi               // Adjust offset and pixel count
    subl        $8, %ecx
    jae         .LUnalignedLoop
    addl        $8, %ecx                // Adjust pixel count
    jmp         .LLoopCleanup0

    .p2align 4
.LAlphaNotOpaqueOrZero00:
    movdqa      (%rdx, %rdi), %xmm5     // Load four destination pixels
    EXTRACT_ALPHA(xmm1, xmm2)           // Extract and clone alpha value
    SCALE_PIXELS                        // Scale pixels using alpha

    lddqu       16(%rax, %rdi), %xmm2   // Pre-load four source pixels
    pblendvb    %xmm5, %xmm3            // Combine results (mask in %xmm0, implicitly)
    paddb       %xmm3, %xmm1            // Add source and destination pixels together
    movdqa      %xmm1, (%rdx, %rdi)     // Store four destination pixels

    // Handle next four pixels
    ptest       %xmm7, %xmm2            // Check if all alphas are zero or opaque
    ja          .LAlphaNotOpaqueOrZero01
    lddqu       32(%rax, %rdi), %xmm1   // Pre-load four source pixels
    jz          .LAlphaZero02
    movdqa      %xmm2, 16(%rdx, %rdi)   // Store four destination pixels
.LAlphaZero02:
    addq        $32, %rdi               // Adjust offset and pixel count
    subl        $8, %ecx
    jae         .LUnalignedLoop
    addl        $8, %ecx                // Adjust pixel count
    jmp         .LLoopCleanup0

    .p2align 4
.LAlphaNotOpaqueOrZero01:
    movdqa      16(%rdx, %rdi), %xmm5   // Load four destination pixels
    EXTRACT_ALPHA(xmm2, xmm1)           // Extract and clone alpha value
    SCALE_PIXELS                        // Scale pixels using alpha

    lddqu       32(%rax, %rdi), %xmm1   // Pre-load four source pixels
    addq        $32, %rdi
    pblendvb    %xmm5, %xmm3            // Combine results (mask in %xmm0, implicitly)
    paddb       %xmm3, %xmm2            // Add source and destination pixels together
    subl        $8, %ecx
    movdqa      %xmm2, -16(%rdx, %rdi)  // Store four destination pixels
    jae         .LUnalignedLoop
    addl        $8, %ecx                // Adjust pixel count

    // Cleanup - handle pending pixels from loop
.LLoopCleanup0:
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    ja          .LAlphaNotOpaqueOrZero02
    jz          .LAlphaZero03
    movdqa      %xmm1, (%rdx, %rdi)     // Store four destination pixels
.LAlphaZero03:
    addq        $16, %rdi
    subl        $4, %ecx
    js          .LSmallRemaining        // Reuse code from small loop

.LRemain0:
    lddqu       (%rax, %rdi), %xmm1     // Load four source pixels
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    ja          .LAlphaNotOpaqueOrZero02
    jz          .LAlphaZero04
    movdqa      %xmm1, (%rdx, %rdi)     // Store four destination pixels
.LAlphaZero04:
    addq        $16, %rdi
    subl        $4, %ecx
    jmp         .LSmallRemaining        // Reuse code from small loop

.LAlphaNotOpaqueOrZero02:
    movdqa      (%rdx, %rdi), %xmm5     // Load four destination pixels
    EXTRACT_ALPHA(xmm1, xmm2)           // Extract and clone alpha value
    SCALE_PIXELS                        // Scale pixels using alpha

    addq        $16, %rdi
    subl        $4, %ecx
    pblendvb    %xmm5, %xmm3            // Combine results (mask in %xmm0, implicitly)
    paddb       %xmm3, %xmm1            // Add source and destination pixels together
    movdqa      %xmm1, -16(%rdx, %rdi)  // Store four destination pixels
    js          .LSmallRemaining        // Reuse code from small loop
    jmp         .LRemain0

    // Source aligned to destination
    ////////////////////////////////////////////////////////////////////////////////
    .p2align 4
.LAlignedLoop:                          // Main loop for aligned, handles eight pixels per iteration
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    ja          .LAlphaNotOpaqueOrZero10
    movdqa      16(%rax, %rdi), %xmm2   // Pre-load four source pixels
    jz          .LAlphaZero10
    movdqa      %xmm1, (%rdx, %rdi)     // Store four destination pixels

.LAlphaZero10:
    ptest       %xmm7, %xmm2            // Check if all alphas are zero or opaque
    ja          .LAlphaNotOpaqueOrZero11
    movdqa      32(%rax, %rdi), %xmm1   // Pre-load four source pixels
    jz          .LAlphaZero11
    movdqa      %xmm2, 16(%rdx, %rdi)   // Store four destination pixels

.LAlphaZero11:
    addq        $32, %rdi               // Adjust offset and pixel count
    subl        $8, %ecx
    jae         .LAlignedLoop
    addl        $8, %ecx                // Adjust pixel count
    jmp         .LLoopCleanup1

    .p2align 4
.LAlphaNotOpaqueOrZero10:
    movdqa      (%rdx, %rdi), %xmm5     // Load four destination pixels
    EXTRACT_ALPHA(xmm1, xmm2)           // Extract and clone alpha value
    SCALE_PIXELS                        // Scale pixels using alpha

    movdqa      16(%rax, %rdi), %xmm2   // Pre-load four source pixels
    pblendvb    %xmm5, %xmm3            // Combine results (mask in %xmm0, implicitly)
    paddb       %xmm3, %xmm1            // Add source and destination pixels together
    movdqa      %xmm1, (%rdx, %rdi)     // Store four destination pixels

    // Handle next four pixels
    ptest       %xmm7, %xmm2            // Check if all alphas are zero or opaque
    ja          .LAlphaNotOpaqueOrZero11
    movdqa      32(%rax, %rdi), %xmm1   // Pre-load four source pixels
    jz          .LAlphaZero12
    movdqa      %xmm2, 16(%rdx, %rdi)   // Store four destination pixels
.LAlphaZero12:
    addq        $32, %rdi               // Adjust offset and pixel count
    subl        $8, %ecx
    jae         .LAlignedLoop
    addl        $8, %ecx                // Adjust pixel count
    jmp         .LLoopCleanup1

    .p2align 4
.LAlphaNotOpaqueOrZero11:
    movdqa      16(%rdx, %rdi), %xmm5   // Load four destination pixels
    EXTRACT_ALPHA(xmm2, xmm1)           // Extract and clone alpha value
    SCALE_PIXELS                        // Scale pixels using alpha
    movdqa      32(%rax, %rdi), %xmm1   // Pre-load four source pixels

    addq        $32, %rdi
    pblendvb    %xmm5, %xmm3            // Combine results (mask in %xmm0, implicitly)
    paddb       %xmm3, %xmm2            // Add source and destination pixels together
    subl        $8, %ecx
    movdqa      %xmm2, -16(%rdx, %rdi)  // Store four destination pixels
    jae         .LAlignedLoop
    addl        $8, %ecx                // Adjust pixel count

    // Cleanup - handle four pending pixels from loop
.LLoopCleanup1:
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    ja          .LAlphaNotOpaqueOrZero12
    jz          .LAlphaZero13
    movdqa      %xmm1, (%rdx, %rdi)     // Store four destination pixels
.LAlphaZero13:
    addq        $16, %rdi
    subl        $4, %ecx
    js          .LSmallRemaining        // Reuse code from small loop

.LRemain1:
    movdqa      (%rax, %rdi), %xmm1     // Pre-load four source pixels
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    ja          .LAlphaNotOpaqueOrZero12
    jz          .LAlphaZero14
    movdqa      %xmm1, (%rdx, %rdi)     // Store four destination pixels
.LAlphaZero14:
    addq        $16, %rdi
    subl        $4, %ecx
    jmp         .LSmallRemaining        // Reuse code from small loop

.LAlphaNotOpaqueOrZero12:
    movdqa      (%rdx, %rdi), %xmm5     // Load four destination pixels
    EXTRACT_ALPHA(xmm1, xmm2)           // Extract and clone alpha value
    SCALE_PIXELS                        // Scale pixels using alpha

    addq        $16, %rdi
    subl        $4, %ecx
    pblendvb    %xmm5, %xmm3            // Combine results (mask in %xmm0, implicitly)
    paddb       %xmm3, %xmm1            // Add source and destination pixels together
    movdqa      %xmm1, -16(%rdx, %rdi)  // Store four destination pixels
    js          .LSmallRemaining        // Reuse code from small loop
    jmp         .LRemain1

    .cfi_endproc
#ifndef __clang__
    .size S32A_Opaque_BlitRow32_SSE4_asm, .-S32A_Opaque_BlitRow32_SSE4_asm
#endif

    // Constants for SSE code
#ifndef __clang__
    .section .rodata
#endif
    .p2align 4
.LAlphaCheckMask:
    .long   0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000
.LInverseAlphaCalc:
    .word   256, 256, 256, 256, 256, 256, 256, 256
.LResultMergeMask:
    .long   0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF

/*
 * void S32A_Blend_BlitRow32_SSE4(SkPMColor* SK_RESTRICT dst,
 *                                const SkPMColor* SK_RESTRICT src,
 *                                int count, U8CPU alpha)
 *
 * The primary optimization comes from checking the source pixels' alpha value.
 * If the alpha is zero, the pixel can be skipped entirely.
 * According to collected statistics, this case is quite common.
 * The main loop(s) uses pre-loading and unrolling in an attempt to reduce the
 * memory latency worse-case.
 */

    .section .text.sse4.2,"ax",@progbits
    .type S32A_Blend_BlitRow32_SSE4_asm, @function
    .globl S32A_Blend_BlitRow32_SSE4_asm
    .hidden S32A_Blend_BlitRow32_SSE4_asm
    .p2align 4
S32A_Blend_BlitRow32_SSE4_asm:
    .cfi_startproc
    prefetcht0  (%rsi)
    movq        %rsi, %rax              // Source pointer
    movl        %ecx, %esi              // Global alpha
    movl        %edx, %ecx              // Pixel count
    movq        %rdi, %rdx              // Destination pointer

    // Setup SSE constants
    addl        $1, %esi                // Modify global alpha range to 1..256
    imul        $0x10001, %esi          // Duplicate alpha to two 16-bit values
    movdqa      .LAlphaCheckMask(%rip), %xmm7  // 0xFF000000 mask to check alpha
    movdqa      .LInverseAlphaCalc(%rip), %xmm6// 16-bit 256 to calculate inv. alpha
    movdqa      .LResultMergeMask(%rip), %xmm0 // 0x00FF00FF mask (Must be in xmm0 because of pblendvb)
    movd        %esi, %xmm8             // Create global alpha constant
    pshufd      $0, %xmm8, %xmm8

    subl        $4, %ecx                // Check if we have only 0-3 pixels
    js          .LBlendReallySmall
    cmpl        $11, %ecx               // Do we have enough pixels to run the main loop?
    ja          .LBlendBigBlit

    // Handle small blits (4-15 pixels)
    // ********************************
    xorq        %rdi, %rdi              // Reset offset to zero

.LBlendSmallLoop:
    lddqu       (%rax, %rdi), %xmm1     // Load four source pixels
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    jz          .LBlendSmallAlphaZero

    // Handle global and pixel alphas (calculate and scale)
    movdqa      %xmm0, %xmm3
    pandn       %xmm1, %xmm3            // Filter out alpha and green components from source
    lddqu       (%rdx, %rdi), %xmm5     // Load last four destination pixels (overlapping)
    pmulhuw     %xmm8, %xmm3            // Scale alpha and green
    psllw       $8, %xmm1               // Filter out red and blue components from source
    pmulhuw     %xmm8, %xmm1            // Scale red and blue

    pshufhw     $0xF5, %xmm3, %xmm2     // Repeat source alpha for scaling of destination (high)
    psllw       $8, %xmm3               // Combine scaled source results
    pshuflw     $0xF5, %xmm2, %xmm2     // Repeat source alpha (low)
    movdqa      %xmm6, %xmm4            // Clone 256 constant
    por         %xmm3, %xmm1
    psubw       %xmm2, %xmm4            // Finalize alpha calculations
    movdqa      %xmm5, %xmm3            // Clone destination pixels

    psllw       $8, %xmm5               // Filter out red and blue components
    pmulhuw     %xmm4, %xmm5            // Scale red and blue
    psrlw       $8, %xmm3               // Filter out alpha and green components
    pmullw      %xmm4, %xmm3            // Scale alpha and green

    addq        $16, %rdi
    subl        $4, %ecx                // Check if we can store all four pixels
    pblendvb    %xmm0, %xmm5, %xmm3     // Combine scaled destination results
    paddb       %xmm3, %xmm1            // Add source and destination pixels together
    movdqu      %xmm1, -16(%rdx, %rdi)  // Store four destination pixels
    jns         .LBlendSmallLoop
    jmp         .LBlendSmallRemaining

    .p2align 4
.LBlendSmallAlphaZero:
    addq        $16, %rdi
    subl        $4, %ecx                // Check if there are four additional pixels, at least
    jns         .LBlendSmallLoop

    // Handle the last 0-3 pixels (also used by the big unaligned loop)
.LBlendSmallRemaining:
    cmpl        $-4, %ecx               // Check if we are done
    je          .LBlendSmallExit
    sall        $2, %ecx                // Calculate offset for last pixels
    movslq      %ecx, %rcx
    addq        %rcx, %rdi

    lddqu       (%rax, %rdi), %xmm1     // Load last four source pixels (overlapping)
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    jz          .LBlendSmallExit

    // Handle mixed alphas (calculate and scale)
    movdqa      %xmm0, %xmm3
    pandn       %xmm1, %xmm3            // Filter out alpha and green components from source
    lddqu       (%rdx, %rdi), %xmm5     // Load last four destination pixels (overlapping)
    pmulhuw     %xmm8, %xmm3            // Scale alpha and green
    psllw       $8, %xmm1               // Filter out red and blue components from source
    pmulhuw     %xmm8, %xmm1            // Scale red and blue

    pshufhw     $0xF5, %xmm3, %xmm2     // Repeat source alpha for scaling of destination (high)
    psllw       $8, %xmm3               // Combine scaled source results
    pshuflw     $0xF5, %xmm2, %xmm2     // Repeat source alpha (low)
    movdqa      %xmm6, %xmm4            // Clone 256 constant
    por         %xmm3, %xmm1
    psubw       %xmm2, %xmm4            // Finalize alpha calculations
    movdqa      %xmm5, %xmm3            // Clone destination pixels

    psllw       $8, %xmm3               // Filter out red and blue components
    pmulhuw     %xmm4, %xmm3            // Scale red and blue
    movdqa      %xmm5, %xmm2            // Clone destination pixels
    psrlw       $8, %xmm2               // Filter out alpha and green components
    pmullw      %xmm4, %xmm2            // Scale alpha and green

    pblendvb    %xmm0, %xmm3, %xmm2     // Combine scaled destination results
    cmpl        $-8, %ecx               // Check how many pixels should be written
    paddb       %xmm2, %xmm1            // Add source and destination pixels together
    jb          .LBlendSmallPixelsLeft1
    ja          .LBlendSmallPixelsLeft3
    pblendw     $0xF0, %xmm1, %xmm5
    movdqu      %xmm5, (%rdx, %rdi)     // Store last two destination pixels
.LBlendSmallExit:
    ret

.LBlendSmallPixelsLeft1:
    pblendw     $0xC0, %xmm1, %xmm5
    movdqu      %xmm5, (%rdx, %rdi)     // Store last destination pixel
    ret

.LBlendSmallPixelsLeft3:
    pblendw     $0xFC, %xmm1, %xmm5
    movdqu      %xmm5, (%rdx, %rdi)     // Store last three destination pixels
    ret


    // Handle really small blits (0-3 pixels)
    // **************************************
.LBlendReallySmall:
    addl        $4, %ecx
    jle         .LBlendReallySmallExit
    pxor        %xmm1, %xmm1
    cmpl        $2, %ecx                // Check how many pixels should be read
    pinsrd      $0x0, (%rax), %xmm1     // Load one source pixel
    pinsrd      $0x0, (%rdx), %xmm5     // Load one destination pixel
    jb          .LBlendReallySmallCalc
    pinsrd      $0x1, 4(%rax), %xmm1    // Load second source pixel
    pinsrd      $0x1, 4(%rdx), %xmm5    // Load second destination pixel
    je          .LBlendReallySmallCalc
    pinsrd      $0x2, 8(%rax), %xmm1    // Load third source pixel
    pinsrd      $0x2, 8(%rdx), %xmm5    // Load third destination pixel

.LBlendReallySmallCalc:
    ptest       %xmm7, %xmm1            // Check if all alphas are opaque
    jz          .LBlendReallySmallExit  // If all alphas are zero, just store

    // Handle mixed alphas (calculate and scale)
    movdqa      %xmm0, %xmm3
    pandn       %xmm1, %xmm3            // Filter out alpha and green components from source
    pmulhuw     %xmm8, %xmm3            // Scale alpha and green
    psllw       $8, %xmm1               // Filter out red and blue components from source
    pmulhuw     %xmm8, %xmm1            // Scale red and blue

    pshufhw     $0xF5, %xmm3, %xmm2     // Repeat source alpha for scaling of destination (high)
    psllw       $8, %xmm3               // Combine scaled source results
    pshuflw     $0xF5, %xmm2, %xmm2     // Repeat source alpha (low)
    movdqa      %xmm6, %xmm4            // Clone 256 constant
    por         %xmm3, %xmm1
    psubw       %xmm2, %xmm4            // Finalize alpha calculations
    movdqa      %xmm5, %xmm3            // Clone destination pixels

    psllw       $8, %xmm5               // Filter out red and blue components
    pmulhuw     %xmm4, %xmm5            // Scale red and blue
    psrlw       $8, %xmm3               // Filter out alpha and green components
    pmullw      %xmm4, %xmm3            // Scale alpha and green

    pblendvb    %xmm0, %xmm5, %xmm3     // Combine scaled destination results
    cmpl        $2, %ecx                // Check how many pixels should be written
    paddb       %xmm3, %xmm1            // Add source and destination pixels together
    pextrd      $0x0, %xmm1, (%rdx)     // Store one destination pixel
    jb          .LBlendReallySmallExit
    pextrd      $0x1, %xmm1, 4(%rdx)    // Store second destination pixel
    je          .LBlendReallySmallExit
    pextrd      $0x2, %xmm1, 8(%rdx)    // Store third destination pixel
.LBlendReallySmallExit:
    ret

    // Handle bigger blit operations (16+ pixels)
    // ******************************************
    .p2align 4
.LBlendBigBlit:
    // Align destination?
    testl       $0xF, %edx
    lddqu       (%rax), %xmm1           // Pre-load four source pixels
    jz          .LBlendAligned

    movq        %rdx, %rdi              // Calculate alignment of destination pointer
    negq        %rdi
    andl        $0xF, %edi

    // Handle 1-3 pixels to align destination
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    jz          .LBlendAlignDone        // If all alphas are opaque, just skip

    // Handle global and pixel alphas (calculate and scale)
    movdqa      %xmm0, %xmm3
    pandn       %xmm1, %xmm3            // Filter out alpha and green components from source
    lddqu       (%rdx), %xmm5           // Load four destination pixels
    pmulhuw     %xmm8, %xmm3            // Scale alpha and green
    psllw       $8, %xmm1               // Filter out red and blue components from source
    pmulhuw     %xmm8, %xmm1            // Scale red and blue

    pshufhw     $0xF5, %xmm3, %xmm2     // Repeat source alpha for scaling of destination (high)
    psllw       $8, %xmm3               // Combine scaled source results
    pshuflw     $0xF5, %xmm2, %xmm2     // Repeat source alpha (low)
    movdqa      %xmm6, %xmm4            // Clone 256 constant
    por         %xmm3, %xmm1
    psubw       %xmm2, %xmm4            // Finalize alpha calculations
    movdqa      %xmm5, %xmm3            // Clone destination pixels

    psllw       $8, %xmm3               // Filter out red and blue components
    pmulhuw     %xmm4, %xmm3            // Scale red and blue
    movdqa      %xmm5, %xmm2            // Clone destination pixels
    psrlw       $8, %xmm2               // Filter out alpha and green components
    pmullw      %xmm4, %xmm2            // Scale alpha and green

    cmpl        $8, %edi                // Check how many pixels should be written
    pblendvb    %xmm0, %xmm3, %xmm2     // Combine scaled destination results
    paddb       %xmm2, %xmm1            // Add source and destination pixels together
    jb          .LBlendAlignPixelsLeft1
    ja          .LBlendAlignPixelsLeft3
    pblendw     $0x0F, %xmm1, %xmm5     // Blend two pixels
    jmp .LBlendAlignStorePixels

.LBlendAlignPixelsLeft1:
    pblendw     $0x03, %xmm1, %xmm5     // Blend one pixel
    jmp .LBlendAlignStorePixels

.LBlendAlignPixelsLeft3:
    pblendw     $0x3F, %xmm1, %xmm5     // Blend three pixels

.LBlendAlignStorePixels:
    movdqu      %xmm5, (%rdx)           // Store destination pixels

.LBlendAlignDone:
    addq        %rdi, %rax              // Adjust pointers and pixel count
    addq        %rdi, %rdx
    shrq        $2, %rdi
    lddqu       (%rax), %xmm1           // Pre-load new source pixels (after alignment)
    subl        %edi, %ecx

.LBlendAligned:                         // Destination is guaranteed to be 16 byte aligned
    xorq        %rdi, %rdi              // Reset offset to zero
    subl        $8, %ecx                // Decrease counter (Reserve four pixels for the cleanup)
    testl       $0xF, %eax              // Check alignment of source pointer
    jz          .LBlendAlignedLoop

    // Source not aligned to destination
    // *********************************
    .p2align 4
.LBlendUnalignedLoop:                   // Main loop for unaligned, handles eight pixels per iteration
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    jz          .LBlendAlphaZero00

    // Handle global and pixel alphas (calculate and scale)
.LBlendAlphaNotZero00:
    movdqa      %xmm0, %xmm3
    pandn       %xmm1, %xmm3            // Filter out alpha and green components from source
    movdqa      (%rdx, %rdi), %xmm5     // Load four destination pixels
    pmulhuw     %xmm8, %xmm3            // Scale alpha and green
    psllw       $8, %xmm1               // Filter out red and blue components from source
    pmulhuw     %xmm8, %xmm1            // Scale red and blue

    pshufhw     $0xF5, %xmm3, %xmm2     // Repeat source alpha for scaling of destination (high)
    psllw       $8, %xmm3               // Combine scaled source results
    pshuflw     $0xF5, %xmm2, %xmm2     // Repeat source alpha (low)
    movdqa      %xmm6, %xmm4            // Clone 256 constant
    por         %xmm3, %xmm1
    psubw       %xmm2, %xmm4            // Finalize alpha calculations
    movdqa      %xmm5, %xmm3            // Clone destination pixels

    psllw       $8, %xmm5               // Filter out red and blue components
    pmulhuw     %xmm4, %xmm5            // Scale red and blue
    psrlw       $8, %xmm3               // Filter out alpha and green components
    pmullw      %xmm4, %xmm3            // Scale alpha and green

    lddqu       16(%rax, %rdi), %xmm2   // Pre-load four source pixels
    pblendvb    %xmm0, %xmm5, %xmm3     // Combine scaled destination results
    paddb       %xmm3, %xmm1            // Add source and destination pixels together
    movdqa      %xmm1, (%rdx, %rdi)     // Store four destination pixels

    // Handle next four pixels
    ptest       %xmm7, %xmm2            // Check if all alphas are zero or opaque
    jz         .LBlendAlphaZero01

    // Handle global and pixel alphas (calculate and scale)
.LBlendAlphaNotZero01:
    movdqa      %xmm0, %xmm3
    pandn       %xmm2, %xmm3            // Filter out alpha and green components from source
    movdqa      16(%rdx, %rdi), %xmm5   // Load four destination pixels
    pmulhuw     %xmm8, %xmm3            // Scale alpha and green
    psllw       $8, %xmm2               // Filter out red and blue components from source
    pmulhuw     %xmm8, %xmm2            // Scale red and blue

    pshufhw     $0xF5, %xmm3, %xmm1     // Repeat source alpha for scaling of destination (high)
    psllw       $8, %xmm3               // Combine scaled source results
    pshuflw     $0xF5, %xmm1, %xmm1     // Repeat source alpha (low)
    movdqa      %xmm6, %xmm4            // Clone 256 constant
    por         %xmm3, %xmm2
    psubw       %xmm1, %xmm4            // Finalize alpha calculations
    movdqa      %xmm5, %xmm3            // Clone destination pixels

    psllw       $8, %xmm5               // Filter out red and blue components
    pmulhuw     %xmm4, %xmm5            // Scale red and blue
    psrlw       $8, %xmm3               // Filter out alpha and green components
    pmullw      %xmm4, %xmm3            // Scale alpha and green

    lddqu       32(%rax, %rdi), %xmm1   // Pre-load four source pixels
    addq        $32, %rdi
    pblendvb    %xmm0, %xmm5, %xmm3     // Combine scaled destination results
    paddb       %xmm3, %xmm2            // Add source and destination pixels together
    subl        $8, %ecx
    movdqa      %xmm2, -16(%rdx, %rdi)  // Store four destination pixels
    jae         .LBlendUnalignedLoop
    addl        $8, %ecx                // Adjust pixel count
    jmp         .LBlendLoopCleanup0

    .p2align 4
.LBlendUnalignedLoopZero:               // Alternate 'alpha zero' main loop for unaligned
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    jnz         .LBlendAlphaNotZero00
.LBlendAlphaZero00:
    lddqu       16(%rax, %rdi), %xmm2   // Pre-load four source pixels
    ptest       %xmm7, %xmm2            // Check if all alphas are zero or opaque
    jnz         .LBlendAlphaNotZero01
.LBlendAlphaZero01:
    lddqu       32(%rax, %rdi), %xmm1   // Pre-load four source pixels
    addq        $32, %rdi               // Adjust offset and pixel count
    subl        $8, %ecx
    jae         .LBlendUnalignedLoopZero
    addl        $8, %ecx                // Adjust pixel count

    // Cleanup - handle pending pixels from loop
.LBlendLoopCleanup0:
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    jz          .LBlendAlphaZero03

    // Handle global and pixel alphas (calculate and scale)
    movdqa      %xmm0, %xmm3
    pandn       %xmm1, %xmm3            // Filter out alpha and green components from source
    movdqa      (%rdx, %rdi), %xmm5     // Load four destination pixels
    pmulhuw     %xmm8, %xmm3            // Scale alpha and green
    psllw       $8, %xmm1               // Filter out red and blue components from source
    pmulhuw     %xmm8, %xmm1            // Scale red and blue

    pshufhw     $0xF5, %xmm3, %xmm2     // Repeat source alpha for scaling of destination (high)
    psllw       $8, %xmm3               // Combine scaled source results
    pshuflw     $0xF5, %xmm2, %xmm2     // Repeat source alpha (low)
    movdqa      %xmm6, %xmm4            // Clone 256 constant
    por         %xmm3, %xmm1
    psubw       %xmm2, %xmm4            // Finalize alpha calculations
    movdqa      %xmm5, %xmm3            // Clone destination pixels

    psllw       $8, %xmm5               // Filter out red and blue components
    pmulhuw     %xmm4, %xmm5            // Scale red and blue
    psrlw       $8, %xmm3               // Filter out alpha and green components
    pmullw      %xmm4, %xmm3            // Scale alpha and green

    pblendvb    %xmm0, %xmm5, %xmm3     // Combine scaled destination results
    paddb       %xmm3, %xmm1            // Add source and destination pixels together
    movdqa      %xmm1, (%rdx, %rdi)     // Store four destination pixels

.LBlendAlphaZero03:
    addq        $16, %rdi
    subl        $4, %ecx
    js          .LBlendSmallRemaining   // Reuse code from small loop
    lddqu       (%rax, %rdi), %xmm1     // Pre-load four source pixels
    jmp         .LBlendLoopCleanup0

    // Source aligned to destination
    // *****************************
    .p2align 4
.LBlendAlignedLoop:                     // Main loop for aligned, handles eight pixels per iteration
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    jz          .LBlendAlphaZero10

    // Handle global and pixel alphas (calculate and scale)
.LBlendAlphaNotZero10:
    movdqa      %xmm0, %xmm3
    pandn       %xmm1, %xmm3            // Filter out alpha and green components from source
    movdqa      (%rdx, %rdi), %xmm5     // Load four destination pixels
    pmulhuw     %xmm8, %xmm3            // Scale alpha and green
    psllw       $8, %xmm1               // Filter out red and blue components from source
    pmulhuw     %xmm8, %xmm1            // Scale red and blue

    pshufhw     $0xF5, %xmm3, %xmm2     // Repeat source alpha for scaling of destination (high)
    psllw       $8, %xmm3               // Combine scaled source results
    pshuflw     $0xF5, %xmm2, %xmm2     // Repeat source alpha (low)
    movdqa      %xmm6, %xmm4            // Clone 256 constant
    por         %xmm3, %xmm1
    psubw       %xmm2, %xmm4            // Finalize alpha calculations
    movdqa      %xmm5, %xmm3            // Clone destination pixels

    psllw       $8, %xmm5               // Filter out red and blue components
    pmulhuw     %xmm4, %xmm5            // Scale red and blue
    psrlw       $8, %xmm3               // Filter out alpha and green components
    pmullw      %xmm4, %xmm3            // Scale alpha and green

    movdqa      16(%rax, %rdi), %xmm2   // Pre-load four source pixels
    pblendvb    %xmm0, %xmm5, %xmm3     // Combine scaled destination results
    paddb       %xmm3, %xmm1            // Add source and destination pixels together
    movdqa      %xmm1, (%rdx, %rdi)     // Store four destination pixels

    // Handle next four pixels
    ptest       %xmm7, %xmm2            // Check if all alphas are zero or opaque
    jz         .LBlendAlphaZero11

    // Handle global and pixel alphas (calculate and scale)
.LBlendAlphaNotZero11:
    movdqa      %xmm0, %xmm3
    pandn       %xmm2, %xmm3            // Filter out alpha and green components from source
    movdqa      16(%rdx, %rdi), %xmm5   // Load four destination pixels
    pmulhuw     %xmm8, %xmm3            // Scale alpha and green
    psllw       $8, %xmm2               // Filter out red and blue components from source
    pmulhuw     %xmm8, %xmm2            // Scale red and blue

    pshufhw     $0xF5, %xmm3, %xmm1     // Repeat source alpha for scaling of destination (high)
    psllw       $8, %xmm3               // Combine scaled source results
    pshuflw     $0xF5, %xmm1, %xmm1     // Repeat source alpha (low)
    movdqa      %xmm6, %xmm4            // Clone 256 constant
    por         %xmm3, %xmm2
    psubw       %xmm1, %xmm4            // Finalize alpha calculations
    movdqa      %xmm5, %xmm3            // Clone destination pixels

    psllw       $8, %xmm5               // Filter out red and blue components
    pmulhuw     %xmm4, %xmm5            // Scale red and blue
    psrlw       $8, %xmm3               // Filter out alpha and green components
    pmullw      %xmm4, %xmm3            // Scale alpha and green

    movdqa      32(%rax, %rdi), %xmm1   // Pre-load four source pixels
    addq        $32, %rdi
    pblendvb    %xmm0, %xmm5, %xmm3     // Combine scaled destination results
    paddb       %xmm3, %xmm2            // Add source and destination pixels together
    subl        $8, %ecx
    movdqa      %xmm2, -16(%rdx, %rdi)  // Store four destination pixels
    jae         .LBlendAlignedLoop
    jmp         .LBlendLoopCleanup1

    .p2align 4
.LBlendAlignedLoopZero:                 // Alternate 'alpha zero' main loop for unaligned
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    jnz         .LBlendAlphaNotZero10
.LBlendAlphaZero10:
    movdqa      16(%rax, %rdi), %xmm2   // Pre-load four source pixels
    ptest       %xmm7, %xmm2            // Check if all alphas are zero or opaque
    jnz         .LBlendAlphaNotZero11
.LBlendAlphaZero11:
    movdqa      32(%rax, %rdi), %xmm1   // Pre-load four source pixels
    addq        $32, %rdi               // Adjust offset and pixel count
    subl        $8, %ecx
    jae         .LBlendAlignedLoopZero

    // Cleanup - handle four pending pixels from loop
.LBlendLoopCleanup1:
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    jz          .LBlendAlphaZero13

    // Handle global and pixel alphas (calculate and scale)
    movdqa      %xmm0, %xmm3
    pandn       %xmm1, %xmm3            // Filter out alpha and green components from source
    movdqa      (%rdx, %rdi), %xmm5     // Load four destination pixels
    pmulhuw     %xmm8, %xmm3            // Scale alpha and green
    psllw       $8, %xmm1               // Filter out red and blue components from source
    pmulhuw     %xmm8, %xmm1            // Scale red and blue

    pshufhw     $0xF5, %xmm3, %xmm2     // Repeat source alpha for scaling of destination (high)
    psllw       $8, %xmm3               // Combine scaled source results
    pshuflw     $0xF5, %xmm2, %xmm2     // Repeat source alpha (low)
    movdqa      %xmm6, %xmm4            // Clone 256 constant
    por         %xmm3, %xmm1
    psubw       %xmm2, %xmm4            // Finalize alpha calculations
    movdqa      %xmm5, %xmm3            // Clone destination pixels

    psllw       $8, %xmm5               // Filter out red and blue components
    pmulhuw     %xmm4, %xmm5            // Scale red and blue
    psrlw       $8, %xmm3               // Filter out alpha and green components
    pmullw      %xmm4, %xmm3            // Scale alpha and green

    pblendvb    %xmm0, %xmm5, %xmm3     // Combine scaled destination results
    paddb       %xmm3, %xmm1            // Add source and destination pixels together
    movdqa      %xmm1, (%rdx, %rdi)     // Store four destination pixels

.LBlendAlphaZero13:
    addl        $8, %ecx                // Adjust offset and pixel count
    jz          .LBlendExit
    addq        $16, %rdi

    // Handle last 1-7 pixels
.LBlendRemainLoop1:
    movdqa      (%rax, %rdi), %xmm1     // Load four source pixels
    ptest       %xmm7, %xmm1            // Check if all alphas are zero or opaque
    jz          .LBlendRemainAlphaZero1

    // Handle global and pixel alphas (calculate and scale)
    movdqa      %xmm0, %xmm3
    pandn       %xmm1, %xmm3            // Filter out alpha and green components from source
    movdqa      (%rdx, %rdi), %xmm5     // Load four destination pixels
    pmulhuw     %xmm8, %xmm3            // Scale alpha and green
    psllw       $8, %xmm1               // Filter out red and blue components from source
    pmulhuw     %xmm8, %xmm1            // Scale red and blue

    pshufhw     $0xF5, %xmm3, %xmm2     // Repeat source alpha for scaling of destination (high)
    psllw       $8, %xmm3               // Combine scaled source results
    pshuflw     $0xF5, %xmm2, %xmm2     // Repeat source alpha (low)
    movdqa      %xmm6, %xmm4            // Clone 256 constant
    por         %xmm3, %xmm1
    psubw       %xmm2, %xmm4            // Finalize alpha calculations
    movdqa      %xmm5, %xmm3            // Clone destination pixels

    psllw       $8, %xmm5               // Filter out red and blue components
    pmulhuw     %xmm4, %xmm5            // Scale red and blue
    psrlw       $8, %xmm3               // Filter out alpha and green components
    pmullw      %xmm4, %xmm3            // Scale alpha and green

    subl        $4, %ecx
    pblendvb    %xmm0, %xmm5, %xmm3     // Combine scaled destination results
    paddb       %xmm3, %xmm1            // Add source and destination pixels together
    jle         .LBlendRemainStore
    movdqa      %xmm1, (%rdx, %rdi)     // Store four destination pixels
    addq        $16, %rdi
    jmp         .LBlendRemainLoop1

    // All alphas were zero (skip)
    .p2align 4
.LBlendRemainAlphaZero1:
    subl        $4, %ecx                // Check if we have more than four pixels left
    jle         .LBlendExit
    addq        $16, %rdi
    jmp         .LBlendRemainLoop1

    // Store the last 1-4 pixels
    .p2align 4
.LBlendRemainStore:
    jz          .LBlendRemainFull
    movdqa      (%rdx, %rdi), %xmm5     // Load four destination pixels
    cmpl        $-2, %ecx               // Check how many pixels should be written
    jb          .LBlendRemainPixelsLeft11
    ja          .LBlendRemainPixelsLeft13
    pblendw     $0x0F, %xmm1, %xmm5
    movdqa      %xmm5, (%rdx, %rdi)     // Store last 2 destination pixels
.LBlendExit:
    ret

.LBlendRemainPixelsLeft11:
    pblendw     $0x03, %xmm1, %xmm5
    movdqa      %xmm5, (%rdx, %rdi)     // Store last destination pixel
    ret

.LBlendRemainPixelsLeft13:
    pblendw     $0x3F, %xmm1, %xmm5
    movdqa      %xmm5, (%rdx, %rdi)     // Store last 3 destination pixels
    ret

.LBlendRemainFull:
    movdqa      %xmm1, (%rdx, %rdi)     // Store last 4 destination pixels
    ret

    .cfi_endproc
    .size S32A_Blend_BlitRow32_SSE4_asm, .-S32A_Blend_BlitRow32_SSE4_asm
#endif
