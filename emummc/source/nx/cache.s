/**
 * @file cache.s
 * @copyright libnx Authors
 */

.macro CODE_BEGIN name
	.section .text.\name, "ax", %progbits
	.global \name
	.type \name, %function
	.align 2
	.cfi_startproc
\name:
.endm

.macro CODE_END
	.cfi_endproc
.endm

CODE_BEGIN armDCacheFlush
	add x1, x1, x0
	mrs x8, CTR_EL0
	lsr x8, x8, #16
	and x8, x8, #0xf
	mov x9, #4
	lsl x9, x9, x8
	sub x10, x9, #1
	bic x8, x0, x10
	mov x10, x1

armDCacheFlush_L0:
	dc  civac, x8
	add x8, x8, x9
	cmp x8, x10
	bcc armDCacheFlush_L0

	dsb sy
	ret
CODE_END

CODE_BEGIN armDCacheClean
	add x1, x1, x0
	mrs x8, CTR_EL0
	lsr x8, x8, #16
	and x8, x8, #0xf
	mov x9, #4
	lsl x9, x9, x8
	sub x10, x9, #1
	bic x8, x0, x10
	mov x10, x1

armDCacheClean_L0:
	dc  cvac, x8
	add x8, x8, x9
	cmp x8, x10
	bcc armDCacheClean_L0

	dsb sy
	ret
CODE_END

CODE_BEGIN armICacheInvalidate
	add x1, x1, x0
	mrs x8, CTR_EL0
	and x8, x8, #0xf
	mov x9, #4
	lsl x9, x9, x8
	sub x10, x9, #1
	bic x8, x0, x10
	mov x10, x1

armICacheInvalidate_L0:
	ic  ivau, x8
	add x8, x8, x9
	cmp x8, x10
	bcc armICacheInvalidate_L0

	dsb sy
	ret
CODE_END

CODE_BEGIN armDCacheZero
	add x1, x1, x0
	mrs x8, CTR_EL0
	lsr x8, x8, #16
	and x8, x8, #0xf
	mov x9, #4
	lsl x9, x9, x8
	sub x10, x9, #1
	bic x8, x0, x10
	mov x10, x1

armDCacheZero_L0:
	dc  zva, x8
	add x8, x8, x9
	cmp x8, x10
	bcc armDCacheZero_L0

	dsb sy
	ret
CODE_END
