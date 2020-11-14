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

    class Pad;

    class IGpioDriver : public ::ams::ddsf::IDriver {
        NON_COPYABLE(IGpioDriver);
        NON_MOVEABLE(IGpioDriver);
        AMS_DDSF_CASTABLE_TRAITS(ams::gpio::driver::IGpioDriver, ::ams::ddsf::IDriver);
        public:
            IGpioDriver() : IDriver() { /* ... */ }
            virtual ~IGpioDriver() { /* ... */ }

            virtual void InitializeDriver() = 0;
            virtual void FinalizeDriver()   = 0;

            virtual Result InitializePad(Pad *pad) = 0;
            virtual void FinalizePad(Pad *pad)     = 0;

            virtual Result GetDirection(Direction *out, Pad *pad) const = 0;
            virtual Result SetDirection(Pad *pad, Direction direction)  = 0;

            virtual Result GetValue(GpioValue *out, Pad *pad) const = 0;
            virtual Result SetValue(Pad *pad, GpioValue value)      = 0;

            virtual Result GetInterruptMode(InterruptMode *out, Pad *pad) const = 0;
            virtual Result SetInterruptMode(Pad *pad, InterruptMode mode)       = 0;

            virtual Result SetInterruptEnabled(Pad *pad, bool en) = 0;

            virtual Result GetInterruptStatus(InterruptStatus *out, Pad *pad) = 0;
            virtual Result ClearInterruptStatus(Pad *pad)                     = 0;

            virtual os::SdkMutex &GetInterruptControlMutex(const Pad &pad) const = 0;

            virtual Result GetDebounceEnabled(bool *out, Pad *pad) const = 0;
            virtual Result SetDebounceEnabled(Pad *pad, bool en)         = 0;

            virtual Result GetDebounceTime(s32 *out_ms, Pad *pad) const = 0;
            virtual Result SetDebounceTime(Pad *pad, s32 ms)            = 0;

            virtual Result GetUnknown22(u32 *out) = 0;
            virtual void Unknown23();

            virtual Result SetValueForSleepState(Pad *pad, GpioValue value) = 0;
            virtual Result IsWakeEventActive(bool *out, Pad *pad) const = 0;
            virtual Result SetWakeEventActiveFlagSetForDebug(Pad *pad, bool en) = 0;
            virtual Result SetWakePinDebugMode(WakePinDebugMode mode) = 0;

            virtual Result Suspend()    = 0;
            virtual Result SuspendLow() = 0;
            virtual Result Resume()     = 0;
            virtual Result ResumeLow()  = 0;
    };

}
