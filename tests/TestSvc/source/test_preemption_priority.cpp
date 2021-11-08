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

    namespace {

        constinit volatile bool g_spinloop;

        void TestPreemptionPriorityThreadFunction(volatile bool *executed) {
            /* While we should, note that we're executing. */
            while (g_spinloop) {
                __asm__ __volatile__("" ::: "memory");
                *executed = true;
                __asm__ __volatile__("" ::: "memory");
            }

            /* Exit the thread. */
            svc::ExitThread();
        }

    }

    DOCTEST_TEST_CASE( "The scheduler is preemptive at the preemptive priority and cooperative for all other priorities" ) {
        /* Create heap. */
        ScopedHeap heap(3 * os::MemoryPageSize);
        DOCTEST_CHECK(R_SUCCEEDED(svc::SetMemoryPermission(heap.GetAddress() + os::MemoryPageSize, os::MemoryPageSize, svc::MemoryPermission_None)));
        ON_SCOPE_EXIT {
            DOCTEST_CHECK(R_SUCCEEDED(svc::SetMemoryPermission(heap.GetAddress() + os::MemoryPageSize, os::MemoryPageSize, svc::MemoryPermission_ReadWrite)));
        };
        const uintptr_t sp_0 = heap.GetAddress() + 1 * os::MemoryPageSize;
        const uintptr_t sp_1 = heap.GetAddress() + 3 * os::MemoryPageSize;

        for (s32 core = 0; core < NumCores; ++core) {
            for (s32 priority = HighestTestPriority; priority <= LowestTestPriority; ++priority) {
                    svc::Handle thread_handles[2];
                    volatile bool thread_executed[2] = { false, false };

                    /* Start spinlooping. */
                    g_spinloop = true;

                    /* Create threads. */
                    DOCTEST_CHECK(R_SUCCEEDED(svc::CreateThread(thread_handles + 0, reinterpret_cast<uintptr_t>(&TestPreemptionPriorityThreadFunction), reinterpret_cast<uintptr_t>(thread_executed + 0), sp_0, priority, core)));
                    DOCTEST_CHECK(R_SUCCEEDED(svc::CreateThread(thread_handles + 1, reinterpret_cast<uintptr_t>(&TestPreemptionPriorityThreadFunction), reinterpret_cast<uintptr_t>(thread_executed + 1), sp_1, priority, core)));

                    /* Start threads. */
                    DOCTEST_CHECK(R_SUCCEEDED(svc::StartThread(thread_handles[0])));
                    DOCTEST_CHECK(R_SUCCEEDED(svc::StartThread(thread_handles[1])));

                    /* Wait long enough that we can be confident the threads have been balanced. */
                    svc::SleepThread(PreemptionTimeSpan.GetNanoSeconds() * 10);

                    /* Check that we're in a coherent state. */
                    if (IsPreemptionPriority(core, priority)) {
                        DOCTEST_CHECK((thread_executed[0] & thread_executed[1]));
                    } else {
                        DOCTEST_CHECK((thread_executed[0] ^ thread_executed[1]));
                    }

                    /* Stop spinlooping. */
                    g_spinloop = false;

                    /* Wait for threads to exit. */
                    s32 dummy;
                    DOCTEST_CHECK(R_SUCCEEDED(svc::WaitSynchronization(std::addressof(dummy), thread_handles + 0, 1, -1)));
                    DOCTEST_CHECK(R_SUCCEEDED(svc::WaitSynchronization(std::addressof(dummy), thread_handles + 1, 1, -1)));

                    /* Close thread handles. */
                    DOCTEST_CHECK(R_SUCCEEDED(svc::CloseHandle(thread_handles[0])));
                    DOCTEST_CHECK(R_SUCCEEDED(svc::CloseHandle(thread_handles[1])));
            }
        }
    }

}