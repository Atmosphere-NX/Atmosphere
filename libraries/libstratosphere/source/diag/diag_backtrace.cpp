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
#include "impl/diag_get_all_backtrace.hpp"

namespace ams::diag {

    size_t GetBacktrace(uintptr_t *out, size_t out_size) {
        /* Validate pre-conditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(out_size > 0);

        /* Create the backtrace object. */
        ::ams::diag::Backtrace bt{};
        bt.Step();

        /* Get the backtrace. */
        return ::ams::diag::impl::GetAllBacktrace(out, out_size, bt);
    }

    #if defined(ATMOSPHERE_OS_HORIZON)
    size_t GetBacktrace(uintptr_t *out, size_t out_size, uintptr_t fp, uintptr_t sp, uintptr_t pc) {
        /* Validate pre-conditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(out_size > 0);

        /* Create the backtrace object. */
        ::ams::diag::Backtrace bt{fp, sp, pc};
        bt.Step();

        /* Get the backtrace. */
        return ::ams::diag::impl::GetAllBacktrace(out, out_size, bt);
    }
    #endif


}
