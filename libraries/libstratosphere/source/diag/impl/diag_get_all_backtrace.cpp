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

namespace ams::diag::impl {

    namespace {

        constinit uintptr_t g_abort_impl_return_address = std::numeric_limits<uintptr_t>::max();

    }

    void SetAbortImplReturnAddress(uintptr_t address) {
        g_abort_impl_return_address = address;
    }

    size_t GetAllBacktrace(uintptr_t *out, size_t out_size, ::ams::diag::Backtrace &bt) {
        size_t count = 0;
        do {
            /* Check that we can write another return address. */
            if (count >= out_size) {
                break;
            }

            /* Get the current return address. */
            const uintptr_t ret_addr = bt.GetReturnAddress();

            /* If it's abort impl, reset the trace we're writing. */
            if (ret_addr == g_abort_impl_return_address) {
                count = 0;
            }

            /* Set the output pointer. */
            out[count++] = ret_addr;
        } while (bt.Step());

        /* Return the number of addresses written. */
        return count;
    }

}
