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
 
#pragma once
#include <switch.h>
#include <stratosphere.hpp>

enum FatalResult : Result {
    FatalResult_NullGfxBuffer = 0x4A3,
    FatalResult_AlreadyThrown = 0x6A3,
    FatalResult_TooManyEvents = 0x8A3,
    FatalResult_InRepairWithoutVolHeld = 0xAA3,
    FatalResult_InRepairWithoutTimeReviserCartridge = 0xCA3,
};

static constexpr size_t NumAarch64Gprs = 32;
static constexpr size_t NumAarch32Gprs = 16;

struct Aarch64CpuContext {
    using RegisterType = u64;
    static constexpr size_t MaxStackTraceDepth = 0x20;
    
    /* Registers, exception context. N left names for these fields in fatal .rodata. */
    union {
        RegisterType x[NumAarch64Gprs];
        struct {
            RegisterType _x[29];
            RegisterType fp;
            RegisterType lr;
            RegisterType sp;
            RegisterType pc;
        };
    };
    RegisterType pstate;
    RegisterType afsr0;
    RegisterType afsr1;
    RegisterType esr;
    RegisterType far;
    
    /* Misc. */
    RegisterType stack_trace[MaxStackTraceDepth];
    RegisterType start_address;
    RegisterType register_set_flags;
    u32 stack_trace_size;
};

struct Aarch32CpuContext {
    using RegisterType = u32;
    static constexpr size_t MaxStackTraceDepth = 0x20;
    
    /* Registers, exception context. N left names for these fields in fatal .rodata. */
    union {
        RegisterType r[NumAarch32Gprs];
        struct {
            RegisterType _r[11];
            RegisterType fp;
            RegisterType ip;
            RegisterType sp;
            RegisterType lr;
            RegisterType pc;
        };
    };
    RegisterType pstate;
    RegisterType afsr0;
    RegisterType afsr1;
    RegisterType esr;
    RegisterType far;
    
    /* Misc. Yes, stack_trace_size is really laid out differently than aarch64... */
    RegisterType stack_trace[MaxStackTraceDepth];
    u32 stack_trace_size;
    RegisterType start_address;
    RegisterType register_set_flags;
};

struct FatalCpuContext {
    union {
        Aarch64CpuContext aarch64_ctx;
        Aarch32CpuContext aarch32_ctx;
    };
    
    bool is_aarch32;
    u32 type;
};

struct FatalThrowContext {
    u32 error_code;
    bool is_creport;
    bool has_gprs[NumAarch64Gprs];
    size_t stack_dump_size;
    u8 stack_dump[0x100];
    char proc_name[0xD];
    FatalCpuContext cpu_ctx;
};

static_assert(sizeof(Aarch64CpuContext) == 0x248, "Aarch64CpuContext definition!");
static_assert(sizeof(Aarch32CpuContext) == 0xE0, "Aarch32CpuContext definition!");
static_assert(sizeof(FatalCpuContext) == 0x250, "FatalCpuContext definition!");
static_assert(std::is_pod_v<FatalCpuContext>, "FatalCpuContext definition!");

static constexpr const char *Aarch64GprNames[NumAarch64Gprs] = {
    u8"X0",
    u8"X1",
    u8"X2",
    u8"X3",
    u8"X4",
    u8"X5",
    u8"X6",
    u8"X7",
    u8"X8",
    u8"X9",
    u8"X10",
    u8"X11",
    u8"X12",
    u8"X13",
    u8"X14",
    u8"X15",
    u8"X16",
    u8"X17",
    u8"X18",
    u8"X19",
    u8"X20",
    u8"X21",
    u8"X22",
    u8"X23",
    u8"X24",
    u8"X25",
    u8"X26",
    u8"X27",
    u8"X28",
    u8"FP",
    u8"LR",
    u8"SP",
};

static constexpr const char *Aarch32GprNames[NumAarch32Gprs] = {
    u8"R0",
    u8"R1",
    u8"R2",
    u8"R3",
    u8"R4",
    u8"R5",
    u8"R6",
    u8"R7",
    u8"R8",
    u8"R9",
    u8"R10",
    u8"FP",
    u8"IP",
    u8"LR",
    u8"SP",
    u8"PC",
};

