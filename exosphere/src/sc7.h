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
 
#ifndef EXOSPHERE_SC7_H
#define EXOSPHERE_SC7_H

#include <stdint.h>

/* Exosphere Deep Sleep Entry implementation. */

#define LP0_TZRAM_SAVE_SIZE 0xE000

uint32_t cpu_suspend(uint64_t power_state, uint64_t entrypoint, uint64_t argument);

#endif