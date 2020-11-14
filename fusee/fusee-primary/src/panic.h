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

#define AMS_FATAL_ERROR_MAX_STACKTRACE 0x20
#define AMS_FATAL_ERROR_MAX_STACKDUMP 0x100
#define AMS_FATAL_ERROR_TLS_SIZE      0x100

/* Atmosphere reboot-to-fatal-error. */
typedef struct {
    uint32_t magic;
    uint32_t error_desc;
    uint64_t program_id;
    union {
        uint64_t gprs[32];
        struct {
            uint64_t _gprs[29];
            uint64_t fp;
            uint64_t lr;
            uint64_t sp;
        };
    };
    uint64_t pc;
    uint64_t module_base;
    uint32_t pstate;
    uint32_t afsr0;
    uint32_t afsr1;
    uint32_t esr;
    uint64_t far;
    uint64_t report_identifier; /* Normally just system tick. */
    uint64_t stack_trace_size;
    uint64_t stack_dump_size;
    uint64_t stack_trace[AMS_FATAL_ERROR_MAX_STACKTRACE];
    uint8_t stack_dump[AMS_FATAL_ERROR_MAX_STACKDUMP];
    uint8_t tls[AMS_FATAL_ERROR_TLS_SIZE];
} atmosphere_fatal_error_ctx;

/* "AFE2" */
#define ATMOSPHERE_REBOOT_TO_FATAL_MAGIC   0x32454641
/* "AFE1" */
#define ATMOSPHERE_REBOOT_TO_FATAL_MAGIC_1 0x31454641
/* "AFE0" */
#define ATMOSPHERE_REBOOT_TO_FATAL_MAGIC_0 0x30454641

#define ATMOSPHERE_FATAL_ERROR_CONTEXT ((volatile atmosphere_fatal_error_ctx *)(0x4003E000))

void check_and_display_panic(void);
__attribute__ ((noreturn)) void panic(uint32_t code);

#endif
