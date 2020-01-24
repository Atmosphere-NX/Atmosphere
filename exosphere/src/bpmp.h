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
 
#ifndef EXOSPHERE_BPMP_H
#define EXOSPHERE_BPMP_H

#include <stdint.h>
#include "memory_map.h"

/* Exosphere register definitions for the Tegra X1 BPMP vectors. */

static inline uintptr_t get_bpmp_vector_base(void) {
    return MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_EXCEPTION_VECTORS);
}

#define BPMP_VECTOR_BASE (get_bpmp_vector_base())

#define EVP_CPU_RESET_VECTOR_0     MAKE_REG32(BPMP_VECTOR_BASE + 0x100)

#define BPMP_VECTOR_RESET          MAKE_REG32(BPMP_VECTOR_BASE + 0x200)
#define BPMP_VECTOR_UNDEF          MAKE_REG32(BPMP_VECTOR_BASE + 0x204)
#define BPMP_VECTOR_SWI            MAKE_REG32(BPMP_VECTOR_BASE + 0x208)
#define BPMP_VECTOR_PREFETCH_ABORT MAKE_REG32(BPMP_VECTOR_BASE + 0x20C)
#define BPMP_VECTOR_DATA_ABORT     MAKE_REG32(BPMP_VECTOR_BASE + 0x210)
#define BPMP_VECTOR_UNK            MAKE_REG32(BPMP_VECTOR_BASE + 0x214)
#define BPMP_VECTOR_IRQ            MAKE_REG32(BPMP_VECTOR_BASE + 0x218)
#define BPMP_VECTOR_FIQ            MAKE_REG32(BPMP_VECTOR_BASE + 0x21C)

#define BPMP_MITIGATION_RESET_VAL  0x7D000000

#endif
