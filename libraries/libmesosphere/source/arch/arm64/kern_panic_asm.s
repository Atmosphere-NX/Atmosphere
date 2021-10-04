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
#include <mesosphere/kern_build_config.hpp>
#include <mesosphere/kern_select_assembly_offsets.h>

#if defined(MESOSPHERE_ENABLE_PANIC_REGISTER_DUMP)

#define MESOSPHERE_GENERATE_PANIC_EXCEPTION_CONTEXT        \
    /* Save x0/x1 to stack. */                             \
    stp     x0, x1, [sp, #-16]!;                           \
                                                           \
    /* Get the exception context for the core. */          \
    adr     x0, _ZN3ams4kern26g_panic_exception_contextsE; \
    mrs     x1, mpidr_el1;                                 \
    and     x1, x1, #0xFF;                                 \
    lsl     x1, x1, #0x8;                                  \
    add     x0, x0, x1;                                    \
    lsr     x1, x1, #0x3;                                  \
    add     x0, x0, x1;                                    \
                                                           \
    /* Save x0/x1/sp to the context. */                    \
    ldr     x1, [sp, #(8 *  0)];                           \
    str     x1, [x0, #(EXCEPTION_CONTEXT_X0)];             \
    ldr     x1, [sp, #(8 *  1)];                           \
    str     x1, [x0, #(EXCEPTION_CONTEXT_X1)];             \
                                                           \
    /* Save all other registers to the context. */         \
    stp     x2, x3,   [x0, #(EXCEPTION_CONTEXT_X2_X3)];    \
    stp     x4, x5,   [x0, #(EXCEPTION_CONTEXT_X4_X5)];    \
    stp     x6, x7,   [x0, #(EXCEPTION_CONTEXT_X6_X7)];    \
    stp     x8, x9,   [x0, #(EXCEPTION_CONTEXT_X8_X9)];    \
    stp     x10, x11, [x0, #(EXCEPTION_CONTEXT_X10_X11)];  \
    stp     x12, x13, [x0, #(EXCEPTION_CONTEXT_X12_X13)];  \
    stp     x14, x15, [x0, #(EXCEPTION_CONTEXT_X14_X15)];  \
    stp     x16, x17, [x0, #(EXCEPTION_CONTEXT_X16_X17)];  \
    stp     x18, x19, [x0, #(EXCEPTION_CONTEXT_X18_X19)];  \
    stp     x20, x21, [x0, #(EXCEPTION_CONTEXT_X20_X21)];  \
    stp     x22, x23, [x0, #(EXCEPTION_CONTEXT_X22_X23)];  \
    stp     x24, x25, [x0, #(EXCEPTION_CONTEXT_X24_X25)];  \
    stp     x26, x27, [x0, #(EXCEPTION_CONTEXT_X26_X27)];  \
    stp     x28, x29, [x0, #(EXCEPTION_CONTEXT_X28_X29)];  \
                                                           \
    add     x1, sp, #16;                                   \
    stp     x30, x1,  [x0, #(EXCEPTION_CONTEXT_X30_SP)];   \
                                                           \
    /* Restore x0/x1. */                                   \
    ldp     x0, x1, [sp], #16;

#else

#define MESOSPHERE_GENERATE_PANIC_EXCEPTION_CONTEXT

#endif

/* ams::kern::Panic(const char *file, int line, const char *format, ...) */
.section    .text._ZN3ams4kern5PanicEPKciS2_z, "ax", %progbits
.global     _ZN3ams4kern5PanicEPKciS2_z
.type       _ZN3ams4kern5PanicEPKciS2_z, %function
_ZN3ams4kern5PanicEPKciS2_z:
    /* Generate the exception context. */
    MESOSPHERE_GENERATE_PANIC_EXCEPTION_CONTEXT

    /* Jump to the architecturally-common implementation function. */
    b _ZN3ams4kern9PanicImplEPKciS2_z


/* ams::kern::Panic() */
.section    .text._ZN3ams4kern5PanicEv, "ax", %progbits
.global     _ZN3ams4kern5PanicEv
.type       _ZN3ams4kern5PanicEv, %function
_ZN3ams4kern5PanicEv:
    /* Generate the exception context. */
    MESOSPHERE_GENERATE_PANIC_EXCEPTION_CONTEXT

    /* Jump to the architecturally-common implementation function. */
    b _ZN3ams4kern9PanicImplEv
