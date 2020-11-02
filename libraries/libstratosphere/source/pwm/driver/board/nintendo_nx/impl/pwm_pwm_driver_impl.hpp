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
#include <stratosphere.hpp>
#include "pwm_impl_pwm_driver_api.hpp"

namespace ams::pwm::driver::board::nintendo_nx::impl {

    class PwmDeviceImpl : public ::ams::pwm::driver::IPwmDevice {
        NON_COPYABLE(PwmDeviceImpl);
        NON_MOVEABLE(PwmDeviceImpl);
        AMS_DDSF_CASTABLE_TRAITS(ams::pwm::driver::board::nintendo_nx::impl::PwmDeviceImpl, ::ams::pwm::driver::IPwmDevice);
        private:
            os::SdkMutex suspend_mutex;
            u32 suspend_value;
        public:
            PwmDeviceImpl(int channel) : IPwmDevice(channel), suspend_mutex(), suspend_value() { /* ... */ }

            void SetSuspendValue(u32 v) { this->suspend_value = v; }
            u32 GetSuspendValue() const { return this->suspend_value; }

            void lock() { return this->suspend_mutex.lock(); }
            void unlock() { return this->suspend_mutex.unlock(); }
    };

    class PwmDriverImpl : public ::ams::pwm::driver::IPwmDriver {
        NON_COPYABLE(PwmDriverImpl);
        NON_MOVEABLE(PwmDriverImpl);
        AMS_DDSF_CASTABLE_TRAITS(ams::pwm::driver::board::nintendo_nx::impl::PwmDriverImpl, ::ams::pwm::driver::IPwmDriver);
        private:
            dd::PhysicalAddress registers_phys_addr;
            size_t registers_size;
            const ChannelDefinition *channels;
            size_t num_channels;
            uintptr_t registers;
        private:
            ALWAYS_INLINE uintptr_t GetRegistersFor(IPwmDevice &device) {
                return registers + PWM_CONTROLLER_PWM_CHANNEL_OFFSET(device.GetChannelIndex());
            }

            ALWAYS_INLINE uintptr_t GetRegistersFor(IPwmDevice *device) {
                return this->GetRegistersFor(*device);
            }

            void PowerOn();
            void PowerOff();
        public:
            PwmDriverImpl(dd::PhysicalAddress paddr, size_t sz, const ChannelDefinition *c, size_t nsc);

            virtual void InitializeDriver() override;
            virtual void FinalizeDriver() override;

            virtual Result InitializeDevice(IPwmDevice *device) override;
            virtual void FinalizeDevice(IPwmDevice *device) override;

            virtual Result SetPeriod(IPwmDevice *device, TimeSpan period) override;
            virtual Result GetPeriod(TimeSpan *out, IPwmDevice *device) override;

            virtual Result SetDuty(IPwmDevice *device, int duty) override;
            virtual Result GetDuty(int *out, IPwmDevice *device) override;

            virtual Result SetScale(IPwmDevice *device, double scale) override;
            virtual Result GetScale(double *out, IPwmDevice *device) override;

            virtual Result SetEnabled(IPwmDevice *device, bool en) override;
            virtual Result GetEnabled(bool *out, IPwmDevice *device) override;

            virtual Result Suspend() override;
            virtual void Resume() override;
    };

}
