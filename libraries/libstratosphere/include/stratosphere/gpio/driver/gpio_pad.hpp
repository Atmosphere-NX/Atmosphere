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
#include <vapours.hpp>
#include <stratosphere/gpio/gpio_types.hpp>
#include <stratosphere/ddsf.hpp>

namespace ams::gpio::driver {

    class Pad : public ::ams::ddsf::IDevice {
        NON_COPYABLE(Pad);
        NON_MOVEABLE(Pad);
        AMS_DDSF_CASTABLE_TRAITS(ams::gpio::driver::Pad, ::ams::ddsf::IDevice);
        private:
            int pad_number;
            bool is_interrupt_enabled;
        public:
            explicit Pad(int pad) : IDevice(true), pad_number(pad), is_interrupt_enabled(false) { /* ... */ }

            Pad() : Pad(0) { /* ... */ }

            virtual ~Pad() { /* ... */ }

            int GetPadNumber() const {
                return this->pad_number;
            }

            void SetPadNumber(int p) {
                this->pad_number = p;
            }

            bool IsInterruptEnabled() const {
                return this->is_interrupt_enabled;
            }

            void SetInterruptEnabled(bool en) {
                this->is_interrupt_enabled = en;
            }

            bool IsInterruptRequiredForDriver() const {
                return this->IsInterruptEnabled() && this->IsAnySessionBoundToInterrupt();
            }

            bool IsAnySessionBoundToInterrupt() const;
            void SignalInterruptBoundEvent();
    };

}
