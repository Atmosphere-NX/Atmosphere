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

namespace haze {

    enum UsbSessionEndpoint {
        UsbSessionEndpoint_Read      = 0,
        UsbSessionEndpoint_Write     = 1,
        UsbSessionEndpoint_Interrupt = 2,
        UsbSessionEndpoint_Count     = 3,
    };

    class UsbSession {
        private:
            UsbDsInterface *m_interface;
            UsbDsEndpoint *m_endpoints[UsbSessionEndpoint_Count];
        private:
            Result Initialize1x(const UsbCommsInterfaceInfo *info);
            Result Initialize5x(const UsbCommsInterfaceInfo *info);
        public:
            constexpr explicit UsbSession() : m_interface(), m_endpoints() { /* ... */ }

            Result Initialize(const UsbCommsInterfaceInfo *info, u16 id_vendor, u16 id_product);
            void Finalize();

            bool GetConfigured() const;
            Event *GetCompletionEvent(UsbSessionEndpoint ep) const;
            Result TransferAsync(UsbSessionEndpoint ep, void *buffer, u32 size, u32 *out_urb_id);
            Result GetTransferResult(UsbSessionEndpoint ep, u32 urb_id, u32 *out_transferred_size);
    };

}
