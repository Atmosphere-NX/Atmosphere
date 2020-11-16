/*
 * Copyright (c) 2019 m4xw <m4x@m4xw.net>
 * Copyright (c) 2019 Atmosphere-NX
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

#pragma once
#include "../nx/smc.h"

enum FatalReason
{
    Fatal_InitMMC = 0,
    Fatal_InitSD,
    Fatal_InvalidAccessor,
    Fatal_ReadNoAccessor,
    Fatal_WriteNoAccessor,
    Fatal_IoMappingLegacy,
    Fatal_UnknownVersion,
    Fatal_BadResult,
    Fatal_GetConfig,
    Fatal_OpenAccessor,
    Fatal_CloseAccessor,
    Fatal_IoMapping,
    Fatal_FatfsMount,
    Fatal_FatfsFileOpen,
    Fatal_FatfsMemExhaustion,
    Fatal_InvalidEnum,
    Fatal_Max
};

#define AMS_FATAL_ERROR_MAX_STACKTRACE 0x20
#define AMS_FATAL_ERROR_MAX_STACKDUMP 0x100

/* Atmosphere reboot-to-fatal-error. */
typedef struct
{
    uint32_t magic;
    uint32_t error_desc;
    uint64_t title_id;
    union {
        uint64_t gprs[32];
        struct
        {
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
} atmosphere_fatal_error_ctx;

/* "AFE1" */
#define ATMOSPHERE_REBOOT_TO_FATAL_MAGIC 0x31454641
/* "AFE0" */
#define ATMOSPHERE_REBOOT_TO_FATAL_MAGIC_0 0x30454641

#define ATMOSPHERE_FATAL_ERROR_ADDR 0x4003E000
#define ATMOSPHERE_FATAL_ERROR_CONTEXT ((volatile atmosphere_fatal_error_ctx *)(ATMOSPHERE_FATAL_ERROR_ADDR))

void __attribute__((noreturn)) fatal_abort(enum FatalReason abortReason);
