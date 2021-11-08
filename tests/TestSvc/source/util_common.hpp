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
#pragma once
#include "util_test_framework.hpp"

namespace ams::test {

    static constexpr s32 NumCores                           =  4;
    static constexpr s32 DpcManagerNormalThreadPriority     = 59;
    static constexpr s32 DpcManagerPreemptionThreadPriority = 63;

    static constexpr s32 HighestTestPriority = 32;
    static constexpr s32 LowestTestPriority  = svc::LowestThreadPriority;
    static_assert(HighestTestPriority < LowestTestPriority);

    static constexpr TimeSpan PreemptionTimeSpan = TimeSpan::FromMilliSeconds(10);

    constexpr inline bool IsPreemptionPriority(s32 core, s32 priority) {
        return priority == ((core == (NumCores - 1)) ? DpcManagerPreemptionThreadPriority : DpcManagerNormalThreadPriority);
    }

    template<typename F>
    void DoWithThreadPinning(F f) {
        /* Get the thread local region. */
        auto * const tlr = svc::GetThreadLocalRegion();

        /* Require that we're not currently pinned. */
        DOCTEST_CHECK((tlr->disable_count == 0));
        DOCTEST_CHECK((!tlr->interrupt_flag));

        /* Request to pin ourselves. */
        tlr->disable_count = 1;

        /* Wait long enough that we can be confident preemption will occur, and therefore our interrupt flag will be set. */
        {
            constexpr auto MinimumTicksToGuaranteeInterruptFlag = ::ams::svc::Tick(PreemptionTimeSpan) + ::ams::svc::Tick(PreemptionTimeSpan) + 2;

            auto GetSystemTickForPinnedThread = []() ALWAYS_INLINE_LAMBDA -> ::ams::svc::Tick {
                s64 v;
                __asm__ __volatile__ ("mrs %x[v], cntpct_el0" : [v]"=r"(v) :: "memory");
                return ::ams::svc::Tick(v);
            };

            const auto start_tick = GetSystemTickForPinnedThread();
            while (true) {
                if (tlr->interrupt_flag) {
                    break;
                }

                if (const auto cur_tick = GetSystemTickForPinnedThread(); (cur_tick - start_tick) > MinimumTicksToGuaranteeInterruptFlag) {
                    break;
                }
            }
        }

        /* We're pinned. Execute the user callback. */
        bool callback_succeeded = true;
        {
            if constexpr (requires { { f() } -> std::convertible_to<bool>; }) {
                callback_succeeded = f();
            } else {
                f();
            }
        }

        /* Clear our disable count. */
        tlr->disable_count = 0;

        /* Get our interrupt flag. */
        const auto interrupt_flag_while_pinned = tlr->interrupt_flag;

        /* Unpin ourselves. */
        if (interrupt_flag_while_pinned) {
            svc::SynchronizePreemptionState();
        }

        /* Get our interrupt flag. */
        const auto interrupt_flag_after_unpin = tlr->interrupt_flag;

        /* We have access to all SVCs again. Check that our pinning happened as expected. */
        DOCTEST_CHECK(interrupt_flag_while_pinned);
        DOCTEST_CHECK(!interrupt_flag_after_unpin);

        /* Check that our callback succeeded. */
        DOCTEST_CHECK(callback_succeeded);
    }

}