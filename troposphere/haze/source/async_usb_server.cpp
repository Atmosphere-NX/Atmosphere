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
#include <haze.hpp>

namespace haze {

    namespace {

        constinit UsbSession g_usb_session;

    }

    Result AsyncUsbServer::Initialize(const UsbCommsInterfaceInfo *interface_info, u16 id_vendor, u16 id_product, EventReactor *reactor) {
        m_reactor = reactor;

        /* Set up a new USB session. */
        R_TRY(g_usb_session.Initialize(interface_info, id_vendor, id_product));

        R_SUCCEED();
    }

    void AsyncUsbServer::Finalize() {
        g_usb_session.Finalize();
    }

    Result AsyncUsbServer::TransferPacketImpl(bool read, void *page, u32 size, u32 *out_size_transferred) const {
        u32 urb_id;
        s32 waiter_idx;

        /* If we're not configured yet, wait to become configured first. */
        if (!g_usb_session.GetConfigured()) {
            R_TRY(m_reactor->WaitFor(std::addressof(waiter_idx), waiterForEvent(usbDsGetStateChangeEvent())));
            R_TRY(eventClear(usbDsGetStateChangeEvent()));

            R_THROW(haze::ResultNotConfigured());
        }

        /* Select the appropriate endpoint and begin a transfer. */
        UsbSessionEndpoint ep = read ? UsbSessionEndpoint_Read : UsbSessionEndpoint_Write;
        R_TRY(g_usb_session.TransferAsync(ep, page, size, std::addressof(urb_id)));

        /* Try to wait for the event. */
        R_TRY(m_reactor->WaitFor(std::addressof(waiter_idx), waiterForEvent(g_usb_session.GetCompletionEvent(ep))));

        /* Return what we transferred. */
        R_RETURN(g_usb_session.GetTransferResult(ep, urb_id, out_size_transferred));
    }

}
