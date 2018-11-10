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
    FatalResult_TooManyEvents = 0x8A3,
    FatalResult_InRepairWithoutVolHeld = 0xAA3,
    FatalResult_InRepairWithoutTimeReviserCartridge = 0xCA3,
};

struct Aarch64CpuContext {
    using RegisterType = u64;
    static constexpr size_t MaxStackTraceDepth = 0x20;
    
    /* Registers, exception context. N left names for these fields in fatal .rodata. */
    union {
        RegisterType x[31];
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
        RegisterType r[16];
        struct {
            RegisterType _x[11];
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
        Aarch64CpuContext aarch32_ctx;
    };
    
    bool is_aarch32;
    u32 type;
};

static_assert(sizeof(Aarch64CpuContext) == 0x248, "Aarch64CpuContext definition!");
static_assert(sizeof(Aarch32CpuContext) == 0xE0, "Aarch32CpuContext definition!");
static_assert(sizeof(FatalCpuContext) == 0x250, "FatalCpuContext definition!");
static_assert(std::is_pod_v<FatalCpuContext>, "FatalCpuContext definition!");
