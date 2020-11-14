/*
 * memset - fill memory with a constant byte
 *
 * Copyright (c) 2012-2020, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

/* Assumptions:
 *
 * ARMv8-a, AArch64, Advanced SIMD, unaligned accesses.
 *
 */

#include "asmdefs.h"

#define DC_ZVA_THRESHOLD 512

#define dstin	x0
#define val	    x1
#define valw	w1
#define count	x2
#define dst	    x3
#define dstend	x4
#define zva_val	x5

ENTRY (memset)

    bfi     valw, valw,  8,  8
    bfi     valw, valw, 16, 16
    bfi     val,   val, 32, 32

	add	    dstend, dstin, count

	cmp 	count, 96
	b.hi	L(set_long)
	cmp	    count, 16
	b.hs	L(set_medium)

	/* Set 0..15 bytes.  */
	tbz	    count, 3, 1f
	str	    val, [dstin]
	str	    val, [dstend, -8]
    ret
1:	tbz	    count, 2, 2f
	str	    valw, [dstin]
	str	    valw, [dstend, -4]
    ret
2:	cbz	    count, 3f
	strb	valw, [dstin]
	tbz	    count, 1, 3f
	strh	valw, [dstend, -2]
3:	ret

	/* Set 16..96 bytes.  */
    .p2align 4
L(set_medium):
    stp     val, val, [dstin]
	tbnz	count, 6, L(set96)
	stp	    val, val, [dstend, -16]
	tbz	    count, 5, 1f
	stp	    val, val, [dstin, 16]
	stp	    val, val, [dstend, -32]
1:	ret

	.p2align 4
	/* Set 64..96 bytes.  Write 64 bytes from the start and
	   32 bytes from the end.  */
L(set96):
	stp	    val, val, [dstin, 16]
	stp	    val, val, [dstin, 32]
	stp	    val, val, [dstin, 48]
	stp	    val, val, [dstend, -32]
	stp	    val, val, [dstend, -16]
    ret

	.p2align 4
L(set_long):
	stp	    val, val, [dstin]
#if DC_ZVA_THRESHOLD
	cmp	    count, DC_ZVA_THRESHOLD
	ccmp	val, 0, 0, cs
	bic	    dst, dstin, 15
	b.eq	L(zva_64)
#else
	bic	    dst, dstin, 15
#endif
	/* Small-size or non-zero memset does not use DC ZVA. */
	sub	    count, dstend, dst

	/*
	 * Adjust count and bias for loop. By substracting extra 1 from count,
	 * it is easy to use tbz instruction to check whether loop tailing
	 * count is less than 33 bytes, so as to bypass 2 unneccesary stps.
	 */
	sub	    count, count, 64+16+1

#if DC_ZVA_THRESHOLD
	/* Align loop on 16-byte boundary, this might be friendly to i-cache. */
	nop
#endif

1:	stp	    val, val, [dst, 16]
	stp	    val, val, [dst, 32]
	stp	    val, val, [dst, 48]
	stp	    val, val, [dst, 64]!
	subs	count, count, 64
	b.hs	1b

	tbz	    count, 5, 1f	/* Remaining count is less than 33 bytes? */
	stp	    val, val, [dst, 16]
	stp	    val, val, [dst, 32]
1:	stp	    val, val, [dstend, -32]
	stp	    val, val, [dstend, -16]
	ret

#if DC_ZVA_THRESHOLD
	.p2align 4
L(zva_64):
	stp	    val, val, [dst, 16]
	stp	    val, val, [dst, 32]
	stp	    val, val, [dst, 48]
	bic	    dst, dst, 63

	/*
	 * Previous memory writes might cross cache line boundary, and cause
	 * cache line partially dirty. Zeroing this kind of cache line using
	 * DC ZVA will incur extra cost, for it requires loading untouched
	 * part of the line from memory before zeoring.
	 *
	 * So, write the first 64 byte aligned block using stp to force
	 * fully dirty cache line.
	 */
	stp	    val, val, [dst, 64]
	stp	    val, val, [dst, 80]
	stp	    val, val, [dst, 96]
	stp	    val, val, [dst, 112]

	sub	    count, dstend, dst
	/*
	 * Adjust count and bias for loop. By substracting extra 1 from count,
	 * it is easy to use tbz instruction to check whether loop tailing
	 * count is less than 33 bytes, so as to bypass 2 unneccesary stps.
	 */
	sub	    count, count, 128+64+64+1
	add	    dst, dst, 128
	nop

	/* DC ZVA sets 64 bytes each time. */
1:	dc	    zva, dst
	add	    dst, dst, 64
	subs	count, count, 64
	b.hs	1b

	/*
	 * Write the last 64 byte aligned block using stp to force fully
	 * dirty cache line.
	 */
	stp	    val, val, [dst, 0]
	stp	    val, val, [dst, 16]
	stp	    val, val, [dst, 32]
	stp	    val, val, [dst, 48]

	tbz	    count, 5, 1f	/* Remaining count is less than 33 bytes? */
	stp	    val, val, [dst, 64]
	stp	    val, val, [dst, 80]
1:	stp	    val, val, [dstend, -32]
	stp	    val, val, [dstend, -16]
	ret
#endif


END (memset)
