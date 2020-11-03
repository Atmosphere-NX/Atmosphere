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
#include <stratosphere/ddsf.hpp>

namespace ams::pwm::driver {

    class IPwmDevice : public ::ams::ddsf::IDevice {
        NON_COPYABLE(IPwmDevice);
        NON_MOVEABLE(IPwmDevice);
        AMS_DDSF_CASTABLE_TRAITS(ams::pwm::driver::IPwmDevice, ::ams::ddsf::IDevice);
        private:
            int channel_index;
        public:
            IPwmDevice(int id) : IDevice(false), channel_index(id) { /* ... */ }
            virtual ~IPwmDevice() { /* ... */ }

            constexpr int GetChannelIndex() const { return this->channel_index; }
    };

}
