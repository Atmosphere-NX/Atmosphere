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
#pragma once
#include <vapours/includes.hpp>
#include <vapours/defines.hpp>

namespace ams::impl {

    struct FatalErrorContext {
        static constexpr size_t MaxStackTrace = 0x20;
        static constexpr size_t MaxStackDumpSize = 0x100;
        static constexpr size_t ThreadLocalSize = 0x100;
        static constexpr size_t NumGprs = 29;
        static constexpr uintptr_t StdAbortMagicAddress = 0x8;
        static constexpr u64       StdAbortMagicValue = 0xA55AF00DDEADCAFEul;
        static constexpr u32       StdAbortErrorDesc = 0xFFE;
        static constexpr u32       StackOverflowErrorDesc = 0xFFD;
        static constexpr u32       KernelPanicDesc = 0xF00;
        static constexpr u32       DataAbortErrorDesc = 0x101;
        static constexpr u32       Magic = util::FourCC<'A', 'F', 'E', '2'>::Code;

        u32 magic;
        u32 error_desc;
        u64 program_id;
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
        u64 stack_trace[MaxStackTrace];
        u8  stack_dump[MaxStackDumpSize];
        u8  tls[ThreadLocalSize];
    };

    static_assert(sizeof(FatalErrorContext) == 0x450);
    static_assert(std::is_standard_layout<FatalErrorContext>::value);
    static_assert(std::is_trivial<FatalErrorContext>::value);

}
