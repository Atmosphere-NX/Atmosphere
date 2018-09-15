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
 
#ifndef FUSEE_NX_BOOT_H
#define FUSEE_NX_BOOT_H

#include "utils.h"

#define MAILBOX_NX_BOOTLOADER_BASE 0x40002000
#define MAILBOX_NX_BOOTLOADER_BOOT_REASON_BASE (MAILBOX_NX_BOOTLOADER_BASE + 0xE10)
#define MAKE_MAILBOX_NX_BOOTLOADER_REG(n) MAKE_REG32(MAILBOX_NX_BOOTLOADER_BASE + n)

#define MAILBOX_NX_BOOTLOADER_SETUP_STATE       MAKE_MAILBOX_NX_BOOTLOADER_REG(0xEF8)
#define MAILBOX_NX_BOOTLOADER_IS_SECMON_AWAKE   MAKE_MAILBOX_NX_BOOTLOADER_REG(0xEFC)

#define NX_BOOTLOADER_STATE_INIT 0
#define NX_BOOTLOADER_STATE_MOVED_BOOTCONFIG 1
#define NX_BOOTLOADER_STATE_LOADED_PACKAGE2 2
#define NX_BOOTLOADER_STATE_FINISHED 3
#define NX_BOOTLOADER_STATE_DRAM_INITIALIZED_4X 2
#define NX_BOOTLOADER_STATE_LOADED_PACKAGE2_4X 3
#define NX_BOOTLOADER_STATE_FINISHED_4X 4

typedef struct {
    uint32_t bootloader_version;
    uint32_t bootloader_start_block;
    uint32_t bootloader_start_page;
    uint32_t bootloader_attribute;
    uint32_t boot_reason_value;
    uint32_t boot_reason_state;
} boot_reason_t;

uint32_t nxboot_main(void);
void nxboot_finish(uint32_t boot_memaddr);

#endif
