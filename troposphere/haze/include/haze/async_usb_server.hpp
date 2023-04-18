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

#include <haze/common.hpp>
#include <haze/event_reactor.hpp>

namespace haze {

    class AsyncUsbServer final {
        private:
            EventReactor *m_reactor;
        public:
            constexpr explicit AsyncUsbServer() : m_reactor() { /* ... */ }

            Result Initialize(const UsbCommsInterfaceInfo *interface_info, u16 id_vendor, u16 id_product, EventReactor *reactor);
            void Finalize();
        private:
            Result TransferPacketImpl(bool read, void *page, u32 size, u32 *out_size_transferred) const;
        public:
            Result ReadPacket(void *page, u32 size, u32 *out_size_transferred) const {
                R_RETURN(this->TransferPacketImpl(true, page, size, out_size_transferred));
            }

            Result WritePacket(void *page, u32 size) const {
                u32 size_transferred;
                R_RETURN(this->TransferPacketImpl(false, page, size, std::addressof(size_transferred)));
            }
    };

}
