/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2014, STMicroelectronics International N.V.
 */

/*
 * signed ret_idivmod_values(signed quot, signed rem);
 * return quotient and remaining the EABI way (regs r0,r1)
 */
.section    .text.ret_idivmod_values, "ax", %progbits
.globl ret_idivmod_values
.align      0
.syntax unified
ret_idivmod_values:
    bx lr
.type ret_idivmod_values, %function
.size ret_idivmod_values, .-ret_idivmod_values

/*
 * unsigned ret_uidivmod_values(unsigned quot, unsigned rem);
 * return quotient and remaining the EABI way (regs r0,r1)
 */
.section    .text.ret_uidivmod_values, "ax", %progbits
.globl ret_uidivmod_values
.align      0
.syntax unified
ret_uidivmod_values:
    bx lr
.type ret_uidivmod_values, %function
.size ret_uidivmod_values, .-ret_uidivmod_values
