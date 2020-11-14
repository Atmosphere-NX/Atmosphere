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
#include <stratosphere/pwm/pwm_types.hpp>
#include <stratosphere/pwm/driver/pwm_i_pwm_device.hpp>
#include <stratosphere/ddsf.hpp>

namespace ams::pwm::driver {

    class IPwmDriver : public ::ams::ddsf::IDriver {
        NON_COPYABLE(IPwmDriver);
        NON_MOVEABLE(IPwmDriver);
        AMS_DDSF_CASTABLE_TRAITS(ams::pwm::driver::IPwmDriver, ::ams::ddsf::IDriver);
        public:
            IPwmDriver() : IDriver() { /* ... */ }
            virtual ~IPwmDriver() { /* ... */ }

            virtual void InitializeDriver() = 0;
            virtual void FinalizeDriver()   = 0;

            virtual Result InitializeDevice(IPwmDevice *device) = 0;
            virtual void FinalizeDevice(IPwmDevice *device)     = 0;

            virtual Result SetPeriod(IPwmDevice *device, TimeSpan period) = 0;
            virtual Result GetPeriod(TimeSpan *out, IPwmDevice *device)   = 0;

            virtual Result SetDuty(IPwmDevice *device, int duty) = 0;
            virtual Result GetDuty(int *out, IPwmDevice *device) = 0;

            virtual Result SetScale(IPwmDevice *device, double scale) = 0;
            virtual Result GetScale(double *out, IPwmDevice *device)  = 0;

            virtual Result SetEnabled(IPwmDevice *device, bool en)   = 0;
            virtual Result GetEnabled(bool *out, IPwmDevice *device) = 0;

            virtual Result Suspend() = 0;
            virtual void Resume()  = 0;
    };

}
