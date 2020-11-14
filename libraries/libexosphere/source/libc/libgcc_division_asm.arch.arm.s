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

/*
 * __value_in_regs lldiv_t __aeabi_ldivmod( long long n, long long d)
 */
.section    .text.__aeabi_ldivmod, "ax", %progbits
.globl __aeabi_ldivmod
.align      0
.syntax unified
__aeabi_ldivmod:
    push    {ip, lr}
    push    {r0-r3}
    mov     r0, sp
	bl	    __l_divmod
	pop	{r0-r3}
	pop	{ip, pc}

.type __aeabi_ldivmod, %function
.size __aeabi_ldivmod, .-__aeabi_ldivmod

/*
 * __value_in_regs ulldiv_t __aeabi_uldivmod(
 *		unsigned long long n, unsigned long long d)
 */
.section    .text.__aeabi_uldivmod , "ax", %progbits
.globl __aeabi_uldivmod
.align      0
.syntax unified
__aeabi_uldivmod :
    push    {ip, lr}
    push    {r0-r3}
    mov     r0, sp
	bl	    __ul_divmod
	pop	{r0-r3}
	pop	{ip, pc}

.type __aeabi_uldivmod, %function
.size __aeabi_uldivmod, .-__aeabi_uldivmod
