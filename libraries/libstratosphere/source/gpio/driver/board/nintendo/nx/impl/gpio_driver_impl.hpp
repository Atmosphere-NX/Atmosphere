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

#include "gpio_tegra_pad.hpp"
#include "gpio_register_accessor.hpp"
#include "gpio_suspend_handler.hpp"

namespace ams::gpio::driver::board::nintendo::nx::impl {

    class DriverImpl;

    class InterruptEventHandler : public ddsf::IEventHandler {
        private:
            DriverImpl *driver;
            os::InterruptName interrupt_name;
            os::InterruptEventType interrupt_event;
            int controller_number;
        private:
            bool CheckAndHandleInterrupt(TegraPad &pad);
        public:
            InterruptEventHandler() : IEventHandler(), driver(nullptr), interrupt_name(), interrupt_event(), controller_number() { /* ... */ }

            void Initialize(DriverImpl *drv, os::InterruptName intr, int ctlr);

            virtual void HandleEvent() override;
    };

    class DriverImpl : public ::ams::gpio::driver::IGpioDriver {
        NON_COPYABLE(DriverImpl);
        NON_MOVEABLE(DriverImpl);
        AMS_DDSF_CASTABLE_TRAITS(ams::gpio::driver::board::nintendo::nx::impl::DriverImpl, ::ams::gpio::driver::IGpioDriver);
        friend class InterruptEventHandler;
        private:
            dd::PhysicalAddress gpio_physical_address;
            uintptr_t gpio_virtual_address;
            SuspendHandler suspend_handler;
            TegraPad::InterruptList interrupt_pad_list;
            mutable os::SdkMutex interrupt_control_mutex;
        public:
            DriverImpl(dd::PhysicalAddress reg_paddr, size_t size);

            virtual void InitializeDriver() override;
            virtual void FinalizeDriver()   override;

            virtual Result InitializePad(Pad *pad) override;
            virtual void FinalizePad(Pad *pad)     override;

            virtual Result GetDirection(Direction *out, Pad *pad) const override;
            virtual Result SetDirection(Pad *pad, Direction direction)  override;

            virtual Result GetValue(GpioValue *out, Pad *pad) const override;
            virtual Result SetValue(Pad *pad, GpioValue value)      override;

            virtual Result GetInterruptMode(InterruptMode *out, Pad *pad) const override;
            virtual Result SetInterruptMode(Pad *pad, InterruptMode mode)       override;

            virtual Result SetInterruptEnabled(Pad *pad, bool en) override;

            virtual Result GetInterruptStatus(InterruptStatus *out, Pad *pad) override;
            virtual Result ClearInterruptStatus(Pad *pad)                     override;

            virtual os::SdkMutex &GetInterruptControlMutex(const Pad &pad) const override {
                AMS_UNUSED(pad);
                return this->interrupt_control_mutex;
            }

            virtual Result GetDebounceEnabled(bool *out, Pad *pad) const override;
            virtual Result SetDebounceEnabled(Pad *pad, bool en)         override;

            virtual Result GetDebounceTime(s32 *out_ms, Pad *pad) const override;
            virtual Result SetDebounceTime(Pad *pad, s32 ms)            override;

            virtual Result GetUnknown22(u32 *out) override;
            virtual void Unknown23() override;

            virtual Result SetValueForSleepState(Pad *pad, GpioValue value) override;
            virtual Result IsWakeEventActive(bool *out, Pad *pad) const override;
            virtual Result SetWakeEventActiveFlagSetForDebug(Pad *pad, bool en) override;
            virtual Result SetWakePinDebugMode(WakePinDebugMode mode) override;

            virtual Result Suspend()    override;
            virtual Result SuspendLow() override;
            virtual Result Resume()     override;
            virtual Result ResumeLow()  override;
        private:
            static constexpr ALWAYS_INLINE TegraPad &GetTegraPad(Pad *pad) {
                AMS_ASSERT(pad != nullptr);
                return static_cast<TegraPad &>(*pad);
            }

            static ALWAYS_INLINE const PadInfo &GetInfo(Pad *pad) {
                return GetTegraPad(pad).GetInfo();
            }

            static ALWAYS_INLINE PadStatus &GetStatus(Pad *pad) {
                return GetTegraPad(pad).GetStatus();
            }

            void AddInterruptPad(TegraPad *pad) {
                AMS_ASSERT(pad != nullptr);
                if (!pad->IsLinkedToInterruptBoundPadList()) {
                    this->interrupt_pad_list.push_back(*pad);
                }
            }

            void RemoveInterruptPad(TegraPad *pad) {
                AMS_ASSERT(pad != nullptr);
                if (pad->IsLinkedToInterruptBoundPadList()) {
                    this->interrupt_pad_list.erase(this->interrupt_pad_list.iterator_to(*pad));
                }
            }
    };

}
