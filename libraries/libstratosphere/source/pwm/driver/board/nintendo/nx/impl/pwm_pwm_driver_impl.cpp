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
#include <stratosphere.hpp>
#include "pwm_pwm_driver_impl.hpp"

namespace ams::pwm::driver::board::nintendo::nx::impl {

    namespace {

        constexpr inline u32 PwmClockRateHz = 45'333'333;

        constexpr inline TimeSpan DefaultChannelPeriod = TimeSpan::FromMilliSeconds(10);

        constexpr inline int MaxDuty = 0x100;

        template<typename T>
        T DivideRoundUp(T a, T b) {
            return (a + (b / 2)) / b;
        }

    }

    PwmDriverImpl::PwmDriverImpl(dd::PhysicalAddress paddr, size_t sz, const ChannelDefinition *c, size_t nc) : registers_phys_addr(paddr), registers_size(sz), channels(c), num_channels(nc), registers(0) {
        /* ... */
    }

    void PwmDriverImpl::PowerOn() {
        /* Initialize pcv driver. */
        pcv::Initialize();

        /* Setup clock/power for pwm. */
        R_ABORT_UNLESS(pcv::SetReset(pcv::Module_Pwm, true));
        R_ABORT_UNLESS(pcv::SetClockEnabled(pcv::Module_Pwm, true));
        R_ABORT_UNLESS(pcv::SetClockRate(pcv::Module_Pwm, PwmClockRateHz));
        R_ABORT_UNLESS(pcv::SetReset(pcv::Module_Pwm, false));
    }

    void PwmDriverImpl::PowerOff() {
        /* Disable clock and hold pwm in reset. */
        /* NOTE: Nintendo does not check this succeeds. */
        pcv::SetClockEnabled(pcv::Module_Pwm, false);
        pcv::SetReset(pcv::Module_Pwm, true);

        /* Finalize pcv driver. */
        pcv::Finalize();
    }

    void PwmDriverImpl::InitializeDriver() {
        /* Get the registers virtual address. */
        this->registers = dd::QueryIoMapping(this->registers_phys_addr, this->registers_size);
        AMS_ABORT_UNLESS(this->registers != 0);

        /* Setup power to pwm. */
        this->PowerOn();
    }

    void PwmDriverImpl::FinalizeDriver() {
        /* Shut down power to pwm. */
        this->PowerOff();
    }

    Result PwmDriverImpl::InitializeDevice(IPwmDevice *device) {
        /* Validate the device. */
        AMS_ASSERT(device != nullptr);

        /* Configure initial settings. */
        /* NOTE: None of these results are checked. */
        this->SetEnabled(device, false);
        this->SetDuty(device, 0);
        this->SetPeriod(device, DefaultChannelPeriod);
        return ResultSuccess();
    }

    void PwmDriverImpl::FinalizeDevice(IPwmDevice *device) {
        /* Validate the device. */
        AMS_ASSERT(device != nullptr);

        /* Nothing to do here. */
        AMS_UNUSED(device);
    }

    Result PwmDriverImpl::SetPeriod(IPwmDevice *device, TimeSpan period) {
        /* Validate the device. */
        AMS_ASSERT(device != nullptr);

        /* Verify the period is valid. */
        const auto ns = period.GetNanoSeconds();
        R_UNLESS(ns > 0, pwm::ResultInvalidArgument());

        /* Convert the ns to a desired frequency (rounding up). */
        const auto hz = DivideRoundUp(TimeSpan::FromSeconds(1).GetNanoSeconds(), ns);
        R_UNLESS(hz > 0, pwm::ResultInvalidArgument());

        /* Convert the frequency to a pfm value. */
        const u32 pfm = std::min<u32>(std::max<u32>(DivideRoundUp<u64>(PwmClockRateHz, hz * 256), 1) - 1, 0x1FFF);

        /* Acquire exclusive access to the device registers. */
        std::scoped_lock lk(device->SafeCastTo<PwmDeviceImpl>());

        /* Update the period. */
        reg::ReadWrite(this->GetRegistersFor(device) + PWM_CONTROLLER_PWM_CSR, PWM_REG_BITS_VALUE(PWM_CSR_PFM, pfm));

        return ResultSuccess();
    }

    Result PwmDriverImpl::GetPeriod(TimeSpan *out, IPwmDevice *device) {
        /* Validate the device. */
        AMS_ASSERT(out    != nullptr);
        AMS_ASSERT(device != nullptr);

        /* Get the pfm value. */
        const u32 pfm = reg::GetValue(this->GetRegistersFor(device) + PWM_CONTROLLER_PWM_CSR, PWM_REG_BITS_MASK(PWM_CSR_PFM));

        /* Convert it to a frequency. */
        /* pfm = ((ClockRate / (hz * 256)) - 1) -> hz = (ClockRate / ((pfm + 1) * 256)) */
        const auto hz = DivideRoundUp<s64>(PwmClockRateHz, (pfm + 1) * 256);

        /* Convert the frequency to a period. */
        const auto ns = DivideRoundUp(TimeSpan::FromSeconds(1).GetNanoSeconds(), hz);

        /* Set the output. */
        *out = TimeSpan::FromNanoSeconds(ns);
        return ResultSuccess();
    }

    Result PwmDriverImpl::SetDuty(IPwmDevice *device, int duty) {
        /* Validate the device. */
        AMS_ASSERT(device != nullptr);

        /* Validate the duty. */
        R_UNLESS(0 <= duty && duty <= MaxDuty, pwm::ResultInvalidArgument());

        /* Acquire exclusive access to the device registers. */
        std::scoped_lock lk(device->SafeCastTo<PwmDeviceImpl>());

        /* Update the duty. */
        reg::ReadWrite(this->GetRegistersFor(device) + PWM_CONTROLLER_PWM_CSR, PWM_REG_BITS_VALUE(PWM_CSR_PWM, static_cast<u32>(duty)));

        return ResultSuccess();
    }

    Result PwmDriverImpl::GetDuty(int *out, IPwmDevice *device) {
        /* Validate the device. */
        AMS_ASSERT(out    != nullptr);
        AMS_ASSERT(device != nullptr);

        /* Get the duty. */
        *out = static_cast<int>(reg::GetValue(this->GetRegistersFor(device) + PWM_CONTROLLER_PWM_CSR, PWM_REG_BITS_MASK(PWM_CSR_PWM)));
        return ResultSuccess();
    }

    Result PwmDriverImpl::SetScale(IPwmDevice *device, double scale) {
        /* Validate the device. */
        AMS_ASSERT(device != nullptr);

        /* Convert the scale to a duty. */
        const int duty = static_cast<int>(((scale * 256.0) / 100.0) + 0.5);

        /* Set the duty. */
        return this->SetDuty(device, duty);
    }

    Result PwmDriverImpl::GetScale(double *out, IPwmDevice *device) {
        /* Validate the device. */
        AMS_ASSERT(out    != nullptr);
        AMS_ASSERT(device != nullptr);

        /* Get the duty. */
        const int duty = static_cast<int>(reg::GetValue(this->GetRegistersFor(device) + PWM_CONTROLLER_PWM_CSR, PWM_REG_BITS_MASK(PWM_CSR_PWM)));

        /* Convert to scale. */
        *out = (static_cast<double>(duty) * 100.0) / 256.0;
        return ResultSuccess();
    }

    Result PwmDriverImpl::SetEnabled(IPwmDevice *device, bool en) {
        /* Validate the device. */
        AMS_ASSERT(device != nullptr);

        /* Acquire exclusive access to the device registers. */
        std::scoped_lock lk(device->SafeCastTo<PwmDeviceImpl>());

        /* Update the enable. */
        reg::ReadWrite(this->GetRegistersFor(device) + PWM_CONTROLLER_PWM_CSR, PWM_REG_BITS_ENUM_SEL(PWM_CSR_ENB, en, ENABLE, DISABLE));

        return ResultSuccess();
    }

    Result PwmDriverImpl::GetEnabled(bool *out, IPwmDevice *device) {
        /* Validate the device. */
        AMS_ASSERT(out    != nullptr);
        AMS_ASSERT(device != nullptr);

        /* Get the enable. */
        *out = reg::HasValue(this->GetRegistersFor(device) + PWM_CONTROLLER_PWM_CSR, PWM_REG_BITS_ENUM(PWM_CSR_ENB, ENABLE));
        return ResultSuccess();
    }

    Result PwmDriverImpl::Suspend() {
        /* Suspend each device. */
        this->ForEachDevice([&](ddsf::IDevice &device) -> bool {
            /* Convert the device to a pwm device. */
            auto &pwm_device = device.SafeCastTo<PwmDeviceImpl>();

            /* Cache the suspend value. */
            pwm_device.SetSuspendValue(reg::Read(this->GetRegistersFor(pwm_device) + PWM_CONTROLLER_PWM_CSR));

            /* Acquire exclusive access to the device. */
            std::scoped_lock lk(pwm_device);

            /* Disable the device. */
            reg::ReadWrite(this->GetRegistersFor(pwm_device) + PWM_CONTROLLER_PWM_CSR, PWM_REG_BITS_ENUM(PWM_CSR_ENB, DISABLE));

            /* Continue to the next device. */
            return true;
        });

        /* Disable clock to pwm. */
        return pcv::SetClockEnabled(pcv::Module_Pwm, false);
    }

    void PwmDriverImpl::Resume() {
        /* Power on. */
        this->PowerOn();

        /* Resume each device. */
        this->ForEachDevice([&](ddsf::IDevice &device) -> bool {
            /* Convert the device to a pwm device. */
            auto &pwm_device = device.SafeCastTo<PwmDeviceImpl>();

            /* Acquire exclusive access to the device. */
            std::scoped_lock lk(pwm_device);

            /* Write the device's suspend value. */
            reg::Write(this->GetRegistersFor(pwm_device) + PWM_CONTROLLER_PWM_CSR, pwm_device.GetSuspendValue());

            /* Continue to the next device. */
            return true;
        });
    }

}
