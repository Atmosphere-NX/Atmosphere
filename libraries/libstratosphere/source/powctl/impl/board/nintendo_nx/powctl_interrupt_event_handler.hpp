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
#include "../../powctl_i_power_control_driver.hpp"

namespace ams::powctl::impl::board::nintendo_nx {

    template<typename Derived>
    class InterruptEventHandler : public ddsf::IEventHandler {
        private:
            IDevice *device;
            gpio::GpioPadSession gpio_session;
            os::SystemEventType gpio_system_event;
            os::SdkMutex mutex;
        public:
            InterruptEventHandler(IDevice *dv) : IEventHandler(), device(dv), mutex() {
                /* Initialize the gpio session. */
                Derived::Initialize(std::addressof(this->gpio_session), std::addressof(this->gpio_system_event));

                /* Initialize ourselves as an event handler. */
                IEventHandler::Initialize(std::addressof(this->gpio_system_event));
            }

            os::SystemEventType *GetSystemEvent() {
                return std::addressof(this->gpio_system_event);
            }

            void SetInterruptEnabled(bool en) {
                std::scoped_lock lk(this->mutex);

                gpio::SetInterruptEnable(std::addressof(this->gpio_session), en);
            }

            virtual void HandleEvent() override final {
                /* Acquire exclusive access to ourselves. */
                std::scoped_lock lk(this->mutex);

                /* Clear our interrupt status. */
                gpio::ClearInterruptStatus(std::addressof(this->gpio_session));

                /* Clear our system event. */
                os::ClearSystemEvent(std::addressof(this->gpio_system_event));

                /* Signal the event. */
                static_cast<Derived *>(this)->SignalEvent(this->device);
            }
    };

    class ChargerInterruptEventHandler : public InterruptEventHandler<ChargerInterruptEventHandler> {
        friend class InterruptEventHandler<ChargerInterruptEventHandler>;
        private:
            static void Initialize(gpio::GpioPadSession *session, os::SystemEventType *event) {
                /* Open the gpio session. */
                R_ABORT_UNLESS(gpio::OpenSession(session, gpio::DeviceCode_Bq24190Irq));

                /* Configure the gpio session. */
                gpio::SetDirection(session, gpio::Direction_Input);
                gpio::SetInterruptMode(session, gpio::InterruptMode_FallingEdge);
                gpio::SetInterruptEnable(session, true);

                /* Bind the interrupt event. */
                R_ABORT_UNLESS(gpio::BindInterrupt(event, session));
            }

            void SignalEvent(IDevice *device);
        public:
            ChargerInterruptEventHandler(IDevice *dv) : InterruptEventHandler<ChargerInterruptEventHandler>(dv) { /* ... */ }
    };

    class BatteryInterruptEventHandler : public InterruptEventHandler<BatteryInterruptEventHandler> {
        friend class InterruptEventHandler<BatteryInterruptEventHandler>;
        private:
            static void Initialize(gpio::GpioPadSession *session, os::SystemEventType *event) {
                /* Open the gpio session. */
                R_ABORT_UNLESS(gpio::OpenSession(session, gpio::DeviceCode_BattMgicIrq));

                /* Configure the gpio session. */
                gpio::SetDirection(session, gpio::Direction_Input);
                gpio::SetInterruptMode(session, gpio::InterruptMode_LowLevel);

                /* Bind the interrupt event. */
                R_ABORT_UNLESS(gpio::BindInterrupt(event, session));
            }

            void SignalEvent(IDevice *device);
        public:
            BatteryInterruptEventHandler(IDevice *dv) : InterruptEventHandler<BatteryInterruptEventHandler>(dv) { /* ... */ }
    };

}