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
#include <stratosphere.hpp>

#include "gpio_tegra_pad.hpp"

namespace ams::gpio::driver::board::nintendo::nx::impl {

    class SuspendHandler {
        NON_COPYABLE(SuspendHandler);
        NON_MOVEABLE(SuspendHandler);
        private:
            struct RegisterValues {
                u16 conf;
                u8  oe;
                u8  out;
                u8  int_enb;
                u32 int_lvl;
                u8  db_ctrl;
                u8  db_cnt;

                void Reset() {
                    this->conf    = 0;
                    this->oe      = 0;
                    this->out     = 0;
                    this->int_enb = 0;
                    this->int_lvl = 0;
                    this->db_ctrl = 0;
                    this->db_cnt  = 0;
                }
            };

            struct ValuesForSleepState {
                u8 force_set;
                u8 out;

                void Reset() {
                    this->force_set = 0;
                    this->out       = 0;
                }
            };
        private:
            ddsf::IDriver &m_driver;
            uintptr_t m_gpio_virtual_address;
            RegisterValues m_register_values[GpioPadPort_Count];
            ValuesForSleepState m_values_for_sleep_state[GpioPadPort_Count];
        private:
            uintptr_t GetGpioVirtualAddress() const {
                AMS_ASSERT(m_gpio_virtual_address != 0);
                return m_gpio_virtual_address;
            }
        public:
            explicit SuspendHandler(ddsf::IDriver *drv) : m_driver(*drv), m_gpio_virtual_address(0) {
                for (auto &rv : m_register_values) {
                    rv.Reset();
                }
                for (auto &v : m_values_for_sleep_state) {
                    v.Reset();
                }
            }

            void Initialize(uintptr_t gpio_vaddr);

            void SetValueForSleepState(TegraPad *pad, GpioValue value);
            Result IsWakeEventActive(bool *out, TegraPad *pad) const;
            Result SetWakeEventActiveFlagSetForDebug(TegraPad *pad, bool en);
            void SetWakePinDebugMode(WakePinDebugMode mode);

            void Suspend();
            void SuspendLow();
            void Resume();
            void ResumeLow();
    };

}
