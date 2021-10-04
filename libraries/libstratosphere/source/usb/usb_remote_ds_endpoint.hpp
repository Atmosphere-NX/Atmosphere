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

    class RemoteDsEndpoint {
        private:
            Service m_srv;
        public:
            RemoteDsEndpoint(Service &srv) : m_srv(srv) { /* ... */ }
            virtual ~RemoteDsEndpoint() { serviceClose(std::addressof(m_srv)); }
        public:
            Result PostBufferAsync(sf::Out<u32> out_urb_id, u64 address, u32 size);
            Result Cancel();
            Result GetCompletionEvent(sf::OutCopyHandle out);
            Result GetUrbReport(sf::Out<usb::UrbReport> out);
            Result Stall();
            Result SetZlt(bool zlt);
    };
    static_assert(ds::IsIDsEndpoint<RemoteDsEndpoint>);

}
