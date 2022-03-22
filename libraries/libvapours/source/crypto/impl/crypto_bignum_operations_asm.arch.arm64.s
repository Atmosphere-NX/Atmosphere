/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* ams::crypto::impl::BigNum::Add(Word *dst, const Word *lhs, const Word *rhs, size_t num_words) */
.section    .text._ZN3ams6crypto4impl6BigNum3AddEPjPKjS5_m, "ax", %progbits
.global     _ZN3ams6crypto4impl6BigNum3AddEPjPKjS5_m
.type       _ZN3ams6crypto4impl6BigNum3AddEPjPKjS5_m, %function
.balign 0x10
_ZN3ams6crypto4impl6BigNum3AddEPjPKjPKjm:
    /* Check if we have anything to do at all. */
    msr     nzcv, xzr
    cbz     x3, 7f

    /* Save registers. */
    stp     x16, x17, [sp, #-16]!
    stp     xzr, x19, [sp, #-16]!
    stp     x20, x21, [sp, #-16]!

    /* Check if we have less than 16 words to process. */
    lsr     x20, x3, #4
    cbz     x20, 2f

    sub     x3, x3, x20, lsl #4

1:  /* Process 16 words at a time. */
    /* NOTE: Nintendo uses X18 here, we will use X21 for EL1+ compat. */
    ldp      x4,  x5, [x1], #16
    ldp     x12, x13, [x2], #16
    ldp      x6,  x7, [x1], #16
    ldp     x14, x15, [x2], #16
    ldp      x8,  x9, [x1], #16
    ldp     x16, x17, [x2], #16
    ldp     x10, x11, [x1], #16
    ldp     x21, x19, [x2], #16

    adcs    x4, x4, x12
    adcs    x5, x5, x13
    stp     x4, x5, [x0], #16

    adcs    x6, x6, x14
    adcs    x7, x7, x15
    stp     x6, x7, [x0], #16

    adcs    x8, x8, x16
    adcs    x9, x9, x17
    stp     x8, x9, [x0], #16

    adcs    x10, x10, x21
    adcs    x11, x11, x19
    stp     x10, x11, [x0], #16

    sub     x20, x20, #1
    cbnz    x20, 1b

2:  /* We have less than 16 words to process. */
    lsr     x15, x3, #2
    cbz     x15, 4f

    sub     x3, x3, x15, lsl #2

3:  /* Process 4 words at a time. */
    ldp     x4, x5, [x1], #16
    ldp     x8, x9, [x2], #16

    sub     x15, x15, #1

    adcs    x4, x4, x8
    adcs    x5, x5, x9

    stp     x4, x5, [x0], #16

    cbnz    x15, 3b

4:  /* We have less than 4 words to process. */
    cbz     x3, 6f

5:  /* Process 1 word at a time. */
    ldr     w4, [x1], #4
    ldr     w8, [x2], #4
    adcs    w4, w4, w8
    str     w4, [x0], #4

    sub     x3, x3, #1
    cbnz    x3, 5b

6:  /* Restore registers we used while adding. */
    ldp     x20, x21, [sp], #16
    ldp     xzr, x19, [sp], #16
    ldp     x16, x17, [sp], #16

7:  /* We're done. */
    adc     x0, xzr, xzr
    ret

/* ams::crypto::impl::BigNum::Sub(Word *dst, const Word *lhs, const Word *rhs, size_t num_words) */
.section    .text._ZN3ams6crypto4impl6BigNum3SubEPjPKjS5_m, "ax", %progbits
.global     _ZN3ams6crypto4impl6BigNum3SubEPjPKjS5_m
.type       _ZN3ams6crypto4impl6BigNum3SubEPjPKjS5_m, %function
.balign 0x10
_ZN3ams6crypto4impl6BigNum3SubEPjPKjS5_m:
    /* Check if we have anything to do at all. */
    mov     x4, #0x20000000
    msr     nzcv, x4
    cbz     x3, 7f

    /* Save registers. */
    stp     x16, x17, [sp, #-16]!
    stp     xzr, x19, [sp, #-16]!
    stp     x20, x21, [sp, #-16]!

    /* Check if we have less than 16 words to process. */
    lsr     x20, x3, #4
    cbz     x20, 2f

    sub     x3, x3, x20, lsl #4

1:  /* Process 16 words at a time. */
    /* NOTE: Nintendo uses X18 here, we will use X21 for EL1+ compat. */
    ldp      x4,  x5, [x1], #16
    ldp     x12, x13, [x2], #16
    ldp      x6,  x7, [x1], #16
    ldp     x14, x15, [x2], #16
    ldp      x8,  x9, [x1], #16
    ldp     x16, x17, [x2], #16
    ldp     x10, x11, [x1], #16
    ldp     x21, x19, [x2], #16

    sbcs    x4, x4, x12
    sbcs    x5, x5, x13
    stp     x4, x5, [x0], #16

    sbcs    x6, x6, x14
    sbcs    x7, x7, x15
    stp     x6, x7, [x0], #16

    sbcs    x8, x8, x16
    sbcs    x9, x9, x17
    stp     x8, x9, [x0], #16

    sbcs    x10, x10, x21
    sbcs    x11, x11, x19
    stp     x10, x11, [x0], #16

    sub     x20, x20, #1
    cbnz    x20, 1b

2:  /* We have less than 16 words to process. */
    lsr     x15, x3, #2
    cbz     x15, 4f

    sub     x3, x3, x15, lsl #2

3:  /* Process 4 words at a time. */
    ldp     x4, x5, [x1], #16
    ldp     x8, x9, [x2], #16

    sub     x15, x15, #1

    sbcs    x4, x4, x8
    sbcs    x5, x5, x9

    stp     x4, x5, [x0], #16

    cbnz    x15, 3b

4:  /* We have less than 4 words to process. */
    cbz     x3, 6f

5:  /* Process 1 word at a time. */
    ldr     w4, [x1], #4
    ldr     w8, [x2], #4
    sbcs    w4, w4, w8
    str     w4, [x0], #4

    sub     x3, x3, #1
    cbnz    x3, 5b

6:  /* Restore registers we used while adding. */
    ldp     x20, x21, [sp], #16
    ldp     xzr, x19, [sp], #16
    ldp     x16, x17, [sp], #16

7:  /* We're done. */
    cinc    x0, xzr, cc
    ret

/* ams::crypto::impl::BigNum::MultAdd(Word *dst, const Word *w, size_t num_words, Word mult) */
.section    .text._ZN3ams6crypto4impl6BigNum7MultAddEPjPKjmj, "ax", %progbits
.global     _ZN3ams6crypto4impl6BigNum7MultAddEPjPKjmj
.type       _ZN3ams6crypto4impl6BigNum7MultAddEPjPKjmj, %function
.balign 0x10
_ZN3ams6crypto4impl6BigNum7MultAddEPjPKjmj:
    /* Check if we have anything to do at all. */
    mov     x15, xzr
    cbz     x2, 5f

    /* Check if we have less than four words to process. */
    lsr     x6, x2, #2
    cbz     x6, 2f

    /* We have more than four words to process. */
    sub     x2, x2, x6, lsl #2
    stp     x16, x17, [sp, #-16]!

1:  /* Loop processing four words at a time. */
    ldp     w4, w5, [x1], #8
    ldp     w16, w7, [x1], #8
    ldp     w8, w9, [x0]
    ldp     w10, w11, [x0, #8]

    umaddl  x4,  w3, w4,  x8
    umaddl  x5,  w3, w5,  x9
    umaddl  x16, w3, w16, x10
    umaddl  x7,  w3, w7,  x11

    add     x12, x4, x15, lsr #32
    add     x13, x5, x12, lsr #32
    stp     w12, w13, [x0], #8

    add     x14, x16, x13, lsr #32
    add     x15, x7, x14, lsr #32
    stp     w14, w15, [x0], #8

    sub     x6, x6, #1
    cbnz    x6, 1b

    ldp     x16, x17, [sp], #16

2:  /* We have less than four words. Check if we have less than two. */
    lsr     x6, x2, #1
    cbz     x6, 4f

    /* We have more than two words to process. */
    sub     x2, x2, x6, lsl #1

3:  /* Loop processing two words at a time. */
    ldp     w4, w5, [x1], #8
    ldp     w8, w9, [x0]

    umaddl  x4, w3, w4, x8
    umaddl  x5, w3, w5, x9

    sub     x6, x6, #1

    add     x14, x4, x15, lsr #32
    add     x15, x5, x14, lsr #32

    stp     w14, w15, [x0], #8

    cbnz    x6, 3b

4:  /* We have less than two words to process. */
    cbz     x2, 5f

    /* We have one word to process. */
    ldr     w4, [x1], #4
    ldr     w8, [x0]

    umaddl  x4, w3, w4, x8
    add     x15, x4, x15, lsr #32

    str     w15, [x0], #4

5:  /* We're done. */
    lsr     x0, x15, #32
    ret
