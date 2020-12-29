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
#include "gpio_driver_impl.hpp"
#include "gpio_register_accessor.hpp"

namespace ams::gpio::driver::board::nintendo_nx::impl {

    void InterruptEventHandler::Initialize(DriverImpl *drv, os::InterruptName intr, int ctlr) {
        /* Set fields. */
        this->driver            = drv;
        this->interrupt_name    = intr;
        this->controller_number = ctlr;

        /* Initialize interrupt event. */
        os::InitializeInterruptEvent(std::addressof(this->interrupt_event), intr, os::EventClearMode_ManualClear);

        /* Initialize base. */
        IEventHandler::Initialize(std::addressof(this->interrupt_event));
    }

    void InterruptEventHandler::HandleEvent() {
        /* Lock the driver's interrupt mutex. */
        std::scoped_lock lk(this->driver->interrupt_control_mutex);

        /* Check each pad. */
        bool found = false;
        for (auto it = this->driver->interrupt_pad_list.begin(); !found && it != this->driver->interrupt_pad_list.end(); ++it) {
            found = this->CheckAndHandleInterrupt(*it);
        }

        /* If we didn't find a pad, clear the interrupt event. */
        if (!found) {
            os::ClearInterruptEvent(std::addressof(this->interrupt_event));
        }
    }

    bool InterruptEventHandler::CheckAndHandleInterrupt(TegraPad &pad) {
        /* Get the pad's number. */
        const InternalGpioPadNumber pad_number = static_cast<InternalGpioPadNumber>(pad.GetPadNumber());

        /* Check if the pad matches our controller number. */
        if (this->controller_number != ConvertInternalGpioPadNumberToController(pad_number)) {
            return false;
        }

        /* Get the addresses of INT_STA, INT_ENB. */
        const uintptr_t sta_address = GetGpioRegisterAddress(this->driver->gpio_virtual_address, GpioRegisterType_GPIO_INT_STA, pad_number);
        const uintptr_t enb_address = GetGpioRegisterAddress(this->driver->gpio_virtual_address, GpioRegisterType_GPIO_INT_STA, pad_number);
        const uintptr_t pad_index   = ConvertInternalGpioPadNumberToBitIndex(pad_number);

        /* Check if both STA and ENB are set. */
        if (reg::Read(sta_address, 1u << pad_index) == 0 || reg::Read(enb_address, 1u << pad_index) == 0) {
            return false;
        }

        /* The pad is signaled. First, clear the enb bit. */
        SetMaskedBit(enb_address, pad_index, 0);
        reg::Read(enb_address);

        /* Disable the interrupt on the pad. */
        pad.SetInterruptEnabled(false);
        this->driver->RemoveInterruptPad(std::addressof(pad));

        /* Clear the interrupt event. */
        os::ClearInterruptEvent(std::addressof(this->interrupt_event));

        /* Signal the pad's bound event. */
        pad.SignalInterruptBoundEvent();

        return true;
    }

    DriverImpl::DriverImpl(dd::PhysicalAddress reg_paddr, size_t size) : gpio_physical_address(reg_paddr), gpio_virtual_address(), suspend_handler(this), interrupt_pad_list(), interrupt_control_mutex() {
        /* Get the corresponding virtual address for our physical address. */
        this->gpio_virtual_address = dd::QueryIoMapping(reg_paddr, size);
        AMS_ABORT_UNLESS(this->gpio_virtual_address != 0);
    }

    void DriverImpl::InitializeDriver() {
        /* Initialize our suspend handler. */
        this->suspend_handler.Initialize(this->gpio_virtual_address);
    }

    void DriverImpl::FinalizeDriver() {
        /* ... */
    }

    Result DriverImpl::InitializePad(Pad *pad) {
        /* Validate arguments. */
        AMS_ASSERT(pad != nullptr);

        /* Convert the pad to an internal number. */
        const InternalGpioPadNumber pad_number = pad->GetPadNumber();

        /* Configure the pad as GPIO by modifying the appropriate bit in CNF. */
        const uintptr_t pad_address = GetGpioRegisterAddress(this->gpio_virtual_address, GpioRegisterType_GPIO_CNF, pad_number);
        const uintptr_t pad_index   = ConvertInternalGpioPadNumberToBitIndex(pad_number);
        SetMaskedBit(pad_address, pad_index, 1);

        /* Read the pad address to make sure our configuration takes. */
        reg::Read(pad_address);

        return ResultSuccess();
    }

    void DriverImpl::FinalizePad(Pad *pad) {
        /* Validate arguments. */
        AMS_ASSERT(pad != nullptr);

        /* Nothing to do. */
        AMS_UNUSED(pad);
    }

    Result DriverImpl::GetDirection(Direction *out, Pad *pad) const {
        /* Validate arguments. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(pad != nullptr);

        /* Convert the pad to an internal number. */
        const InternalGpioPadNumber pad_number = pad->GetPadNumber();

        /* Get the pad direction by reading the appropriate bit in OE */
        const uintptr_t pad_address = GetGpioRegisterAddress(this->gpio_virtual_address, GpioRegisterType_GPIO_OE, pad_number);
        const uintptr_t pad_index   = ConvertInternalGpioPadNumberToBitIndex(pad_number);

        if (reg::Read(pad_address, 1u << pad_index) != 0) {
            *out = Direction_Output;
        } else {
            *out = Direction_Input;
        }

        return ResultSuccess();

    }

    Result DriverImpl::SetDirection(Pad *pad, Direction direction) {
        /* Validate arguments. */
        AMS_ASSERT(pad != nullptr);

        /* Convert the pad to an internal number. */
        const InternalGpioPadNumber pad_number = pad->GetPadNumber();

        /* Configure the pad direction by modifying the appropriate bit in OE */
        const uintptr_t pad_address = GetGpioRegisterAddress(this->gpio_virtual_address, GpioRegisterType_GPIO_OE, pad_number);
        const uintptr_t pad_index   = ConvertInternalGpioPadNumberToBitIndex(pad_number);
        SetMaskedBit(pad_address, pad_index, direction);

        /* Read the pad address to make sure our configuration takes. */
        reg::Read(pad_address);

        return ResultSuccess();
    }

    Result DriverImpl::GetValue(GpioValue *out, Pad *pad) const {
        /* Validate arguments. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(pad != nullptr);

        /* Convert the pad to an internal number. */
        const InternalGpioPadNumber pad_number = pad->GetPadNumber();

        /* Get the pad value by reading the appropriate bit in IN */
        const uintptr_t pad_address = GetGpioRegisterAddress(this->gpio_virtual_address, GpioRegisterType_GPIO_IN, pad_number);
        const uintptr_t pad_index   = ConvertInternalGpioPadNumberToBitIndex(pad_number);

        if (reg::Read(pad_address, 1u << pad_index) != 0) {
            *out = GpioValue_High;
        } else {
            *out = GpioValue_Low;
        }

        return ResultSuccess();
    }

    Result DriverImpl::SetValue(Pad *pad, GpioValue value) {
        /* Validate arguments. */
        AMS_ASSERT(pad != nullptr);

        /* Convert the pad to an internal number. */
        const InternalGpioPadNumber pad_number = pad->GetPadNumber();

        /* Configure the pad value by modifying the appropriate bit in IN */
        const uintptr_t pad_address = GetGpioRegisterAddress(this->gpio_virtual_address, GpioRegisterType_GPIO_IN, pad_number);
        const uintptr_t pad_index   = ConvertInternalGpioPadNumberToBitIndex(pad_number);
        SetMaskedBit(pad_address, pad_index, value);

        /* Read the pad address to make sure our configuration takes. */
        reg::Read(pad_address);

        return ResultSuccess();
    }

    Result DriverImpl::GetInterruptMode(InterruptMode *out, Pad *pad) const {
        /* Validate arguments. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(pad != nullptr);

        /* Convert the pad to an internal number. */
        const InternalGpioPadNumber pad_number = pad->GetPadNumber();

        /* Get the pad mode by reading the appropriate bits in INT_LVL */
        const uintptr_t pad_address = GetGpioRegisterAddress(this->gpio_virtual_address, GpioRegisterType_GPIO_INT_LVL, pad_number);
        const uintptr_t pad_index   = ConvertInternalGpioPadNumberToBitIndex(pad_number);

        switch ((reg::Read(pad_address) >> pad_index) & InternalInterruptMode_Mask) {
            case InternalInterruptMode_LowLevel:    *out = InterruptMode_LowLevel;    break;
            case InternalInterruptMode_HighLevel:   *out = InterruptMode_HighLevel;   break;
            case InternalInterruptMode_RisingEdge:  *out = InterruptMode_RisingEdge;  break;
            case InternalInterruptMode_FallingEdge: *out = InterruptMode_FallingEdge; break;
            case InternalInterruptMode_AnyEdge:     *out = InterruptMode_AnyEdge;     break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        return ResultSuccess();
    }

    Result DriverImpl::SetInterruptMode(Pad *pad, InterruptMode mode) {
        /* Validate arguments. */
        AMS_ASSERT(pad != nullptr);

        /* Convert the pad to an internal number. */
        const InternalGpioPadNumber pad_number = pad->GetPadNumber();

        /* Configure the pad mode by modifying the appropriate bits in INT_LVL */
        const uintptr_t pad_address = GetGpioRegisterAddress(this->gpio_virtual_address, GpioRegisterType_GPIO_INT_LVL, pad_number);
        const uintptr_t pad_index   = ConvertInternalGpioPadNumberToBitIndex(pad_number);

        switch (mode) {
            case InterruptMode_LowLevel:    reg::ReadWrite(pad_address, InternalInterruptMode_LowLevel    << pad_index, InternalInterruptMode_Mask << pad_index); break;
            case InterruptMode_HighLevel:   reg::ReadWrite(pad_address, InternalInterruptMode_HighLevel   << pad_index, InternalInterruptMode_Mask << pad_index); break;
            case InterruptMode_RisingEdge:  reg::ReadWrite(pad_address, InternalInterruptMode_RisingEdge  << pad_index, InternalInterruptMode_Mask << pad_index); break;
            case InterruptMode_FallingEdge: reg::ReadWrite(pad_address, InternalInterruptMode_FallingEdge << pad_index, InternalInterruptMode_Mask << pad_index); break;
            case InterruptMode_AnyEdge:     reg::ReadWrite(pad_address, InternalInterruptMode_AnyEdge     << pad_index, InternalInterruptMode_Mask << pad_index); break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Read the pad address to make sure our configuration takes. */
        reg::Read(pad_address);

        return ResultSuccess();
    }

    Result DriverImpl::SetInterruptEnabled(Pad *pad, bool en) {
        /* TODO */
        AMS_ABORT();
    }

    Result DriverImpl::GetInterruptStatus(InterruptStatus *out, Pad *pad) {
        /* TODO */
        AMS_ABORT();
    }

    Result DriverImpl::ClearInterruptStatus(Pad *pad) {
        /* TODO */
        AMS_ABORT();
    }

    Result DriverImpl::GetDebounceEnabled(bool *out, Pad *pad) const {
        /* TODO */
        AMS_ABORT();
    }

    Result DriverImpl::SetDebounceEnabled(Pad *pad, bool en) {
        /* TODO */
        AMS_ABORT();
    }

    Result DriverImpl::GetDebounceTime(s32 *out_ms, Pad *pad) const {
        /* TODO */
        AMS_ABORT();
    }

    Result DriverImpl::SetDebounceTime(Pad *pad, s32 ms) {
        /* TODO */
        AMS_ABORT();
    }

    Result DriverImpl::GetUnknown22(u32 *out) {
        /* TODO */
        AMS_ABORT();
    }

    void DriverImpl::Unknown23() {
        /* TODO */
        AMS_ABORT();
    }

    Result DriverImpl::SetValueForSleepState(Pad *pad, GpioValue value) {
        /* TODO */
        AMS_ABORT();
    }

    Result DriverImpl::IsWakeEventActive(bool *out, Pad *pad) const {
        /* TODO */
        AMS_ABORT();
    }

    Result DriverImpl::SetWakeEventActiveFlagSetForDebug(Pad *pad, bool en) {
        /* TODO */
        AMS_ABORT();
    }

    Result DriverImpl::SetWakePinDebugMode(WakePinDebugMode mode) {
        /* TODO */
        AMS_ABORT();
    }

    Result DriverImpl::Suspend() {
        /* TODO */
        AMS_ABORT();
    }

    Result DriverImpl::SuspendLow() {
        /* TODO */
        AMS_ABORT();
    }

    Result DriverImpl::Resume() {
        /* TODO */
        AMS_ABORT();
    }

    Result DriverImpl::ResumeLow() {
        /* TODO */
        AMS_ABORT();
    }

}
