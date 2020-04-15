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

#ifndef EXOSPHERE_MC0_H
#define EXOSPHERE_MC0_H

#include <stdint.h>
#include "memory_map.h"

/* Exosphere driver for the Tegra X1 MC1. */

static inline uintptr_t get_mc1_base(void) {
    return MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_MC1);
}

#define MC1_BASE  (get_mc1_base())
#define MAKE_MC1_REG(n) MAKE_REG32(MC1_BASE + n)

#endif
