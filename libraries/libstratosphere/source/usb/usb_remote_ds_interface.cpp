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
#include "usb_remote_ds_interface.hpp"
#include "usb_remote_ds_endpoint.hpp"

namespace ams::usb {

    Result RemoteDsInterface::RegisterEndpoint(u8 endpoint_address, sf::Out<sf::SharedPointer<usb::ds::IDsEndpoint>> out) {
        Service srv;

        serviceAssumeDomain(std::addressof(m_srv));
        R_TRY(serviceDispatchIn(std::addressof(m_srv), 0, endpoint_address,
            .out_num_objects = 1,
            .out_objects     = std::addressof(srv),
        ));

        *out = ObjectFactory::CreateSharedEmplaced<ds::IDsEndpoint, RemoteDsEndpoint>(m_allocator, srv);

        return ResultSuccess();
    }

    Result RemoteDsInterface::GetSetupEvent(sf::OutCopyHandle out) {
        serviceAssumeDomain(std::addressof(m_srv));

        os::NativeHandle event_handle;
        R_TRY((serviceDispatch(std::addressof(m_srv), 1,
            .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
            .out_handles = std::addressof(event_handle),
        )));

        out.SetValue(event_handle, true);
        return ResultSuccess();
    }

    Result RemoteDsInterface::GetSetupPacket(const sf::OutBuffer &out) {
        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatch(std::addressof(m_srv), 2,
            .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
            .buffers = { { out.GetPointer(), out.GetSize() } },
        );
    }

    Result RemoteDsInterface::CtrlInAsync(sf::Out<u32> out_urb_id, u64 address, u32 size) {
        const struct {
            u32 size;
            u64 address;
        } in = { size, address };

        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatchInOut(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 3 : 5, in, *out_urb_id);
    }

    Result RemoteDsInterface::CtrlOutAsync(sf::Out<u32> out_urb_id, u64 address, u32 size) {
        const struct {
            u32 size;
            u64 address;
        } in = { size, address };

        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatchInOut(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 4 : 6, in, *out_urb_id);
    }

    Result RemoteDsInterface::GetCtrlInCompletionEvent(sf::OutCopyHandle out) {
        serviceAssumeDomain(std::addressof(m_srv));

        os::NativeHandle event_handle;
        R_TRY((serviceDispatch(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 5 : 7,
            .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
            .out_handles = std::addressof(event_handle),
        )));

        out.SetValue(event_handle, true);
        return ResultSuccess();
    }

    Result RemoteDsInterface::GetCtrlInUrbReport(sf::Out<usb::UrbReport> out) {
        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatchOut(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 6 : 8, *out);
    }

    Result RemoteDsInterface::GetCtrlOutCompletionEvent(sf::OutCopyHandle out) {
        serviceAssumeDomain(std::addressof(m_srv));

        os::NativeHandle event_handle;
        R_TRY((serviceDispatch(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 7 : 9,
            .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
            .out_handles = std::addressof(event_handle),
        )));

        out.SetValue(event_handle, true);
        return ResultSuccess();
    }

    Result RemoteDsInterface::GetCtrlOutUrbReport(sf::Out<usb::UrbReport> out) {
        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatchOut(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 8 : 10, *out);
    }

    Result RemoteDsInterface::CtrlStall() {
        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatch(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 9 : 11);
    }

    Result RemoteDsInterface::AppendConfigurationData(u8 bInterfaceNumber, usb::UsbDeviceSpeed device_speed, const sf::InBuffer &data) {
        if (hos::GetVersion() >= hos::Version_11_0_0) {
            serviceAssumeDomain(std::addressof(m_srv));
            return serviceDispatchIn(std::addressof(m_srv), 10, device_speed,
                .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
                .buffers = { { data.GetPointer(), data.GetSize() } },
            );
        } else {
            const struct {
                u8 bInterfaceNumber;
                usb::UsbDeviceSpeed device_speed;
            } in = { bInterfaceNumber, device_speed };

            serviceAssumeDomain(std::addressof(m_srv));
            return serviceDispatchIn(std::addressof(m_srv), 12, in,
                .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
                .buffers = { { data.GetPointer(), data.GetSize() } },
            );
        }
    }

    Result RemoteDsInterface::Enable() {
        R_SUCCEED_IF(hos::GetVersion() >= hos::Version_11_0_0);

        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatch(std::addressof(m_srv), 3);
    }

    Result RemoteDsInterface::Disable() {
        R_SUCCEED_IF(hos::GetVersion() >= hos::Version_11_0_0);

        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatch(std::addressof(m_srv), 3);
    }


}
