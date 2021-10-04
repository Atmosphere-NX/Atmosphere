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
#include <vapours.hpp>
#include <stratosphere/usb/usb_limits.hpp>
#include <stratosphere/usb/usb_types.hpp>
#include <stratosphere/usb/usb_device_types.hpp>
#include <stratosphere/usb/ds/usb_i_ds_endpoint.hpp>

#define AMS_USB_I_DS_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                               \
    AMS_SF_METHOD_INFO(C, H,     0, Result, RegisterEndpoint,          (u8 endpoint_address, sf::Out<sf::SharedPointer<usb::ds::IDsEndpoint>> out),       (endpoint_address, out))                \
    AMS_SF_METHOD_INFO(C, H,     1, Result, GetSetupEvent,             (sf::OutCopyHandle out),                                                           (out))                                  \
    AMS_SF_METHOD_INFO(C, H,     2, Result, GetSetupPacket,            (const sf::OutBuffer &out),                                                        (out))                                  \
    AMS_SF_METHOD_INFO(C, H,     3, Result, CtrlInAsync,               (sf::Out<u32> out_urb_id, u64 address, u32 size),                                  (out_urb_id, address, size))            \
    AMS_SF_METHOD_INFO(C, H,     4, Result, CtrlOutAsync,              (sf::Out<u32> out_urb_id, u64 address, u32 size),                                  (out_urb_id, address, size))            \
    AMS_SF_METHOD_INFO(C, H,     5, Result, GetCtrlInCompletionEvent,  (sf::OutCopyHandle out),                                                           (out))                                  \
    AMS_SF_METHOD_INFO(C, H,     6, Result, GetCtrlInUrbReport,        (sf::Out<usb::UrbReport> out),                                                     (out))                                  \
    AMS_SF_METHOD_INFO(C, H,     7, Result, GetCtrlOutCompletionEvent, (sf::OutCopyHandle out),                                                           (out))                                  \
    AMS_SF_METHOD_INFO(C, H,     8, Result, GetCtrlOutUrbReport,       (sf::Out<usb::UrbReport> out),                                                     (out))                                  \
    AMS_SF_METHOD_INFO(C, H,     9, Result, CtrlStall,                 (),                                                                                ())                                     \
    AMS_SF_METHOD_INFO(C, H,    10, Result, AppendConfigurationData,   (u8 bInterfaceNumber, usb::UsbDeviceSpeed device_speed, const sf::InBuffer &data), (bInterfaceNumber, device_speed, data)) \
    AMS_SF_METHOD_INFO(C, H, 65000, Result, Enable,                    (),                                                                                ())                                     \
    AMS_SF_METHOD_INFO(C, H, 65001, Result, Disable,                   (),                                                                                ())

/* TODO: Deprecated interface? */

AMS_SF_DEFINE_INTERFACE(ams::usb::ds, IDsInterface, AMS_USB_I_DS_INTERFACE_INTERFACE_INFO)

