/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere.hpp>
#include "diag_dump_stack_trace.hpp"

namespace ams::diag::impl {

    void TentativeDumpStackTrace() {
        AMS_SDK_LOG("----------------Stack Trace----------------\n");
        {
            /* Get the backtrace. */
            constexpr size_t MaxBackTraceSize = 0x40;
            uintptr_t backtrace[MaxBackTraceSize];
            const size_t num_items = ::ams::diag::GetBacktrace(backtrace, MaxBackTraceSize);

            /* Print each item. */
            for (size_t i = 0; i < num_items; ++i) {
                char symbol_name[0x200];
                if (const uintptr_t symbol_base = ::ams::diag::GetSymbolName(symbol_name, sizeof(symbol_name), backtrace[i] - 1); symbol_base != 0) {
                    AMS_SDK_LOG("0x%016" PRIX64 " [ %s+0x%" PRIX64 " ]\n", static_cast<u64>(backtrace[i]), symbol_name, static_cast<u64>(backtrace[i] - symbol_base));
                } else {
                    AMS_SDK_LOG("0x%016" PRIX64 " [ unknown ]\n", static_cast<u64>(backtrace[i]));
                }
            }
        }
        AMS_SDK_LOG("-------------------------------------------\n");
    }

}
