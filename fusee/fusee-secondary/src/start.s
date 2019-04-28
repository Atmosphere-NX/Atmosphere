/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <atmosphere/version.h>
 
.macro CLEAR_GPR_REG_ITER
    mov r\@, #0
.endm

.section .text.start, "ax", %progbits
.arm

.align 5
.global _start
.type   _start, %function
_start:
    b _crt0
    
.word (_metadata - _start)

_crt0:
    /* Switch to system mode, mask all interrupts, clear all flags */
    msr cpsr_cxsf, #0xDF

    /* Relocate ourselves if necessary */
    ldr r2, =_start
    adr r3, _start
    cmp r2, r3
    beq _relocation_loop_end

    ldr r4, =__bss_start__
    sub r4, r4, r2          /* size >= 32, obviously, and weve declared 32-byte-alignment */
    _relocation_loop:
        ldmia r3!, {r5-r12}
        stmia r2!, {r5-r12}
        subs  r4, #0x20
        bne _relocation_loop

    ldr r12, =_relocation_loop_end
    bx  r12

    _relocation_loop_end:
    /* Set the stack pointer */
    ldr  sp, =__stack_top__
    mov  fp, #0
    bl  __program_init

    /* Set r0 to r12 to 0 (for debugging) & call main */
    .rept 13
    CLEAR_GPR_REG_ITER
    .endr
    ldr r0, =__program_argc
    ldr r1, =__program_argv
    ldr lr, =__program_exit
    ldr r0, [r0]
    ldr r1, [r1]
    b   main
    
/* Fusee-secondary header. */
.align 5
_metadata:
.ascii "FSS0"
.word __total_size__
.word (_crt0 - _start)
.word (_content_headers - _start)
.word (_content_headers_end - _content_headers) / 0x20 /* Number of content headers */
.word ((ATMOSPHERE_SUPPORTED_HOS_VERSION_MAJOR << 24) | (ATMOSPHERE_SUPPORTED_HOS_VERSION_MINOR << 16) | (ATMOSPHERE_SUPPORTED_HOS_VERSION_MICRO << 8) | (0x0))
.word ((ATMOSPHERE_RELEASE_VERSION_MAJOR << 24) | (ATMOSPHERE_RELEASE_VERSION_MINOR << 16) | (ATMOSPHERE_RELEASE_VERSION_MICRO << 8) | (0x0))
#define TO_WORD(x) TO_WORD_(x)
#define TO_WORD_(x) 0x##x
#define AMS_GIT_REV_WORD TO_WORD(ATMOSPHERE_GIT_HASH)
.word AMS_GIT_REV_WORD
#undef  TO_WORD_
#undef  TO_WORD

#define CONTENT_TYPE_FSP 0
#define CONTENT_TYPE_EXO 1
#define CONTENT_TYPE_WBT 2
#define CONTENT_TYPE_RBT 3
#define CONTENT_TYPE_SP1 4
#define CONTENT_TYPE_SP2 5
#define CONTENT_TYPE_KIP 6
#define CONTENT_TYPE_BMP 7

_content_headers:
/* ams_mitm content header */
.word __ams_mitm_kip_start__
.word __ams_mitm_kip_size__
.word CONTENT_TYPE_KIP
.word 0xCCCCCCCC
.asciz "ams_mitm"
.align 5

/* boot_100 content header */
.word __boot_100_kip_start__
.word __boot_100_kip_size__
.word CONTENT_TYPE_KIP
.word 0xCCCCCCCC
.asciz "boot_100"
.align 5

/* boot_200 content header */
.word __boot_200_kip_start__
.word __boot_200_kip_size__
.word CONTENT_TYPE_KIP
.word 0xCCCCCCCC
.asciz "boot_200"
.align 5

/* exosphere content header */
.word __exosphere_bin_start__
.word __exosphere_bin_size__
.word CONTENT_TYPE_EXO
.word 0xCCCCCCCC
.asciz "exosphere"
.align 5

/* fusee_primary content header */
.word __fusee_primary_bin_start__
.word __fusee_primary_bin_size__
.word CONTENT_TYPE_FSP
.word 0xCCCCCCCC
.asciz "fusee_primary"
.align 5

/* loader content header */
.word __loader_kip_start__
.word __loader_kip_size__
.word CONTENT_TYPE_KIP
.word 0xCCCCCCCC
.asciz "loader"
.align 5

/* lp0fw content header */
.word __lp0fw_bin_start__
.word __lp0fw_bin_size__
.word CONTENT_TYPE_WBT
.word 0xCCCCCCCC
.asciz "lp0fw"
.align 5

/* pm content header */
.word __pm_kip_start__
.word __pm_kip_size__
.word CONTENT_TYPE_KIP
.word 0xCCCCCCCC
.asciz "pm"
.align 5

/* rebootstub content header */
.word __rebootstub_bin_start__
.word __rebootstub_bin_size__
.word CONTENT_TYPE_RBT
.word 0xCCCCCCCC
.asciz "rebootstub"
.align 5

/* sept_primary content header */
.word __sept_primary_bin_start__
.word __sept_primary_bin_size__
.word CONTENT_TYPE_SP1
.word 0xCCCCCCCC
.asciz "sept_primary"
.align 5

/* sept_secondary content header */
.word __sept_secondary_enc_start__
.word __sept_secondary_enc_size__
.word CONTENT_TYPE_SP2
.word 0xCCCCCCCC
.asciz "sept_secondary"
.align 5

/* sm content header */
.word __sm_kip_start__
.word __sm_kip_size__
.word CONTENT_TYPE_KIP
.word 0xCCCCCCCC
.asciz "sm"
.align 5

/* spl content header */
.word __spl_kip_start__
.word __spl_kip_size__
.word CONTENT_TYPE_KIP
.word 0xCCCCCCCC
.asciz "spl"
.align 5

/* splash_screen content header */
.word __splash_screen_bmp_start__
.word __splash_screen_bmp_size__
.word CONTENT_TYPE_BMP
.word 0xCCCCCCCC
.asciz "splash_screen"
.align 5
_content_headers_end:

/* No need to include this in normal programs: */
.section .chainloader.text.start, "ax", %progbits
.arm
.align 5
.global relocate_and_chainload
.type   relocate_and_chainload, %function
relocate_and_chainload:
    ldr sp, =__stack_top__
    b   relocate_and_chainload_main

.section .nxboot.text.start, "ax", %progbits
.arm
.align 5
.global nxboot
.type   nxboot, %function
nxboot:
    ldr sp, =__stack_top__
    b   nxboot_finish