/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
    str     x1, [x0, #(8 *  0)];                           \
    ldr     x1, [sp, #(8 *  1)];                           \
    str     x1, [x0, #(8 *  1)];                           \
                                                           \
    /* Save all other registers to the context. */         \
    stp     x2, x3,   [x0, #(8 *  2)];                     \
    stp     x4, x5,   [x0, #(8 *  4)];                     \
    stp     x6, x7,   [x0, #(8 *  6)];                     \
    stp     x8, x9,   [x0, #(8 *  8)];                     \
    stp     x10, x11, [x0, #(8 * 10)];                     \
    stp     x12, x13, [x0, #(8 * 12)];                     \
    stp     x14, x15, [x0, #(8 * 14)];                     \
    stp     x16, x17, [x0, #(8 * 16)];                     \
    stp     x18, x19, [x0, #(8 * 18)];                     \
    stp     x20, x21, [x0, #(8 * 20)];                     \
    stp     x22, x23, [x0, #(8 * 22)];                     \
    stp     x24, x25, [x0, #(8 * 24)];                     \
    stp     x26, x27, [x0, #(8 * 26)];                     \
    stp     x28, x29, [x0, #(8 * 28)];                     \
                                                           \
    add     x1, sp, #16;                                   \
    stp     x30, x1,  [x0, #(8 * 30)];                     \
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
