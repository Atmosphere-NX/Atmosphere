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

    class RemoteDsService {
        private:
            using Allocator     = sf::ExpHeapAllocator;
            using ObjectFactory = sf::ObjectFactory<Allocator::Policy>;
        private:
            Service m_srv;
            Allocator *m_allocator;
        public:
            RemoteDsService(Service &srv, sf::ExpHeapAllocator *allocator) : m_srv(srv), m_allocator(allocator) { /* ... */ }
            virtual ~RemoteDsService() { serviceClose(std::addressof(m_srv)); }
        public:
            Result Bind(usb::ComplexId complex_id, sf::CopyHandle &&process_h);
            Result RegisterInterface(sf::Out<sf::SharedPointer<usb::ds::IDsInterface>> out, u8 bInterfaceNumber);
            Result GetStateChangeEvent(sf::OutCopyHandle out);
            Result GetState(sf::Out<usb::UsbState> out);
            Result ClearDeviceData();
            Result AddUsbStringDescriptor(sf::Out<u8> out, const sf::InBuffer &desc);
            Result DeleteUsbStringDescriptor(u8 index);
            Result SetUsbDeviceDescriptor(const sf::InBuffer &desc, usb::UsbDeviceSpeed speed);
            Result SetBinaryObjectStore(const sf::InBuffer &bos);
            Result Enable();
            Result Disable();
    };
    static_assert(ds::IsIDsService<RemoteDsService>);

}
