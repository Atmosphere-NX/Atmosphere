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
#include "usb_remote_ds_service.hpp"
#include "usb_remote_ds_interface.hpp"

namespace ams::usb {

    Result RemoteDsService::Bind(usb::ComplexId complex_id, sf::CopyHandle &&process_h) {
        if (hos::GetVersion() >= hos::Version_11_0_0) {
            serviceAssumeDomain(std::addressof(m_srv));
            R_TRY(serviceDispatchIn(std::addressof(m_srv), 0, complex_id,
                .in_num_handles = 1,
                .in_handles = { process_h.GetOsHandle() }
            ));
        } else {
            serviceAssumeDomain(std::addressof(m_srv));
            R_TRY(serviceDispatchIn(std::addressof(m_srv), 0, complex_id));

            serviceAssumeDomain(std::addressof(m_srv));
            R_TRY(serviceDispatch(std::addressof(m_srv), 1,
                .in_num_handles = 1,
                .in_handles = { process_h.GetOsHandle() })
            );
        }

        return ResultSuccess();
    }

    Result RemoteDsService::RegisterInterface(sf::Out<sf::SharedPointer<usb::ds::IDsInterface>> out, u8 bInterfaceNumber) {
        Service srv;

        serviceAssumeDomain(std::addressof(m_srv));
        R_TRY(serviceDispatchIn(std::addressof(m_srv), (hos::GetVersion() >= hos::Version_11_0_0 ? 1 : 2), bInterfaceNumber,
            .out_num_objects = 1,
            .out_objects     = std::addressof(srv),
        ));

        *out = ObjectFactory::CreateSharedEmplaced<ds::IDsInterface, RemoteDsInterface>(m_allocator, srv, m_allocator);

        return ResultSuccess();
    }

    Result RemoteDsService::GetStateChangeEvent(sf::OutCopyHandle out) {
        serviceAssumeDomain(std::addressof(m_srv));

        os::NativeHandle event_handle;
        R_TRY((serviceDispatch(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 2 : 3,
            .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
            .out_handles = std::addressof(event_handle),
        )));

        out.SetValue(event_handle, true);
        return ResultSuccess();
    }

    Result RemoteDsService::GetState(sf::Out<usb::UsbState> out) {
        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatchOut(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 3 : 4, *out);
    }

    Result RemoteDsService::ClearDeviceData() {
        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatch(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 4 : 5);
    }

    Result RemoteDsService::AddUsbStringDescriptor(sf::Out<u8> out, const sf::InBuffer &desc) {
        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatchOut(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 5 : 6, *out,
            .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
            .buffers = { { desc.GetPointer(), desc.GetSize() } },
        );
    }

    Result RemoteDsService::DeleteUsbStringDescriptor(u8 index) {
        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatchIn(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 6 : 7, index);
    }

    Result RemoteDsService::SetUsbDeviceDescriptor(const sf::InBuffer &desc, usb::UsbDeviceSpeed speed) {
        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatchIn(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 7 : 8, speed,
            .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
            .buffers = { { desc.GetPointer(), desc.GetSize() } },
        );
    }

    Result RemoteDsService::SetBinaryObjectStore(const sf::InBuffer &bos) {
        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatch(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 8 : 9,
            .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_In },
            .buffers = { { bos.GetPointer(), bos.GetSize() } },
        );
    }

    Result RemoteDsService::Enable() {
        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatch(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 9 : 10);
    }

    Result RemoteDsService::Disable() {
        serviceAssumeDomain(std::addressof(m_srv));
        return serviceDispatch(std::addressof(m_srv), hos::GetVersion() >= hos::Version_11_0_0 ? 10 : 11);
    }

}
