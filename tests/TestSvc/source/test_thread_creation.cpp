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
#include "util_common.hpp"
#include "util_scoped_heap.hpp"

namespace ams::test {

    void TestThreadCreateRegistersOnFunctionEntry(void *ctx);

    DOCTEST_TEST_CASE( "Creating a thread results in fixed register contents." ) {
        /* Create heap. */
        ScopedHeap heap(os::MemoryPageSize);

        /* Create register buffer. */
        u64 thread_registers[32];
        std::memset(thread_registers, 0xCC, sizeof(thread_registers));

        /* Create thread. */
        svc::Handle thread_handle;
        DOCTEST_CHECK(R_SUCCEEDED(svc::CreateThread(std::addressof(thread_handle), reinterpret_cast<uintptr_t>(&TestThreadCreateRegistersOnFunctionEntry), reinterpret_cast<uintptr_t>(thread_registers), heap.GetAddress() + os::MemoryPageSize, HighestTestPriority, NumCores - 1)));

        /* Start thread. */
        DOCTEST_CHECK(R_SUCCEEDED(svc::StartThread(thread_handle)));

        /* Wait for thread to exit. */
        s32 dummy;
        DOCTEST_CHECK(R_SUCCEEDED(svc::WaitSynchronization(std::addressof(dummy), std::addressof(thread_handle), 1, -1)));

        /* Close thread handle. */
        DOCTEST_CHECK(R_SUCCEEDED(svc::CloseHandle(thread_handle)));

        /* Check thread initial registers. */
        for (size_t i = 0; i < util::size(thread_registers); ++i) {
            if (i == 0) {
                /* X0 is argument. */
                DOCTEST_CHECK(thread_registers[i] == reinterpret_cast<uintptr_t>(thread_registers));
            } else if (i == 18) {
                /* X18 is an odd cfi value. */
                DOCTEST_CHECK(thread_registers[i] != 0);
                DOCTEST_CHECK((thread_registers[i] & 0x1) != 0);
            } else if (i == 31) {
                /* SP is user-provided sp. */
                DOCTEST_CHECK(thread_registers[i] == (heap.GetAddress() + os::MemoryPageSize));
            } else {
                /* All other registers are zero. */
                DOCTEST_CHECK(thread_registers[i] == 0);
            }
        }
    }

}