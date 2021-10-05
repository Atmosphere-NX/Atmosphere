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
#include <stratosphere.hpp>
#include "usb_remote_ds_endpoint.hpp"

namespace ams::usb {

    Result RemoteDsEndpoint::PostBufferAsync(sf::Out<u32> out_urb_id, u64 address, u32 size) {
        const struct {
            u32 size;
            u64 address;
        } in = { size, address };

        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatchInOut(std::addressof(m_srv), 0, in, *out_urb_id);
    }

    Result RemoteDsEndpoint::Cancel() {
        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatch(std::addressof(m_srv), 1);
    }

    Result RemoteDsEndpoint::GetCompletionEvent(sf::OutCopyHandle out) {
        serviceAssumeDomain(std::addressof(m_srv));

        os::NativeHandle event_handle;
        R_TRY((serviceDispatch(std::addressof(m_srv), 2,
            .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
            .out_handles = std::addressof(event_handle),
        )));

        out.SetValue(event_handle, true);
        return ResultSuccess();
    }

    Result RemoteDsEndpoint::GetUrbReport(sf::Out<usb::UrbReport> out) {
        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatchOut(std::addressof(m_srv), 3, *out);
    }

    Result RemoteDsEndpoint::Stall() {
        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatch(std::addressof(m_srv), 4);
    }

    Result RemoteDsEndpoint::SetZlt(bool zlt) {
        const u8 in = zlt ? 1 : 0;

        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatchIn(std::addressof(m_srv), 5, in);
    }

}
