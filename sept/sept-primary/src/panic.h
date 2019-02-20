/*
 * Copyright (c) 2018 Atmosphère-NX
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
 
#ifndef FUSEE_PANIC_H
#define FUSEE_PANIC_H

#include <stdint.h>

#define PANIC_COLOR_KERNEL              0x0000FF
#define PANIC_COLOR_SECMON_EXCEPTION    0xFF7700
#define PANIC_COLOR_SECMON_GENERIC      0x00FFFF
#define PANIC_COLOR_SECMON_DEEPSLEEP    0xFF77FF /* 4.0+ color */
#define PANIC_COLOR_BOOTLOADER_GENERIC  0xAA00FF
#define PANIC_COLOR_BOOTLOADER_SAFEMODE 0xFFFFAA /* Removed */

#define PANIC_CODE_SAFEMODE 0x00000020

void check_and_display_panic(void);
__attribute__ ((noreturn)) void panic(uint32_t code);

#endif
