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

namespace ams::usb {

    class RemoteDsInterface {
        private:
            using Allocator     = sf::ExpHeapAllocator;
            using ObjectFactory = sf::ObjectFactory<Allocator::Policy>;
        private:
            Service m_srv;
            Allocator *m_allocator;
        public:
            RemoteDsInterface(Service &srv, sf::ExpHeapAllocator *allocator) : m_srv(srv), m_allocator(allocator) { /* ... */ }
            virtual ~RemoteDsInterface() { serviceClose(std::addressof(m_srv)); }
        public:
            Result RegisterEndpoint(u8 endpoint_address, sf::Out<sf::SharedPointer<usb::ds::IDsEndpoint>> out);
            Result GetSetupEvent(sf::OutCopyHandle out);
            Result GetSetupPacket(const sf::OutBuffer & out);
            Result CtrlInAsync(sf::Out<u32> out_urb_id, u64 address, u32 size);
            Result CtrlOutAsync(sf::Out<u32> out_urb_id, u64 address, u32 size);
            Result GetCtrlInCompletionEvent(sf::OutCopyHandle out);
            Result GetCtrlInUrbReport(sf::Out<usb::UrbReport> out);
            Result GetCtrlOutCompletionEvent(sf::OutCopyHandle out);
            Result GetCtrlOutUrbReport(sf::Out<usb::UrbReport> out);
            Result CtrlStall();
            Result AppendConfigurationData(u8 bInterfaceNumber, usb::UsbDeviceSpeed device_speed, const sf::InBuffer &data);
            Result Enable();
            Result Disable();
    };
    static_assert(ds::IsIDsInterface<RemoteDsInterface>);

}
