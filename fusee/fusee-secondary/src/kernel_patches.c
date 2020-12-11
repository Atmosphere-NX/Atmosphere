/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 * Copyright (c) 2019 m4xw <m4x@m4xw.net>
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

#include <string.h>
#include "utils.h"
#include "se.h"
#include "kernel_patches.h"
#include "ips.h"

#define u8 uint8_t
#define u32 uint32_t
#include "kernel_ldr_bin.h"
#undef u8
#undef u32

#define MAKE_BRANCH(a, o) 0x14000000 | ((((o) - (a)) >> 2) & 0x3FFFFFF)
#define MAKE_NOP 0xD503201F

#define MAKE_KERNEL_PATTERN_NAME(vers, name) g_kernel_pattern_##vers##_##name
#define MAKE_KERNEL_PATCH_NAME(vers, name) g_kernel_patch_##vers##_##name

typedef uint32_t instruction_t;

typedef struct {
    size_t pattern_size;
    const uint8_t *pattern;
    size_t pattern_hook_offset;
    size_t payload_num_instructions;
    size_t branch_back_offset;
    size_t patch_offset;
    const instruction_t *payload;
} kernel_patch_t;

typedef struct {
    uint8_t hash[0x20]; /* TODO: Come up with a better way to identify kernels, that doesn't rely on hashing them. */
    size_t hash_offset; /* Start hashing at offset N, if this is set. */
    size_t hash_size;   /* Only hash the first N bytes of the kernel, if this is set. */
    size_t embedded_ini_offset; /* 8.0.0+ embeds the INI in kernel section. */
    size_t embedded_ini_ptr;    /* 8.0.0+ embeds the INI in kernel section. */
    size_t free_code_space_offset;
    unsigned int num_patches;
    const kernel_patch_t *patches;
} kernel_info_t;

/* Patch definitions. */
/*
    stp x10, x11, [sp, #-0x10]!
    mov w11, w14
    lsl x11, x11, #2
    ldr x11, [x28, x11]
    mov x9, #0x0000ffffffffffff
    and x8, x11, x9
    mov x9, #0xffff000000000000
    and x11, x11, x9
    mov x9, #0xfffe000000000000
    cmp x11, x9
    beq #8
    ldr x8, [x10,#0x250]
    ldp x10, x11, [sp],#0x10
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(100, proc_id_send)[] = {0x48, 0x29, 0x41, 0xF9, 0xC9, 0xF5, 0x7E, 0xD3, 0xCE, 0x09, 0x00, 0x11, 0x48, 0x6A, 0x29, 0xF8};
static const instruction_t MAKE_KERNEL_PATCH_NAME(100, proc_id_send)[] = {0xA9BF2FEA, 0x2A0E03EB, 0xD37EF56B, 0xF86B6B8B, 0x92FFFFE9, 0x8A090168, 0xD2FFFFE9, 0x8A09016B, 0xD2FFFFC9, 0xEB09017F, 0x54000040, 0xF9412948, 0xA8C12FEA};
/*
    stp x10, x11, [sp, #-0x10]!
    mov w10, w28
    lsl x10, x10, #2
    ldr x10, [x13, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #8
    ldr x8, [x11,#0x250]
    ldp x10, x11, [sp],#0x10
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(100, proc_id_recv)[] = {0x68, 0x29, 0x41, 0xF9, 0x89, 0xF7, 0x7E, 0xD3, 0x9C, 0x0B, 0x00, 0x11, 0x48, 0x69, 0x29, 0xF8};
static const instruction_t MAKE_KERNEL_PATCH_NAME(100, proc_id_recv)[] = {0xA9BF2FEA, 0x2A1C03EA, 0xD37EF54A, 0xF86A69AA, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9412968, 0xA8C12FEA};
/*
    stp x10, x11, [sp, #-0x10]!
    mov w11, w24
    lsl x11, x11, #2
    ldr x11, [x28, x11]
    mov x9, #0x0000ffffffffffff
    and x8, x11, x9
    mov x9, #0xffff000000000000
    and x11, x11, x9
    mov x9, #0xfffe000000000000
    cmp x11, x9
    beq #8
    ldr x8, [x10,#0x260]
    ldp x10, x11, [sp],#0x10
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(200, proc_id_send)[] = {0x48, 0x31, 0x41, 0xF9, 0xE9, 0x03, 0x18, 0x2A, 0x29, 0xF5, 0x7E, 0xD3, 0xC8, 0x6A, 0x29, 0xF8};
static const instruction_t MAKE_KERNEL_PATCH_NAME(200, proc_id_send)[] = {0xA9BF2FEA, 0x2A1803EB, 0xD37EF56B, 0xF86B6B8B, 0x92FFFFE9, 0x8A090168, 0xD2FFFFE9, 0x8A09016B, 0xD2FFFFC9, 0xEB09017F, 0x54000040, 0xF9413148, 0xA8C12FEA};
/*
    stp x10, x11, [sp, #-0x10]!
    mov w10, w15
    lsl x10, x10, #2
    ldr x11, [sp, #0xB8]
    ldr x10, [x11, x10]
    ldr x11, [sp, #0xF0]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #8
    ldr x8, [x11,#0x260]
    ldp x10, x11, [sp],#0x10
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(200, proc_id_recv)[] = {0x08, 0x31, 0x41, 0xF9, 0xE9, 0x03, 0x0F, 0x2A, 0x29, 0xF5, 0x7E, 0xD3, 0x48, 0x6B, 0x29, 0xF8};
static const instruction_t MAKE_KERNEL_PATCH_NAME(200, proc_id_recv)[] = {0xA9BF2FEA, 0x2A0F03EA, 0xD37EF54A, 0xF9405FEB, 0xF86A696A, 0xF9407BEB, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9413168, 0xA8C12FEA};
/*
    stp x10, x11, [sp, #-0x10]!
    mov w11, w24
    lsl x11, x11, #2
    ldr x11, [x28, x11]
    mov x9, #0x0000ffffffffffff
    and x8, x11, x9
    mov x9, #0xffff000000000000
    and x11, x11, x9
    mov x9, #0xfffe000000000000
    cmp x11, x9
    beq #8
    ldr x8, [x10,#0x2A8]
    ldp x10, x11, [sp],#0x10
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(300, proc_id_send)[] = {0x48, 0x55, 0x41, 0xF9, 0xE9, 0x03, 0x18, 0x2A, 0x29, 0xF5, 0x7E, 0xD3, 0xC8, 0x6A, 0x29, 0xF8};
static const instruction_t MAKE_KERNEL_PATCH_NAME(300, proc_id_send)[] = {0xA9BF2FEA, 0x2A1803EB, 0xD37EF56B, 0xF86B6B8B, 0x92FFFFE9, 0x8A090168, 0xD2FFFFE9, 0x8A09016B, 0xD2FFFFC9, 0xEB09017F, 0x54000040, 0xF9415548, 0xA8C12FEA};
/*
    stp x10, x11, [sp, #-0x10]!
    mov w10, w15
    lsl x10, x10, #2
    ldr x11, [sp, #0xB8]
    ldr x10, [x11, x10]
    ldr x11, [sp, #0xF0]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #8
    ldr x8, [x11,#0x2A8]
    ldp x10, x11, [sp],#0x10
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(300, proc_id_recv)[] = {0x08, 0x55, 0x41, 0xF9, 0xE9, 0x03, 0x0F, 0x2A, 0x29, 0xF5, 0x7E, 0xD3, 0x48, 0x6B, 0x29, 0xF8};
static const instruction_t MAKE_KERNEL_PATCH_NAME(300, proc_id_recv)[] = {0xA9BF2FEA, 0x2A0F03EA, 0xD37EF54A, 0xF9405FEB, 0xF86A696A, 0xF9407BEB, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415568, 0xA8C12FEA};
/*
    stp x10, x11, [sp, #-0x10]!
    mov w11, w24
    lsl x11, x11, #2
    ldr x11, [x28, x11]
    mov x9, #0x0000ffffffffffff
    and x8, x11, x9
    mov x9, #0xffff000000000000
    and x11, x11, x9
    mov x9, #0xfffe000000000000
    cmp x11, x9
    beq #8
    ldr x8, [x10,#0x2A8]
    ldp x10, x11, [sp],#0x10
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(302, proc_id_send)[] = {0x48, 0x55, 0x41, 0xF9, 0xE9, 0x03, 0x18, 0x2A, 0x29, 0xF5, 0x7E, 0xD3, 0xC8, 0x6A, 0x29, 0xF8};
static const instruction_t MAKE_KERNEL_PATCH_NAME(302, proc_id_send)[] = {0xA9BF2FEA, 0x2A1803EB, 0xD37EF56B, 0xF86B6B8B, 0x92FFFFE9, 0x8A090168, 0xD2FFFFE9, 0x8A09016B, 0xD2FFFFC9, 0xEB09017F, 0x54000040, 0xF9415548, 0xA8C12FEA};
/*
    stp x10, x11, [sp, #-0x10]!
    mov w10, w15
    lsl x10, x10, #2
    ldr x11, [sp, #0xB8]
    ldr x10, [x11, x10]
    ldr x11, [sp, #0xF0]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #8
    ldr x8, [x11,#0x2A8]
    ldp x10, x11, [sp],#0x10
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(302, proc_id_recv)[] = {0x08, 0x55, 0x41, 0xF9, 0xE9, 0x03, 0x0F, 0x2A, 0x29, 0xF5, 0x7E, 0xD3, 0x48, 0x6B, 0x29, 0xF8};
static const instruction_t MAKE_KERNEL_PATCH_NAME(302, proc_id_recv)[] = {0xA9BF2FEA, 0x2A0F03EA, 0xD37EF54A, 0xF9405FEB, 0xF86A696A, 0xF9407BEB, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415568, 0xA8C12FEA};
/*
    mov w10, w23
    lsl x10, x10, #2
    ldr x10, [x28, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #12
    ldr x10, [sp,#0xa0]
    ldr x8, [x10,#0x2b0]
    ldr x10, [sp,#0xa0]
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(400, proc_id_send)[] = {0xEA, 0x53, 0x40, 0xF9, 0x48, 0x59, 0x41, 0xF9, 0xE9, 0x03, 0x17, 0x2A, 0x29, 0xF5, 0x7E, 0xD3};
static const instruction_t MAKE_KERNEL_PATCH_NAME(400, proc_id_send)[] = {0x2A1703EA, 0xD37EF54A, 0xF86A6B8A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000060, 0xF94053EA, 0xF9415948, 0xF94053EA};
/*
    ldr x13, [sp,#0x70]
    mov w10, w14
    lsl x10, x10, #2
    ldr x10, [x13, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #8
    ldr x8, [x25,#0x2b0]
    nop
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(400, proc_id_recv)[] = {0x28, 0x5B, 0x41, 0xF9, 0xE9, 0x03, 0x0E, 0x2A, 0xCE, 0x09, 0x00, 0x11, 0x29, 0xF5, 0x7E, 0xD3};
static const instruction_t MAKE_KERNEL_PATCH_NAME(400, proc_id_recv)[] = {0xF9403BED, 0x2A0E03EA, 0xD37EF54A, 0xF86A69AA, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415B28, 0xD503201F};
/*
    mov w10, w23
    lsl x10, x10, #2
    ldr x10, [x27, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #12
    ldr x10, [sp,#0x80]
    ldr x8, [x10,#0x2b0]
    ldr x10, [sp,#0x80]
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(500, proc_id_send)[] = {0xEA, 0x43, 0x40, 0xF9, 0x48, 0x59, 0x41, 0xF9, 0xE9, 0x03, 0x17, 0x2A, 0x29, 0xF5, 0x7E, 0xD3};
static const instruction_t MAKE_KERNEL_PATCH_NAME(500, proc_id_send)[] = {0x2A1703EA, 0xD37EF54A, 0xF86A6B6A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000060, 0xF94043EA, 0xF9415948, 0xF94043EA};
/*
    ldr x13, [sp, #0x70]
    mov w10, w21
    lsl x10, x10, #2
    ldr x10, [x13, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #8
    ldr x8, [x24,#0x2b0]
    ldr x10, [sp,#0xd8]
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(500, proc_id_recv)[] = {0x08, 0x5B, 0x41, 0xF9, 0xEA, 0x6F, 0x40, 0xF9, 0xE9, 0x03, 0x15, 0x2A, 0x29, 0xF5, 0x7E, 0xD3};
static const instruction_t MAKE_KERNEL_PATCH_NAME(500, proc_id_recv)[] = {0xF9403BED, 0x2A1503EA, 0xD37EF54A, 0xF86A69AA, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415B08, 0xF9406FEA};
/*
    stp x10, x11, [sp, #-0x10]!
    ldr x11, [sp, #0x68]
    mov w10, w21
    lsl x10, x10, #2
    ldr x10, [x11, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #0x20

    stp x8, x9, [sp, #-0x10]!
    ldr x8, [x24]
    ldr x8, [x8, #0x38]
    mov x0, x24
    blr x8
    ldp x8, x9, [sp],#0x10
    mov x8, x0

    ldp x10, x11, [sp],#0x10
    mov x0, x8
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(600, proc_id_send)[] = {0x08, 0x03, 0x40, 0xF9, 0x08, 0x1D, 0x40, 0xF9, 0xE0, 0x03, 0x18, 0xAA, 0x00, 0x01, 0x3F, 0xD6, 0xE8, 0x03, 0x15, 0x2A, 0xB5, 0x0A, 0x00, 0x11, 0x08, 0xF5, 0x7E, 0xD3};
static const instruction_t MAKE_KERNEL_PATCH_NAME(600, proc_id_send)[] = {0xA9BF2FEA, 0xF94037EB, 0x2A1503EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400308, 0xF9401D08, 0xAA1803E0, 0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0};
/*
    stp x10, x11, [sp, #-0x10]!
    ldr x11, [sp, #0x80]
    mov w10, w21
    lsl x10, x10, #2
    ldr x10, [x11, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #0x20

    stp x8, x9, [sp, #-0x10]!
    ldr x8, [x24]
    ldr x8, [x8, #0x38]
    mov x0, x24
    blr x8
    ldp x8, x9, [sp],#0x10
    mov x8, x0

    ldp x10, x11, [sp],#0x10
    mov x0, x8
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(600, proc_id_recv)[] = {0x08, 0x03, 0x40, 0xF9, 0x08, 0x1D, 0x40, 0xF9, 0xE0, 0x03, 0x18, 0xAA, 0x00, 0x01, 0x3F, 0xD6, 0xE9, 0x6F, 0x40, 0xF9, 0xE8, 0x03, 0x15, 0x2A, 0xB5, 0x0A, 0x00, 0x11};
static const instruction_t MAKE_KERNEL_PATCH_NAME(600, proc_id_recv)[] = {0xA9BF2FEA, 0xF94043EB, 0x2A1503EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400308, 0xF9401D08, 0xAA1803E0, 0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0};
/*
    stp x10, x11, [sp, #-0x10]!
    ldr x11, [sp, #0x70]
    mov w10, w25
    lsl x10, x10, #2
    ldr x10, [x11, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #0x20

    stp x8, x9, [sp, #-0x10]!
    ldr x8, [x21]
    ldr x8, [x8, #0x38]
    mov x0, x21
    blr x8
    ldp x8, x9, [sp],#0x10
    mov x8, x0

    ldp x10, x11, [sp],#0x10
    mov x0, x8
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(700, proc_id_send)[] = {0xA8, 0x02, 0x40, 0xF9, 0x08, 0x1D, 0x40, 0xF9, 0xE0, 0x03, 0x15, 0xAA, 0x00, 0x01, 0x3F, 0xD6, 0xE8, 0x03, 0x19, 0x2A, 0x39, 0x0B, 0x00, 0x11, 0x08, 0xF5, 0x7E, 0xD3};
static const instruction_t MAKE_KERNEL_PATCH_NAME(700, proc_id_send)[] = {0xA9BF2FEA, 0xF9403BEB, 0x2A1903EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF94002A8, 0xF9401D08, 0xAA1503E0, 0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0};
/*
    stp x10, x11, [sp, #-0x10]!
    ldr x11, [sp, #0x98]
    mov w10, w22
    lsl x10, x10, #2
    ldr x10, [x11, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #0x20

    stp x8, x9, [sp, #-0x10]!
    ldr x8, [x27]
    ldr x8, [x8, #0x38]
    mov x0, x27
    blr x8
    ldp x8, x9, [sp],#0x10
    mov x8, x0

    ldp x10, x11, [sp],#0x10
    mov x0, x8
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(700, proc_id_recv)[] = {0x68, 0x03, 0x40, 0xF9, 0x08, 0x1D, 0x40, 0xF9, 0xE0, 0x03, 0x1B, 0xAA, 0x00, 0x01, 0x3F, 0xD6, 0xA9, 0x83, 0x50, 0xF8, 0xE8, 0x03, 0x16, 0x2A, 0xD6, 0x0A, 0x00, 0x11};
static const instruction_t MAKE_KERNEL_PATCH_NAME(700, proc_id_recv)[] = {0xA9BF2FEA, 0xF9404FEB, 0x2A1603EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400368, 0xF9401D08, 0xAA1B03E0, 0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0};

/*
    stp x10, x11, [sp, #-0x10]!
    ldr x11, [sp, #0x70]
    mov w10, w25
    lsl x10, x10, #2
    ldr x10, [x11, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #0x20

    stp x8, x9, [sp, #-0x10]!
    ldr x8, [x21]
    ldr x8, [x8, #0x38]
    mov x0, x21
    blr x8
    ldp x8, x9, [sp],#0x10
    mov x8, x0

    ldp x10, x11, [sp],#0x10
    mov x0, x8
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(800, proc_id_send)[] = {0xA8, 0x02, 0x40, 0xF9, 0x08, 0x1D, 0x40, 0xF9, 0xE0, 0x03, 0x15, 0xAA, 0x00, 0x01, 0x3F, 0xD6, 0xE8, 0x03, 0x19, 0x2A, 0x39, 0x0B, 0x00, 0x11, 0x08, 0xF5, 0x7E, 0xD3};
static const instruction_t MAKE_KERNEL_PATCH_NAME(800, proc_id_send)[] = {0xA9BF2FEA, 0xF9403BEB, 0x2A1903EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF94002A8, 0xF9401D08, 0xAA1503E0, 0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0};
/*
    stp x10, x11, [sp, #-0x10]!
    ldr x11, [sp, #0x98]
    mov w10, w22
    lsl x10, x10, #2
    ldr x10, [x11, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #0x20

    stp x8, x9, [sp, #-0x10]!
    ldr x8, [x27]
    ldr x8, [x8, #0x38]
    mov x0, x27
    blr x8
    ldp x8, x9, [sp],#0x10
    mov x8, x0

    ldp x10, x11, [sp],#0x10
    mov x0, x8
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(800, proc_id_recv)[] = {0x68, 0x03, 0x40, 0xF9, 0x08, 0x1D, 0x40, 0xF9, 0xE0, 0x03, 0x1B, 0xAA, 0x00, 0x01, 0x3F, 0xD6, 0xA9, 0x83, 0x50, 0xF8, 0xE8, 0x03, 0x16, 0x2A, 0xD6, 0x0A, 0x00, 0x11};
static const instruction_t MAKE_KERNEL_PATCH_NAME(800, proc_id_recv)[] = {0xA9BF2FEA, 0xF9404FEB, 0x2A1603EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400368, 0xF9401D08, 0xAA1B03E0, 0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0};

/*
    stp x10, x11, [sp, #-0x10]!
    ldr x11, [sp, #0x68]
    mov w10, w22
    lsl x10, x10, #2
    ldr x10, [x11, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #0x20

    stp x8, x9, [sp, #-0x10]!
    ldr x8, [x23]
    ldr x8, [x8, #0x38]
    mov x0, x23
    blr x8
    ldp x8, x9, [sp],#0x10
    mov x8, x0

    ldp x10, x11, [sp],#0x10
    mov x0, x8
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(900, proc_id_send)[] = {0xE8, 0x02, 0x40, 0xF9, 0x08, 0x1D, 0x40, 0xF9, 0xE0, 0x03, 0x17, 0xAA, 0x00, 0x01, 0x3F, 0xD6, 0xE8, 0x03, 0x16, 0x2A, 0xD6, 0x0A, 0x00, 0x11, 0x08, 0xF5, 0x7E, 0xD3};
static const instruction_t MAKE_KERNEL_PATCH_NAME(900, proc_id_send)[] = {0xA9BF2FEA, 0xF94037EB, 0x2A1603EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF94002E8, 0xF9401D08, 0xAA1703E0, 0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0};
/*
    stp x10, x11, [sp, #-0x10]!
    ldr x11, [sp, #0x90]
    mov w10, w23
    lsl x10, x10, #2
    ldr x10, [x11, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #0x20

    stp x8, x9, [sp, #-0x10]!
    ldr x8, [x27]
    ldr x8, [x8, #0x38]
    mov x0, x27
    blr x8
    ldp x8, x9, [sp],#0x10
    mov x8, x0

    ldp x10, x11, [sp],#0x10
    mov x0, x8
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(900, proc_id_recv)[] = {0x68, 0x03, 0x40, 0xF9, 0x08, 0x1D, 0x40, 0xF9, 0xE0, 0x03, 0x1B, 0xAA, 0x00, 0x01, 0x3F, 0xD6, 0xE8, 0x03, 0x17, 0x2A, 0xF7, 0x0A, 0x00, 0x11, 0x08, 0xF5, 0x7E, 0xD3};
static const instruction_t MAKE_KERNEL_PATCH_NAME(900, proc_id_recv)[] = {0xA9BF2FEA, 0xF9404BEB, 0x2A1703EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400368, 0xF9401D08, 0xAA1B03E0, 0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0};

/*
    stp x10, x11, [sp, #-0x10]!
    ldr x11, [sp, #0xC0]
    mov w10, w22
    lsl x10, x10, #2
    ldr x10, [x11, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #0x20

    stp x8, x9, [sp, #-0x10]!
    ldr x8, [x23]
    ldr x8, [x8, #0x38]
    mov x0, x23
    blr x8
    ldp x8, x9, [sp],#0x10
    mov x8, x0

    ldp x10, x11, [sp],#0x10
    mov x0, x8
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(1000, proc_id_send)[] = {0xE8, 0x02, 0x40, 0xF9, 0x08, 0x1D, 0x40, 0xF9, 0xE0, 0x03, 0x17, 0xAA, 0x00, 0x01, 0x3F, 0xD6, 0x08, 0x4B, 0x36, 0x8B, 0x09, 0xFC, 0x60, 0xD3, 0x00, 0x25, 0x00, 0x29};
static const instruction_t MAKE_KERNEL_PATCH_NAME(1000, proc_id_send)[] = {0xA9BF2FEA, 0xF94063EB, 0x2A1603EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF94002E8, 0xF9401D08, 0xAA1703E0, 0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0};
/*
    stp x10, x11, [sp, #-0x10]!
    ldr x11, [sp, #0xC8]
    mov w10, w26
    lsl x10, x10, #2
    ldr x10, [x11, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #0x20

    stp x8, x9, [sp, #-0x10]!
    ldr x8, [x28]
    ldr x8, [x8, #0x38]
    mov x0, x28
    blr x8
    ldp x8, x9, [sp],#0x10
    mov x8, x0

    ldp x10, x11, [sp],#0x10
    mov x0, x8
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(1000, proc_id_recv)[] = {0x88, 0x03, 0x40, 0xF9, 0x08, 0x1D, 0x40, 0xF9, 0xE0, 0x03, 0x1C, 0xAA, 0x00, 0x01, 0x3F, 0xD6, 0xE8, 0x87, 0x40, 0xF9, 0x08, 0x49, 0x3A, 0x8B, 0x09, 0xFC, 0x60, 0xD3};
static const instruction_t MAKE_KERNEL_PATCH_NAME(1000, proc_id_recv)[] = {0xA9BF2FEA, 0xF94067EB, 0x2A1A03EA, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400388, 0xF9401D08, 0xAA1C03E0, 0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0};

/*
    stp x10, x11, [sp, #-0x10]!
    ldr x11, [sp, #0x80]
    mov w10, #3
    lsl x10, x10, #2
    ldr x10, [x11, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #0x20

    stp x8, x9, [sp, #-0x10]!
    ldr x8, [x21]
    ldr x8, [x8, #0x38]
    mov x0, x21
    blr x8
    ldp x8, x9, [sp],#0x10
    mov x8, x0

    ldp x10, x11, [sp],#0x10
    mov x0, x8
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(1100, proc_id_send)[] = {0xE0, 0x03, 0x15, 0xAA, 0xA8, 0x02, 0x40, 0xF9, 0x08, 0x1D, 0x40, 0xF9, 0x00, 0x01, 0x3F, 0xD6, 0x88, 0x4A, 0x3C, 0x8B, 0x09, 0xFC, 0x60, 0xD3, 0x00, 0x25, 0x00, 0x29};
static const instruction_t MAKE_KERNEL_PATCH_NAME(1100, proc_id_send)[] = {0xA9BF2FEA, 0xF94043EB, 0x5280006A, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF94002A8, 0xF9401D08, 0xAA1503E0, 0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0};
/*
    stp x10, x11, [sp, #-0x10]!
    ldr x11, [sp, #0xE0]
    mov w10, #3
    lsl x10, x10, #2
    ldr x10, [x11, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #0x20

    stp x8, x9, [sp, #-0x10]!
    ldr x8, [x24]
    ldr x8, [x8, #0x38]
    mov x0, x24
    blr x8
    ldp x8, x9, [sp],#0x10
    mov x8, x0

    ldp x10, x11, [sp],#0x10
    mov x0, x8
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(1100, proc_id_recv)[] = {0x08, 0x03, 0x40, 0xF9, 0xE0, 0x03, 0x18, 0xAA, 0x08, 0x1D, 0x40, 0xF9, 0x00, 0x01, 0x3F, 0xD6, 0xE8, 0x7F, 0x40, 0xF9, 0x09, 0xFC, 0x60, 0xD3, 0xEA, 0x5B, 0x40, 0xF9};
static const instruction_t MAKE_KERNEL_PATCH_NAME(1100, proc_id_recv)[] = {0xA9BF2FEA, 0xF94073EB, 0x5280006A, 0xD37EF54A, 0xF86A696A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000100, 0xA9BF27E8, 0xF9400308, 0xF9401D08, 0xAA1803E0, 0xD63F0100, 0xA8C127E8, 0xAA0003E8, 0xA8C12FEA, 0xAA0803E0};

/* svcControlCodeMemory Patches */
/* b.eq -> nop */
static const instruction_t MAKE_KERNEL_PATCH_NAME(500,  svc_control_codememory)[] = {MAKE_NOP};
static const instruction_t MAKE_KERNEL_PATCH_NAME(600,  svc_control_codememory)[] = {MAKE_NOP};
static const instruction_t MAKE_KERNEL_PATCH_NAME(700,  svc_control_codememory)[] = {MAKE_NOP};
static const instruction_t MAKE_KERNEL_PATCH_NAME(800,  svc_control_codememory)[] = {MAKE_NOP};
static const instruction_t MAKE_KERNEL_PATCH_NAME(900,  svc_control_codememory)[] = {MAKE_NOP};
static const instruction_t MAKE_KERNEL_PATCH_NAME(1000, svc_control_codememory)[] = {MAKE_NOP};
static const instruction_t MAKE_KERNEL_PATCH_NAME(1100, svc_control_codememory)[] = {MAKE_NOP};

static const instruction_t MAKE_KERNEL_PATCH_NAME(500,  system_memory_increase)[] = {0x52A3C008}; /* MOV W8,   #0x1E000000 */
static const instruction_t MAKE_KERNEL_PATCH_NAME(600,  system_memory_increase)[] = {0x52A3B008}; /* MOV W8,   #0x1D800000 */
static const instruction_t MAKE_KERNEL_PATCH_NAME(700,  system_memory_increase)[] = {0x52A3B008}; /* MOV W8,   #0x1D800000 */
static const instruction_t MAKE_KERNEL_PATCH_NAME(800,  system_memory_increase)[] = {0x52A3B013}; /* MOV W19,  #0x1D800000 */
static const instruction_t MAKE_KERNEL_PATCH_NAME(900,  system_memory_increase)[] = {0x52A3B013}; /* MOV W19,  #0x1D800000 */
static const instruction_t MAKE_KERNEL_PATCH_NAME(1000, system_memory_increase)[] = {0x52A3B013}; /* MOV W19,  #0x1D800000 */
static const instruction_t MAKE_KERNEL_PATCH_NAME(1100, system_memory_increase)[] = {0x52A3B015}; /* MOV W21,  #0x1D800000 */

/* Hook Definitions. */
static const kernel_patch_t g_kernel_patches_100[] = {
    {   /* Send Message Process ID Patch. */
        .pattern_size = 0x10,
        .pattern = MAKE_KERNEL_PATTERN_NAME(100, proc_id_send),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(100, proc_id_send))/sizeof(instruction_t),
        .branch_back_offset = 0x4,
        .payload = MAKE_KERNEL_PATCH_NAME(100, proc_id_send)
    },
    {   /* Receive Message Process ID Patch. */
        .pattern_size = 0x10,
        .pattern = MAKE_KERNEL_PATTERN_NAME(100, proc_id_recv),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(100, proc_id_recv))/sizeof(instruction_t),
        .branch_back_offset = 0x4,
        .payload = MAKE_KERNEL_PATCH_NAME(100, proc_id_recv)
    }
};
static const kernel_patch_t g_kernel_patches_200[] = {
    {   /* Send Message Process ID Patch. */
        .pattern_size = 0x10,
        .pattern = MAKE_KERNEL_PATTERN_NAME(200, proc_id_send),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(200, proc_id_send))/sizeof(instruction_t),
        .branch_back_offset = 0x4,
        .payload = MAKE_KERNEL_PATCH_NAME(200, proc_id_send)
    },
    {   /* Receive Message Process ID Patch. */
        .pattern_size = 0x10,
        .pattern = MAKE_KERNEL_PATTERN_NAME(200, proc_id_recv),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(200, proc_id_recv))/sizeof(instruction_t),
        .branch_back_offset = 0x4,
        .payload = MAKE_KERNEL_PATCH_NAME(200, proc_id_recv)
    }
};
static const kernel_patch_t g_kernel_patches_300[] = {
    {   /* Send Message Process ID Patch. */
        .pattern_size = 0x10,
        .pattern = MAKE_KERNEL_PATTERN_NAME(300, proc_id_send),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(300, proc_id_send))/sizeof(instruction_t),
        .branch_back_offset = 0x4,
        .payload = MAKE_KERNEL_PATCH_NAME(300, proc_id_send)
    },
    {   /* Receive Message Process ID Patch. */
        .pattern_size = 0x10,
        .pattern = MAKE_KERNEL_PATTERN_NAME(300, proc_id_recv),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(300, proc_id_recv))/sizeof(instruction_t),
        .branch_back_offset = 0x4,
        .payload = MAKE_KERNEL_PATCH_NAME(300, proc_id_recv)
    }
};
static const kernel_patch_t g_kernel_patches_302[] = {
    {   /* Send Message Process ID Patch. */
        .pattern_size = 0x10,
        .pattern = MAKE_KERNEL_PATTERN_NAME(302, proc_id_send),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(302, proc_id_send))/sizeof(instruction_t),
        .branch_back_offset = 0x4,
        .payload = MAKE_KERNEL_PATCH_NAME(302, proc_id_send)
    },
    {   /* Receive Message Process ID Patch. */
        .pattern_size = 0x10,
        .pattern = MAKE_KERNEL_PATTERN_NAME(302, proc_id_recv),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(302, proc_id_recv))/sizeof(instruction_t),
        .branch_back_offset = 0x4,
        .payload = MAKE_KERNEL_PATCH_NAME(302, proc_id_recv)
    }
};
static const kernel_patch_t g_kernel_patches_400[] = {
    {   /* Send Message Process ID Patch. */
        .pattern_size = 0x10,
        .pattern = MAKE_KERNEL_PATTERN_NAME(400, proc_id_send),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(400, proc_id_send))/sizeof(instruction_t),
        .branch_back_offset = 0x8,
        .payload = MAKE_KERNEL_PATCH_NAME(400, proc_id_send)
    },
    {   /* Receive Message Process ID Patch. */
        .pattern_size = 0x10,
        .pattern = MAKE_KERNEL_PATTERN_NAME(400, proc_id_recv),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(400, proc_id_recv))/sizeof(instruction_t),
        .branch_back_offset = 0x4,
        .payload = MAKE_KERNEL_PATCH_NAME(400, proc_id_recv)
    }
};
static const kernel_patch_t g_kernel_patches_500[] = {
    {   /* Send Message Process ID Patch. */
        .pattern_size = 0x10,
        .pattern = MAKE_KERNEL_PATTERN_NAME(500, proc_id_send),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(500, proc_id_send))/sizeof(instruction_t),
        .branch_back_offset = 0x8,
        .payload = MAKE_KERNEL_PATCH_NAME(500, proc_id_send)
    },
    {   /* Receive Message Process ID Patch. */
        .pattern_size = 0x10,
        .pattern = MAKE_KERNEL_PATTERN_NAME(500, proc_id_recv),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(500, proc_id_recv))/sizeof(instruction_t),
        .branch_back_offset = 0x8,
        .payload = MAKE_KERNEL_PATCH_NAME(500, proc_id_recv)
    },
    {   /* svcControlCodeMemory Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(500, svc_control_codememory))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(500, svc_control_codememory),
        .patch_offset = 0x38C2C,
    },
    {   /* System Memory Increase Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(500, system_memory_increase))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(500, system_memory_increase),
        .patch_offset = 0x54E30,
    }
};
static const kernel_patch_t g_kernel_patches_600[] = {
    {   /* Send Message Process ID Patch. */
        .pattern_size = 0x1C,
        .pattern = MAKE_KERNEL_PATTERN_NAME(600, proc_id_send),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(600, proc_id_send))/sizeof(instruction_t),
        .branch_back_offset = 0x10,
        .payload = MAKE_KERNEL_PATCH_NAME(600, proc_id_send)
    },
    {   /* Receive Message Process ID Patch. */
        .pattern_size = 0x1C,
        .pattern = MAKE_KERNEL_PATTERN_NAME(600, proc_id_recv),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(600, proc_id_recv))/sizeof(instruction_t),
        .branch_back_offset = 0x10,
        .payload = MAKE_KERNEL_PATCH_NAME(600, proc_id_recv)
    },
    {   /* svcControlCodeMemory Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(600, svc_control_codememory))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(600, svc_control_codememory),
        .patch_offset = 0x3A8CC,
    },
    {   /* System Memory Increase Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(600, system_memory_increase))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(600, system_memory_increase),
        .patch_offset = 0x57330,
    }
};
static const kernel_patch_t g_kernel_patches_700[] = {
    {   /* Send Message Process ID Patch. */
        .pattern_size = 0x1C,
        .pattern = MAKE_KERNEL_PATTERN_NAME(700, proc_id_send),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(700, proc_id_send))/sizeof(instruction_t),
        .branch_back_offset = 0x10,
        .payload = MAKE_KERNEL_PATCH_NAME(700, proc_id_send)
    },
    {   /* Receive Message Process ID Patch. */
        .pattern_size = 0x1C,
        .pattern = MAKE_KERNEL_PATTERN_NAME(700, proc_id_recv),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(700, proc_id_recv))/sizeof(instruction_t),
        .branch_back_offset = 0x10,
        .payload = MAKE_KERNEL_PATCH_NAME(700, proc_id_recv)
    },
    {   /* svcControlCodeMemory Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(700, svc_control_codememory))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(700, svc_control_codememory),
        .patch_offset = 0x3C6E0,
    },
    {   /* System Memory Increase Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(700, system_memory_increase))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(700, system_memory_increase),
        .patch_offset = 0x57F98,
    }
};

static const kernel_patch_t g_kernel_patches_800[] = {
    {   /* Send Message Process ID Patch. */
        .pattern_size = 0x1C,
        .pattern = MAKE_KERNEL_PATTERN_NAME(800, proc_id_send),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(800, proc_id_send))/sizeof(instruction_t),
        .branch_back_offset = 0x10,
        .payload = MAKE_KERNEL_PATCH_NAME(800, proc_id_send)
    },
    {   /* Receive Message Process ID Patch. */
        .pattern_size = 0x1C,
        .pattern = MAKE_KERNEL_PATTERN_NAME(800, proc_id_recv),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(800, proc_id_recv))/sizeof(instruction_t),
        .branch_back_offset = 0x10,
        .payload = MAKE_KERNEL_PATCH_NAME(800, proc_id_recv)
    },
    {   /* svcControlCodeMemory Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(800, svc_control_codememory))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(800, svc_control_codememory),
        .patch_offset = 0x3FAD0,
    },
    {   /* System Memory Increase Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(800, system_memory_increase))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(800, system_memory_increase),
        .patch_offset = 0x5F9A4,
    }
};

static const kernel_patch_t g_kernel_patches_900[] = {
    {   /* Send Message Process ID Patch. */
        .pattern_size = 0x1C,
        .pattern = MAKE_KERNEL_PATTERN_NAME(900, proc_id_send),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(900, proc_id_send))/sizeof(instruction_t),
        .branch_back_offset = 0x10,
        .payload = MAKE_KERNEL_PATCH_NAME(900, proc_id_send)
    },
    {   /* Receive Message Process ID Patch. */
        .pattern_size = 0x1C,
        .pattern = MAKE_KERNEL_PATTERN_NAME(900, proc_id_recv),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(900, proc_id_recv))/sizeof(instruction_t),
        .branch_back_offset = 0x10,
        .payload = MAKE_KERNEL_PATCH_NAME(900, proc_id_recv)
    },
    {   /* svcControlCodeMemory Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(900, svc_control_codememory))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(900, svc_control_codememory),
        .patch_offset = 0x43DFC,
    },
    {   /* System Memory Increase Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(900, system_memory_increase))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(900, system_memory_increase),
        .patch_offset = 0x6493C,
    }
};

static const kernel_patch_t g_kernel_patches_1000[] = {
    {   /* Send Message Process ID Patch. */
        .pattern_size = 0x1C,
        .pattern = MAKE_KERNEL_PATTERN_NAME(1000, proc_id_send),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(1000, proc_id_send))/sizeof(instruction_t),
        .branch_back_offset = 0x10,
        .payload = MAKE_KERNEL_PATCH_NAME(1000, proc_id_send)
    },
    {   /* Receive Message Process ID Patch. */
        .pattern_size = 0x1C,
        .pattern = MAKE_KERNEL_PATTERN_NAME(1000, proc_id_recv),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(1000, proc_id_recv))/sizeof(instruction_t),
        .branch_back_offset = 0x10,
        .payload = MAKE_KERNEL_PATCH_NAME(1000, proc_id_recv)
    },
    {   /* svcControlCodeMemory Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(1000, svc_control_codememory))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(1000, svc_control_codememory),
        .patch_offset = 0x45DAC,
    },
    {   /* System Memory Increase Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(1000, system_memory_increase))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(1000, system_memory_increase),
        .patch_offset = 0x66950,
    }
};

static const kernel_patch_t g_kernel_patches_1100[] = {
    {   /* Send Message Process ID Patch. */
        .pattern_size = 0x1C,
        .pattern = MAKE_KERNEL_PATTERN_NAME(1100, proc_id_send),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(1100, proc_id_send))/sizeof(instruction_t),
        .branch_back_offset = 0x10,
        .payload = MAKE_KERNEL_PATCH_NAME(1100, proc_id_send)
    },
    {   /* Receive Message Process ID Patch. */
        .pattern_size = 0x1C,
        .pattern = MAKE_KERNEL_PATTERN_NAME(1100, proc_id_recv),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(1100, proc_id_recv))/sizeof(instruction_t),
        .branch_back_offset = 0x10,
        .payload = MAKE_KERNEL_PATCH_NAME(1100, proc_id_recv)
    },
    {   /* svcControlCodeMemory Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(1100, svc_control_codememory))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(1100, svc_control_codememory),
        .patch_offset = 0x2FCE0,
    },
    {   /* System Memory Increase Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(1100, system_memory_increase))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(1100, system_memory_increase),
        .patch_offset = 0x490C4,
    }
};

static const kernel_patch_t g_kernel_patches_1101[] = {
    {   /* Send Message Process ID Patch. */
        .pattern_size = 0x1C,
        .pattern = MAKE_KERNEL_PATTERN_NAME(1100, proc_id_send),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(1100, proc_id_send))/sizeof(instruction_t),
        .branch_back_offset = 0x10,
        .payload = MAKE_KERNEL_PATCH_NAME(1100, proc_id_send)
    },
    {   /* Receive Message Process ID Patch. */
        .pattern_size = 0x1C,
        .pattern = MAKE_KERNEL_PATTERN_NAME(1100, proc_id_recv),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(1100, proc_id_recv))/sizeof(instruction_t),
        .branch_back_offset = 0x10,
        .payload = MAKE_KERNEL_PATCH_NAME(1100, proc_id_recv)
    },
    {   /* svcControlCodeMemory Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(1100, svc_control_codememory))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(1100, svc_control_codememory),
        .patch_offset = 0x2FD04,
    },
    {   /* System Memory Increase Patch. */
        .payload_num_instructions = sizeof(MAKE_KERNEL_PATCH_NAME(1100, system_memory_increase))/sizeof(instruction_t),
        .payload = MAKE_KERNEL_PATCH_NAME(1100, system_memory_increase),
        .patch_offset = 0x490C4,
    }
};

#define KERNEL_PATCHES(vers) .num_patches = sizeof(g_kernel_patches_##vers)/sizeof(kernel_patch_t), .patches = g_kernel_patches_##vers,

/* Kernel Infos. */
static const kernel_info_t g_kernel_infos[] = {
    {   /* 1.0.0-7. */
        .hash = {0x64, 0x44, 0x07, 0x2F, 0x56, 0x44, 0x73, 0xDD, 0xD5, 0x46, 0x1B, 0x8C, 0xDC, 0xEF, 0x54, 0x98, 0x16, 0xDA, 0x81, 0xDE, 0x5B, 0x1C, 0x9D, 0xD7, 0x5A, 0x13, 0x91, 0xD9, 0x53, 0xAB, 0x8D, 0x8D},
        .free_code_space_offset = 0x4797C,
        KERNEL_PATCHES(100)
    },
    {   /* 1.0.0. */
        .hash = {0xB8, 0xC5, 0x0C, 0x68, 0x25, 0xA9, 0xB9, 0x5B, 0xD2, 0x4D, 0x2C, 0x7C, 0x81, 0x7F, 0xE6, 0x96, 0xF2, 0x42, 0x4E, 0x1D, 0x78, 0xDF, 0x3B, 0xCA, 0x3D, 0x6B, 0x68, 0x12, 0xDD, 0xA9, 0xCB, 0x9C},
        .free_code_space_offset = 0x4797C,
        KERNEL_PATCHES(100)
    },
    {   /* 2.0.0. */
        .hash = {0x64, 0x0B, 0x51, 0xFF, 0x28, 0x01, 0xB8, 0x30, 0xA7, 0xA3, 0x60, 0x47, 0x86, 0x0D, 0x68, 0xAA, 0x9A, 0x92, 0x10, 0x0D, 0xB9, 0xCC, 0xEC, 0x8B, 0x05, 0x80, 0x73, 0xBD, 0x33, 0xB4, 0x2C, 0x6C},
        .free_code_space_offset = 0x6486C,
        KERNEL_PATCHES(200)
    },
    {   /* 3.0.0. */
        .hash = {0x50, 0x84, 0x23, 0xAC, 0x6F, 0xA1, 0x5D, 0x3B, 0x56, 0xC2, 0xFC, 0x95, 0x22, 0xCC, 0xD5, 0xA8, 0x15, 0xD3, 0xB4, 0x6B, 0xA1, 0x2C, 0xF2, 0x93, 0xD3, 0x02, 0x05, 0xAB, 0x52, 0xEF, 0x73, 0xC5},
        .free_code_space_offset = 0x494A4,
        KERNEL_PATCHES(300)
    },
    {   /* 3.0.2. */
        .hash = {0x81, 0x9D, 0x08, 0xBE, 0xE4, 0x5E, 0x1F, 0xBB, 0x45, 0x5A, 0x6D, 0x70, 0x4B, 0xB2, 0x17, 0xA6, 0x12, 0x69, 0xF8, 0xB8, 0x75, 0x1C, 0x71, 0x16, 0xF0, 0xE9, 0x79, 0x7F, 0xB0, 0xD1, 0x78, 0xB2},
        .free_code_space_offset = 0x494BC,
        KERNEL_PATCHES(302)
    },
    {   /* 4.0.0. */
        .hash = {0xE6, 0xC0, 0xB7, 0xE3, 0x2F, 0xF9, 0x44, 0x51, 0xEC, 0xD5, 0x95, 0x79, 0xE3, 0x46, 0xB1, 0xDA, 0x2E, 0xD9, 0x28, 0xC6, 0xF2, 0x31, 0x4F, 0x95, 0xD8, 0xC7, 0xD5, 0xBD, 0x15, 0xD5, 0xE2, 0x5A},
        .free_code_space_offset = 0x52890,
        KERNEL_PATCHES(400)
    },
    {   /* 5.0.0. */
        .hash = {0xB2, 0x38, 0x61, 0xA8, 0xE1, 0xE2, 0xE4, 0xE4, 0x17, 0x28, 0xED, 0xA9, 0xF6, 0xF6, 0xBD, 0xD2, 0x59, 0xDB, 0x1F, 0xEF, 0x4A, 0x8B, 0x2F, 0x1C, 0x64, 0x46, 0x06, 0x40, 0xF5, 0x05, 0x9C, 0x43},
        .free_code_space_offset = 0x5C020,
        KERNEL_PATCHES(500)
    },
    {   /* 6.0.0. */
        .hash = {0x85, 0x97, 0x40, 0xF6, 0xC0, 0x3E, 0x3D, 0x44, 0xDE, 0xA4, 0xA0, 0x35, 0xFD, 0x12, 0x9C, 0xD4, 0x4F, 0x9C, 0x36, 0x53, 0x74, 0x54, 0x2C, 0x9C, 0x55, 0x47, 0xC4, 0x25, 0xF1, 0x42, 0xFB, 0x97},
        .free_code_space_offset = 0x5EE00,
        KERNEL_PATCHES(600)
    },
    {   /* 7.0.0. */
        .hash = {0xA2, 0x5E, 0x47, 0x0C, 0x8E, 0x6D, 0x2F, 0xD7, 0x5D, 0xAD, 0x24, 0xD7, 0xD8, 0x24, 0x34, 0xFB, 0xCD, 0x77, 0xBB, 0xE6, 0x66, 0x03, 0xCB, 0xAF, 0xAB, 0x85, 0x45, 0xA0, 0x91, 0xAF, 0x34, 0x25},
        .free_code_space_offset = 0x5FEC0,
        KERNEL_PATCHES(700)
    },
    {   /* 8.0.0. */
        .hash = {0xA6, 0xAD, 0x5D, 0x7F, 0xCF, 0x25, 0x80, 0xAE, 0xE6, 0x57, 0x9F, 0x6F, 0xC5, 0xC5, 0xF6, 0x13, 0x77, 0x23, 0xAC, 0x88, 0x79, 0x76, 0xF7, 0x25, 0x06, 0x16, 0x35, 0x3B, 0x3F, 0xA7, 0x59, 0x49},
        .hash_offset = 0x1A8,
        .hash_size = 0x95000 - 0x1A8,
        .embedded_ini_offset = 0x95000,
        .embedded_ini_ptr = 0x168,
        .free_code_space_offset = 0x607F0,
        KERNEL_PATCHES(800)
    },
    {   /* 9.0.0. */
        .hash = {0xD7, 0x95, 0x65, 0x3A, 0x49, 0x4C, 0x5A, 0x9E, 0x2E, 0x04, 0xD6, 0x30, 0x7D, 0x79, 0xE1, 0xEE, 0x10, 0x2B, 0x30, 0xE0, 0x3E, 0xDD, 0x9F, 0xB3, 0x8A, 0x3C, 0x5E, 0xD3, 0x9B, 0x30, 0x11, 0x9B},
        .hash_offset = 0x1C0,
        .hash_size = 0x90000 - 0x1C0,
        .embedded_ini_offset = 0x90000,
        .embedded_ini_ptr = 0x180,
        .free_code_space_offset = 0x65780,
        KERNEL_PATCHES(900)
    },
    {   /* 9.2.0. */
        /* NOTE: 9.2.0 has identical kernel layout to 9.0.0, so patches may be reused. */
        .hash = {0x66, 0xE7, 0x73, 0xE7, 0xF5, 0xCB, 0x9B, 0xB8, 0x66, 0xB7, 0xB1, 0x26, 0x23, 0x02, 0x76, 0xF2, 0xD1, 0x60, 0x3E, 0x09, 0x14, 0x19, 0xC2, 0x84, 0xFA, 0x5D, 0x0F, 0x16, 0xA3, 0x65, 0xFA, 0x17},
        .hash_offset = 0x1C0,
        .hash_size = 0x90000 - 0x1C0,
        .embedded_ini_offset = 0x90000,
        .embedded_ini_ptr = 0x180,
        .free_code_space_offset = 0x65780,
        KERNEL_PATCHES(900)
    },
    {   /* 10.0.0. */
        .hash = {0x59, 0x31, 0xE6, 0x46, 0xF7, 0xAA, 0x15, 0x59, 0x78, 0xC7, 0xB3, 0xA5, 0xFA, 0x80, 0xE2, 0xC0, 0xCA, 0x6F, 0x31, 0x97, 0x3D, 0x86, 0x8A, 0x69, 0xF3, 0xBF, 0xE6, 0xE5, 0x61, 0xA7, 0x1B, 0x5B, },
        .hash_offset = 0x1B8,
        .hash_size = 0x93000 - 0x1B8,
        .embedded_ini_offset = 0x93000,
        .embedded_ini_ptr = 0x178,
        .free_code_space_offset = 0x67790,
        KERNEL_PATCHES(1000)
    },
    {   /* 11.0.0. */
        .hash = {0xC2, 0x0E, 0xB3, 0x1B, 0xBF, 0x0B, 0x82, 0xF3, 0x3D, 0xFD, 0x47, 0x04, 0xB4, 0x44, 0x38, 0x47, 0x64, 0xAB, 0xD8, 0x70, 0x2F, 0x0E, 0x0C, 0x37, 0x82, 0x28, 0x02, 0x24, 0xB8, 0x6E, 0xCE, 0x05, },
        .hash_offset = 0x1C4,
        .hash_size = 0x69000 - 0x1C4,
        .embedded_ini_offset = 0x69000,
        .embedded_ini_ptr = 0x180,
        .free_code_space_offset = 0x49EE8,
        KERNEL_PATCHES(1100)
    },
    {   /* 11.0.1. */
        .hash = {0x68, 0xB9, 0x72, 0xB7, 0x97, 0x55, 0x87, 0x5E, 0x24, 0x95, 0x8D, 0x99, 0x0A, 0x77, 0xAB, 0xF1, 0xC5, 0xC1, 0x32, 0x80, 0x67, 0xF0, 0xA2, 0xEC, 0x9C, 0xEF, 0xC3, 0x22, 0xE3, 0x42, 0xC0, 0x4D, },
        .hash_offset = 0x1C4,
        .hash_size = 0x69000 - 0x1C4,
        .embedded_ini_offset = 0x69000,
        .embedded_ini_ptr = 0x180,
        .free_code_space_offset = 0x49EE8,
        KERNEL_PATCHES(1101)
    }
};

/* Adapted from https://github.com/AuroraWright/Luma3DS/blob/master/sysmodules/loader/source/memory.c:35. */
uint8_t *search_pattern(void *_mem, size_t mem_size, const void *_pattern, size_t pattern_size) {
    const uint8_t *pattern = (const uint8_t *)_pattern;
    uint8_t *mem = (uint8_t *)_mem;

    uint32_t table[0x100];
    for (unsigned int i = 0; i < sizeof(table)/sizeof(uint32_t); i++) {
        table[i] = (uint32_t)pattern_size;
    }
    for (unsigned int i = 0; i < pattern_size - 1; i++) {
        table[pattern[i]] = (uint32_t)pattern_size - i - 1;
    }

    for (unsigned int i = 0; i <= mem_size - pattern_size; i += table[mem[i + pattern_size - 1]]) {
        if (pattern[pattern_size - 1] == mem[i + pattern_size - 1] && memcmp(pattern, mem + i, pattern_size - 1) == 0) {
            return mem + i;
        }
    }
    return NULL;
}

const kernel_info_t *get_kernel_info(void *kernel, size_t size) {
    uint8_t calculated_hash[0x20];
    uint8_t calculated_partial_hash[0x20];
    se_calculate_sha256(calculated_hash, kernel, size);

    for (unsigned int i = 0; i < sizeof(g_kernel_infos)/sizeof(kernel_info_t); i++) {
        if (g_kernel_infos[i].hash_size == 0 || size <= g_kernel_infos[i].hash_size) {
            if (memcmp(calculated_hash, g_kernel_infos[i].hash, sizeof(calculated_hash)) == 0) {
                return &g_kernel_infos[i];
            }
        } else {
            se_calculate_sha256(calculated_partial_hash, (void *)((uintptr_t)kernel + g_kernel_infos[i].hash_offset), g_kernel_infos[i].hash_size);
            if (memcmp(calculated_partial_hash, g_kernel_infos[i].hash, sizeof(calculated_partial_hash)) == 0) {
                return &g_kernel_infos[i];
            }
        }
    }
    return NULL;
}

void package2_patch_kernel(void *_kernel, size_t *kernel_size, bool is_sd_kernel, void **out_ini1, uint32_t target_firmware) {
    const kernel_info_t *kernel_info = get_kernel_info(_kernel, *kernel_size);
    *out_ini1 = NULL;

    /* Apply IPS patches. */
    apply_kernel_ips_patches(_kernel, *kernel_size);

    if (kernel_info == NULL && !is_sd_kernel) {
        /* Should this be fatal? */
        fatal_error("kernel_patcher: unable to identify kernel!\n");
    }

    if (kernel_info == NULL && is_sd_kernel) {
        return;
    }

    if (kernel_info->embedded_ini_offset != 0) {
        /* Set output INI ptr. */
        *out_ini1 = (void *)((uintptr_t)_kernel + kernel_info->embedded_ini_offset);
        *((volatile uint64_t *)((uintptr_t)_kernel + kernel_info->embedded_ini_ptr)) = (uint64_t)*kernel_size;
    }

    /* Apply hooks and patches. */
    uint8_t *kernel = (uint8_t *)_kernel;
    size_t free_space_offset = kernel_info->free_code_space_offset;
    size_t free_space_size = ((free_space_offset + 0xFFFULL) & ~0xFFFULL) - free_space_offset;
    for (unsigned int i = 0; i < kernel_info->num_patches; i++) {
        if (kernel_info->patches[i].patch_offset) {
            for (unsigned int p = 0; p < kernel_info->patches[i].payload_num_instructions; p++) {
                *(volatile instruction_t*)(_kernel + kernel_info->patches[i].patch_offset) = kernel_info->patches[i].payload[p];
            }
            continue;
        }

        size_t hook_size = sizeof(instruction_t) * kernel_info->patches[i].payload_num_instructions;

        if (kernel_info->patches[i].branch_back_offset) {
            hook_size += sizeof(instruction_t);
        }
        if (free_space_size < hook_size) {
            /* TODO: What should be done in this case? */
            fatal_error("kernel_patcher: insufficient space to apply patches!\n");
        }

        uint8_t *pattern_loc = search_pattern(kernel, *kernel_size, kernel_info->patches[i].pattern, kernel_info->patches[i].pattern_size);
        if (pattern_loc == NULL) {
            fatal_error("kernel_patcher: failed to identify patch location!\n");
            continue;
        }
        /* Patch kernel to branch to our hook at the desired place. */
        volatile instruction_t *hook_start = (instruction_t *)(pattern_loc + kernel_info->patches[i].pattern_hook_offset);
        *hook_start = MAKE_BRANCH((uint32_t)((uintptr_t)hook_start - (uintptr_t)kernel), free_space_offset);

        /* Insert hook into free space. */
        volatile instruction_t *payload = (instruction_t *)(kernel + free_space_offset);
        for (unsigned int p = 0; p < kernel_info->patches[i].payload_num_instructions; p++) {
            payload[p] = kernel_info->patches[i].payload[p];
        }
        if (kernel_info->patches[i].branch_back_offset) {
            payload[kernel_info->patches[i].payload_num_instructions] = MAKE_BRANCH(free_space_offset + sizeof(instruction_t) * kernel_info->patches[i].payload_num_instructions, (uint32_t)(kernel_info->patches[i].branch_back_offset + (uintptr_t)hook_start - (uintptr_t)kernel));
        }

        free_space_offset += hook_size;
        free_space_size -= hook_size;
    }
}
