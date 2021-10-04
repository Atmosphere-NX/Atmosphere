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

    class RemoteDsRootService {
        private:
            using Allocator     = sf::ExpHeapAllocator;
            using ObjectFactory = sf::ObjectFactory<Allocator::Policy>;
        private:
            Service m_srv;
            Allocator *m_allocator;
        public:
            RemoteDsRootService(Service &srv, sf::ExpHeapAllocator *allocator) : m_srv(srv), m_allocator(allocator) { /* ... */ }
            virtual ~RemoteDsRootService() { serviceClose(std::addressof(m_srv)); }
        public:
            Result GetService(sf::Out<sf::SharedPointer<usb::ds::IDsService>> out);
    };
    static_assert(ds::IsIDsRootService<RemoteDsRootService>);

}
