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
#include "htclow_i_driver.hpp"
#include "htclow_usb_impl.hpp"

namespace ams::htclow::driver {

    class UsbDriver final : public IDriver {
        private:
            bool m_connected;
            os::Event m_event;
        public:
            UsbDriver() : m_connected(false), m_event(os::EventClearMode_ManualClear) { /* ... */ }
        public:
            static void OnUsbAvailabilityChange(UsbAvailability availability, void *param);
        public:
            virtual Result Open() override;
            virtual void Close() override;
            virtual Result Connect(os::EventType *event) override;
            virtual void Shutdown() override;
            virtual Result Send(const void *src, int src_size) override;
            virtual Result Receive(void *dst, int dst_size) override;
            virtual void CancelSendReceive() override;
            virtual void Suspend() override;
            virtual void Resume() override;
    };

}
