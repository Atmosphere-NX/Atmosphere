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
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_hardware_timer_base.hpp>

namespace ams::kern::arm64 {

    class KHardwareTimer : public KHardwareTimerBase {
        public:
            static constexpr s32 InterruptId = 30; /* Nintendo uses the non-secure timer interrupt. */
        public:
            constexpr KHardwareTimer() : KHardwareTimerBase() { /* ... */ }

            virtual void DoTask() override;

            /* TODO: Actually implement more of KHardwareTimer, */
    };

}
