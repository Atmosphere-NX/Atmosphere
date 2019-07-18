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

#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AMS_FATAL_ERROR_MAX_STACKTRACE 0x20
#define AMS_FATAL_ERROR_MAX_STACKDUMP 0x100

#define STD_ABORT_ADDR_MAGIC   (0x8)
#define STD_ABORT_VALUE_MAGIC  (0xA55AF00DDEADCAFEul)
#define DATA_ABORT_ERROR_DESC  (0x101)
#define STD_ABORT_ERROR_DESC   (0xFFE)

typedef struct {
    u32 magic;
    u32 error_desc;
    u64 title_id;
    union {
        u64 gprs[32];
        struct {
            u64 _gprs[29];
            u64 fp;
            u64 lr;
            u64 sp;
        };
    };
    u64 pc;
    u64 module_base;
    u32 pstate;
    u32 afsr0;
    u32 afsr1;
    u32 esr;
    u64 far;
    u64 report_identifier; /* Normally just system tick. */
    u64 stack_trace_size;
    u64 stack_dump_size;
    u64 stack_trace[AMS_FATAL_ERROR_MAX_STACKTRACE];
    u8 stack_dump[AMS_FATAL_ERROR_MAX_STACKDUMP];
} AtmosphereFatalErrorContext;

Result bpcAmsInitialize(void);
void bpcAmsExit(void);

Result bpcAmsRebootToFatalError(AtmosphereFatalErrorContext *ctx);

#ifdef __cplusplus
}
#endif