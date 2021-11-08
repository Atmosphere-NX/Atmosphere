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

        constinit svc::Handle g_read_handles[3] = { svc::InvalidHandle, svc::InvalidHandle, svc::InvalidHandle };
        constinit svc::Handle g_write_handles[3] = { svc::InvalidHandle, svc::InvalidHandle, svc::InvalidHandle };

        constinit s64 g_thread_wait_ns;
        constinit bool g_should_switch_threads;
        constinit bool g_switched_threads;
        constinit bool g_correct_switch_threads;

        void WaitSynchronization(svc::Handle handle) {
            s32 dummy;
            R_ABORT_UNLESS(svc::WaitSynchronization(std::addressof(dummy), std::addressof(handle), 1, -1));
        }

        void TestYieldHigherOrSamePriorityThread() {
            /* Wait to run. */
            WaitSynchronization(g_read_handles[0]);

            /* Reset our event. */
            R_ABORT_UNLESS(svc::ClearEvent(g_read_handles[0]));

            /* Signal the other thread's event. */
            R_ABORT_UNLESS(svc::SignalEvent(g_write_handles[1]));

            /* Wait, potentially yielding to the lower/same priority thread. */
            g_switched_threads = false;
            svc::SleepThread(g_thread_wait_ns);

            /* Check whether we switched correctly. */
            g_correct_switch_threads = g_should_switch_threads == g_switched_threads;

            /* Exit. */
            svc::ExitThread();
        }

        void TestYieldLowerOrSamePriorityThread() {
            /* Signal thread the higher/same priority thread to run. */
            R_ABORT_UNLESS(svc::SignalEvent(g_write_handles[0]));

            /* Wait to run. */
            WaitSynchronization(g_read_handles[1]);

            /* Reset our event. */
            R_ABORT_UNLESS(svc::ClearEvent(g_read_handles[1]));

            /* We've switched to the lower/same priority thread. */
            g_switched_threads = true;

            /* Wait to be instructed to exit. */
            WaitSynchronization(g_read_handles[2]);

            /* Reset the exit signal. */
            R_ABORT_UNLESS(svc::ClearEvent(g_read_handles[2]));

            /* Exit. */
            svc::ExitThread();
        }

        void TestYieldSamePriority(uintptr_t sp_higher, uintptr_t sp_lower) {
            /* Test each core. */
            for (s32 core = 0; core < NumCores; ++core) {
                for (s32 priority = HighestTestPriority; priority <= LowestTestPriority && !IsPreemptionPriority(core, priority); ++priority) {

                    svc::Handle thread_handles[2];

                    /* Create threads. */
                    DOCTEST_CHECK(R_SUCCEEDED(svc::CreateThread(thread_handles + 0, reinterpret_cast<uintptr_t>(&TestYieldHigherOrSamePriorityThread), 0, sp_higher, priority, core)));
                    DOCTEST_CHECK(R_SUCCEEDED(svc::CreateThread(thread_handles + 1, reinterpret_cast<uintptr_t>(&TestYieldLowerOrSamePriorityThread), 0, sp_lower, priority, core)));

                    /* Start threads. */
                    DOCTEST_CHECK(R_SUCCEEDED(svc::StartThread(thread_handles[1])));
                    DOCTEST_CHECK(R_SUCCEEDED(svc::StartThread(thread_handles[0])));

                    /* Wait for higher priority thread. */
                    WaitSynchronization(thread_handles[0]);
                    DOCTEST_CHECK(R_SUCCEEDED(svc::CloseHandle(thread_handles[0])));

                    /* Signal the lower priority thread to exit. */
                    DOCTEST_CHECK(R_SUCCEEDED(svc::SignalEvent(g_write_handles[2])));

                    /* Wait for the lower priority thread. */
                    WaitSynchronization(thread_handles[1]);
                    DOCTEST_CHECK(R_SUCCEEDED(svc::CloseHandle(thread_handles[1])));

                    /* Check that the switch was correct. */
                    DOCTEST_CHECK(g_correct_switch_threads);
                }
            }
        }

        void TestYieldDifferentPriority(uintptr_t sp_higher, uintptr_t sp_lower) {
            /* Test each core. */
            for (s32 core = 0; core < NumCores; ++core) {
                for (s32 priority = HighestTestPriority; priority < LowestTestPriority && !IsPreemptionPriority(core, priority); ++priority) {

                    svc::Handle thread_handles[2];

                    /* Create threads. */
                    DOCTEST_CHECK(R_SUCCEEDED(svc::CreateThread(thread_handles + 0, reinterpret_cast<uintptr_t>(&TestYieldHigherOrSamePriorityThread), 0, sp_higher, priority, core)));
                    DOCTEST_CHECK(R_SUCCEEDED(svc::CreateThread(thread_handles + 1, reinterpret_cast<uintptr_t>(&TestYieldLowerOrSamePriorityThread), 0, sp_lower, priority + 1, core)));

                    /* Start threads. */
                    DOCTEST_CHECK(R_SUCCEEDED(svc::StartThread(thread_handles[1])));
                    DOCTEST_CHECK(R_SUCCEEDED(svc::StartThread(thread_handles[0])));

                    /* Wait for higher priority thread. */
                    WaitSynchronization(thread_handles[0]);
                    DOCTEST_CHECK(R_SUCCEEDED(svc::CloseHandle(thread_handles[0])));

                    /* Signal the lower priority thread to exit. */
                    DOCTEST_CHECK(R_SUCCEEDED(svc::SignalEvent(g_write_handles[2])));

                    /* Wait for the lower priority thread. */
                    WaitSynchronization(thread_handles[1]);
                    DOCTEST_CHECK(R_SUCCEEDED(svc::CloseHandle(thread_handles[1])));

                    /* Check that the switch was correct. */
                    DOCTEST_CHECK(g_correct_switch_threads);
                }
            }
        }

    }

    DOCTEST_TEST_CASE( "svc::SleepThread: Thread sleeps for time specified" ) {
        for (s64 ns = 1; ns < TimeSpan::FromSeconds(1).GetNanoSeconds(); ns *= 2) {
            const auto start = os::GetSystemTickOrdered();
            svc::SleepThread(ns);
            const auto end   = os::GetSystemTickOrdered();

            const s64 taken_ns = (end - start).ToTimeSpan().GetNanoSeconds();
            DOCTEST_CHECK( taken_ns >= ns );
        }
    }

    DOCTEST_TEST_CASE( "svc::SleepThread: Yield is behaviorally correct" ) {
        /* Create events. */
        for (size_t i = 0; i < util::size(g_write_handles); ++i) {
            g_read_handles[i]  = svc::InvalidHandle;
            g_write_handles[i] = svc::InvalidHandle;
            DOCTEST_CHECK(R_SUCCEEDED(svc::CreateEvent(g_write_handles + i, g_read_handles + i)));
        }

        ON_SCOPE_EXIT {
            for (size_t i = 0; i < util::size(g_write_handles); ++i) {
                DOCTEST_CHECK(R_SUCCEEDED(svc::CloseHandle(g_read_handles[i])));
                DOCTEST_CHECK(R_SUCCEEDED(svc::CloseHandle(g_write_handles[i])));
                g_read_handles[i]  = svc::InvalidHandle;
                g_write_handles[i] = svc::InvalidHandle;
            }
        };

        /* Create heap. */
        ScopedHeap heap(3 * os::MemoryPageSize);
        DOCTEST_CHECK(R_SUCCEEDED(svc::SetMemoryPermission(heap.GetAddress() + os::MemoryPageSize, os::MemoryPageSize, svc::MemoryPermission_None)));
        ON_SCOPE_EXIT {
            DOCTEST_CHECK(R_SUCCEEDED(svc::SetMemoryPermission(heap.GetAddress() + os::MemoryPageSize, os::MemoryPageSize, svc::MemoryPermission_ReadWrite)));
        };
        const uintptr_t sp_higher = heap.GetAddress() + 1 * os::MemoryPageSize;
        const uintptr_t sp_lower  = heap.GetAddress() + 3 * os::MemoryPageSize;

        DOCTEST_SUBCASE("svc::SleepThread: Yields do not switch to a thread of lower priority.") {
            /* Test yield without migration. */
            {
                /* Configure for yield test. */
                g_should_switch_threads = false;
                g_thread_wait_ns        = static_cast<s64>(svc::YieldType_WithoutCoreMigration);

                TestYieldDifferentPriority(sp_higher, sp_lower);
            }

            /* Test yield with migration. */
            {
                /* Configure for yield test. */
                g_should_switch_threads = false;
                g_thread_wait_ns        = static_cast<s64>(svc::YieldType_WithoutCoreMigration);

                TestYieldDifferentPriority(sp_higher, sp_lower);
            }
        }

        DOCTEST_SUBCASE("svc::SleepThread: ToAnyThread switches to a thread of same or lower priority.") {
            /* Test to same priority. */
            {
                /* Configure for yield test. */
                g_should_switch_threads = true;
                g_thread_wait_ns        = static_cast<s64>(svc::YieldType_ToAnyThread);

                TestYieldSamePriority(sp_higher, sp_lower);
            }

            /* Test to lower priority. */
            {
                /* Configure for yield test. */
                g_should_switch_threads = true;
                g_thread_wait_ns        = static_cast<s64>(svc::YieldType_ToAnyThread);

                TestYieldDifferentPriority(sp_higher, sp_lower);
            }
        }

        DOCTEST_SUBCASE("svc::SleepThread: Yield switches to another thread of same priority.") {
            /* Test yield without migration. */
            {
                /* Configure for yield test. */
                g_should_switch_threads = true;
                g_thread_wait_ns        = static_cast<s64>(svc::YieldType_WithoutCoreMigration);

                TestYieldSamePriority(sp_higher, sp_lower);
            }

            /* Test yield with migration. */
            {
                /* Configure for yield test. */
                g_should_switch_threads = true;
                g_thread_wait_ns        = static_cast<s64>(svc::YieldType_WithCoreMigration);

                TestYieldSamePriority(sp_higher, sp_lower);
            }
        }

        DOCTEST_SUBCASE("svc::SleepThread: Yield with bogus timeout does not switch to another thread same priority") {
            /* Configure for yield test. */
            g_should_switch_threads = false;
            g_thread_wait_ns        = INT64_C(-5);

            TestYieldSamePriority(sp_higher, sp_lower);
        }
    }

}