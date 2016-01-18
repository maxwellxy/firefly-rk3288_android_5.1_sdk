/*
 * Copyright 2014 The Android Open Source Project
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * void S32_opaque_D32_nofilter_DX_SSE2_asm(const uint32_t* xy,
 *                                        int count,
 *                                        const SkPMColor* srcAddr,
 *                                        uint32_t* colors);
 */

.globl S32_opaque_D32_nofilter_DX_SSE2_asm;
S32_opaque_D32_nofilter_DX_SSE2_asm:

    push %ebp
    mov  %esp, %ebp
    push %esi
    push %edi
    push %ebx
    sub  $0x8, %esp

    mov 0x08(%ebp), %esi           # xy
    mov 0x0c(%ebp), %ecx           # count
    mov 0x0c(%ebp), %ecx           # count
    mov 0x10(%ebp), %edx           # srcAddr
    mov 0x14(%ebp), %edi           # colors

    sarl $0x2, %ecx                # count / 4
    test %ecx, %ecx
    jle nofilter_done

    sall $0x3, %ecx                # terminal value for index
    movd %ecx, %xmm7

    xor %eax, %eax                 # index = 0

    # prepare for part 1
    movl (%esi,%eax,1), %ebx

    shr $0x4, %ecx                 # count / 8
    jc .Lpart1odd
    jmp .Lpart1even


    .align 16
.Lloop:
    # part 2
    movzx %cx, %ebx
    shr $0x10, %ecx
    movl (%edx,%ebx,4), %ebx
    movl (%edx,%ecx,4), %ecx
    movl %ebx, -0x8(%edi,%eax,2)
    movl (%esi,%eax,1), %ebx       # prepare for part 1
    movl %ecx, -0x4(%edi,%eax,2)

.Lpart1even:
    # part 1
    movzx %bx, %ecx
    shr $0x10, %ebx
    movl (%edx,%ecx,4), %ecx
    movl (%edx,%ebx,4), %ebx
    movl %ecx, (%edi,%eax,2)
    movl 0x4(%esi,%eax,1), %ecx    # prepare for part 2
    movl %ebx, 0x4(%edi,%eax,2)

    lea 0x8(%eax), %eax

    # part 2
    movzx %cx, %ebx
    shr $0x10, %ecx
    movl (%edx,%ebx,4), %ebx
    movl (%edx,%ecx,4), %ecx
    movl %ebx, -0x8(%edi,%eax,2)
    movl (%esi,%eax,1), %ebx       # prepare for part 1
    movl %ecx, -0x4(%edi,%eax,2)

.Lpart1odd:
    # part 1
    movzx %bx, %ecx
    shr $0x10, %ebx
    movl (%edx,%ecx,4), %ecx
    movl (%edx,%ebx,4), %ebx
    movl %ecx, (%edi,%eax,2)
    movl 0x4(%esi,%eax,1), %ecx    # prepare for part 2
    movl %ebx, 0x4(%edi,%eax,2)

    lea 0x8(%eax), %eax

    movd %xmm7, %ebx
    sub %eax, %ebx

    jnz .Lloop


    # part 2
    movzx %cx, %ebx
    shr $0x10, %ecx
    movl (%edx,%ebx,4), %ebx
    movl (%edx,%ecx,4), %ecx
    movl %ebx, -0x8(%edi,%eax,2)
    movl %ecx, -0x4(%edi,%eax,2)


nofilter_done:
    add  $0x8, %esp
    pop  %ebx
    pop  %edi
    pop  %esi
    pop  %ebp
    ret
