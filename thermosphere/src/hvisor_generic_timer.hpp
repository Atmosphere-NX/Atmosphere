/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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
#include "defines.hpp"

#include "hvisor_i_interrupt_task.hpp"
#include "cpu/hvisor_cpu_sysreg_general.hpp"

#include "preprocessor.h"
#include "platform/interrupt_config.h"

#include <chrono>

namespace ams::hvisor {

    class GenericTimer final : public IInterruptTask {
        SINGLETON(GenericTimer);
        private:
            static constexpr u32 irqId = GIC_IRQID_NS_PHYS_HYP_TIMER;

        private:
            static void Configure(bool enabled, bool interruptMasked)
            {
                u64 ebit = enabled          ? cpu::CNTCTL_ENABLE : 0;
                u64 mbit = interruptMasked  ? cpu::CNTCTL_IMASK: 0;
                THERMOSPHERE_SET_SYSREG(cnthp_ctl_el2, mbit | ebit);
            }

            static bool GetInterruptStatus()
            {
                return (THERMOSPHERE_GET_SYSREG(cnthp_ctl_el2) & cpu::CNTCTL_ISTATUS) != 0;
            }

        private:
            u64 m_timerFreq = 0;

        private:
            constexpr GenericTimer() = default;

        public:
            static s64 GetSystemTick()
            {
                return static_cast<s64>(THERMOSPHERE_GET_SYSREG(cntpct_el0));
            }

            static void SetTimeoutTicks(s64 ticks)
            {
                THERMOSPHERE_SET_SYSREG(cnthp_cval_el2, GetSystemTick() + ticks);
                Configure(true, false);
            }

            static void WaitTicks(s64 ticks);

        public:
            void Initialize();
            std::optional<bool> InterruptTopHalfHandler(u32 irqId, u32) final;

            constexpr u64 GetTimerFrequency() const { return m_timerFreq; }

            template<typename SecondRatio = std::ratio<1>>
            auto GetSystemTime() const
            {
                auto tick = GetSystemTick();
                return (tick * SecondRatio::den) / (m_timerFreq * SecondRatio::num);
            }

            std::chrono::nanoseconds GetSystemTimeNs() const
            {
                return std::chrono::nanoseconds{GetSystemTime<std::nano>()};
            }

            template<typename Duration>
            void SetTimeout(Duration d) const
            {
                using SecondRatio = typename Duration::period;
                auto v = (d.count() * m_timerFreq * SecondRatio::num) / SecondRatio::den;
                SetTimeoutTicks(v);
            }

            template<typename Duration>
            void Wait(Duration d) const
            {
                using SecondRatio = typename Duration::period;
                auto v = (d.count() * m_timerFreq * SecondRatio::num) / SecondRatio::den;
                WaitTicks(v);
            }
    };

}
