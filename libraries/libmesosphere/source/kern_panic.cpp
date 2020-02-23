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
#include <mesosphere.hpp>

extern "C" void _start();

namespace ams::result::impl {

    NORETURN void OnResultAssertion(Result result) {
        MESOSPHERE_PANIC("OnResultAssertion(2%03d-%04d)", result.GetModule(), result.GetDescription());
    }

}

namespace ams::kern {

    namespace {

        constexpr std::array<s32, cpu::NumCores> NegativeArray = [] {
            std::array<s32, cpu::NumCores> arr = {};
            for (size_t i = 0; i < arr.size(); i++) {
                arr[i] = -1;
            }
            return arr;
        }();

        std::atomic<s32> g_next_ticket    = 0;
        std::atomic<s32> g_current_ticket = 0;

        std::array<s32, cpu::NumCores> g_core_tickets = NegativeArray;

        s32 GetCoreTicket() {
            const s32 core_id = GetCurrentCoreId();
            if (g_core_tickets[core_id] == -1) {
                g_core_tickets[core_id] = 2 * g_next_ticket.fetch_add(1);
            }
            return g_core_tickets[core_id];
        }

        void WaitCoreTicket() {
            const s32 expected = GetCoreTicket();
            const s32 desired  = expected + 1;
            s32 compare = g_current_ticket;
            do {
                if (compare == desired) {
                    break;
                }
                compare = expected;
            } while (!g_current_ticket.compare_exchange_weak(compare, desired));
        }

        void ReleaseCoreTicket() {
            const s32 expected = GetCoreTicket() + 1;
            const s32 desired  = expected + 1;
            s32 compare = g_current_ticket;
            do {
                if (compare != expected) {
                    break;
                }
            } while (!g_current_ticket.compare_exchange_weak(compare, desired));
        }

        [[gnu::unused]] void PrintCurrentState() {
            /* Wait for it to be our turn to print. */
            WaitCoreTicket();

            const s32 core_id = GetCurrentCoreId();
            MESOSPHERE_RELEASE_LOG("Core[%d] Current State:\n", core_id);

            /* TODO: Dump register state. */

            #ifdef ATMOSPHERE_ARCH_ARM64
                MESOSPHERE_RELEASE_LOG("    Backtrace:\n");
                uintptr_t fp = reinterpret_cast<uintptr_t>(__builtin_frame_address(0));
                for (size_t i = 0; i < 32 && fp && util::IsAligned(fp, 0x10) && cpu::GetPhysicalAddressWritable(nullptr, fp, true); i++) {
                    struct {
                        uintptr_t fp;
                        uintptr_t lr;
                    } *stack_frame = reinterpret_cast<decltype(stack_frame)>(fp);
                    MESOSPHERE_RELEASE_LOG("        [%02zx]: %p\n", i, reinterpret_cast<void *>(stack_frame->lr));
                    fp = stack_frame->fp;
                }
            #endif

            MESOSPHERE_RELEASE_LOG("\n");

            /* Allow the next core to print. */
            ReleaseCoreTicket();
        }

        NORETURN void StopSystem() {
            #ifdef MESOSPHERE_BUILD_FOR_DEBUGGING
                /* Print the current core. */
                PrintCurrentState();
            #endif

            KSystemControl::StopSystem();
        }

    }

    NORETURN WEAK_SYMBOL void Panic(const char *file, int line, const char *format, ...) {
        #ifdef MESOSPHERE_BUILD_FOR_DEBUGGING
            /* Wait for it to be our turn to print. */
            WaitCoreTicket();

            ::std::va_list vl;
            va_start(vl, format);
            MESOSPHERE_RELEASE_LOG("Core[%d]: Kernel Panic at %s:%d\n", GetCurrentCoreId(), file, line);
            MESOSPHERE_RELEASE_VLOG(format, vl);
            MESOSPHERE_RELEASE_LOG("\n");
            va_end(vl);
        #endif

        StopSystem();
    }

    NORETURN WEAK_SYMBOL void Panic() {
        StopSystem();
    }

}
