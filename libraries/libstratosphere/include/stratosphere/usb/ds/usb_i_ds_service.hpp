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
#include <stratosphere/usb/ds/usb_i_ds_interface.hpp>

#define AMS_USB_I_DS_SERVICE_INTERFACE_INFO(C, H)                                                                                                                                     \
    AMS_SF_METHOD_INFO(C, H,  0, Result, Bind,                      (usb::ComplexId complex_id, sf::CopyHandle &&process_h),                      (complex_id, std::move(process_h))) \
    AMS_SF_METHOD_INFO(C, H,  1, Result, RegisterInterface,         (sf::Out<sf::SharedPointer<usb::ds::IDsInterface>> out, u8 bInterfaceNumber), (out, bInterfaceNumber))            \
    AMS_SF_METHOD_INFO(C, H,  2, Result, GetStateChangeEvent,       (sf::OutCopyHandle out),                                                      (out))                              \
    AMS_SF_METHOD_INFO(C, H,  3, Result, GetState,                  (sf::Out<usb::UsbState> out),                                                 (out))                              \
    AMS_SF_METHOD_INFO(C, H,  4, Result, ClearDeviceData,           (),                                                                           ())                                 \
    AMS_SF_METHOD_INFO(C, H,  5, Result, AddUsbStringDescriptor,    (sf::Out<u8> out, const sf::InBuffer &desc),                                  (out, desc))                        \
    AMS_SF_METHOD_INFO(C, H,  6, Result, DeleteUsbStringDescriptor, (u8 index),                                                                   (index))                            \
    AMS_SF_METHOD_INFO(C, H,  7, Result, SetUsbDeviceDescriptor,    (const sf::InBuffer &desc, usb::UsbDeviceSpeed speed),                        (desc, speed))                      \
    AMS_SF_METHOD_INFO(C, H,  8, Result, SetBinaryObjectStore,      (const sf::InBuffer &bos),                                                    (bos))                              \
    AMS_SF_METHOD_INFO(C, H,  9, Result, Enable,                    (),                                                                           ())                                 \
    AMS_SF_METHOD_INFO(C, H, 10, Result, Disable,                   (),                                                                           ())

/* TODO: Deprecated interface? */

AMS_SF_DEFINE_INTERFACE(ams::usb::ds, IDsService, AMS_USB_I_DS_SERVICE_INTERFACE_INFO)

#define AMS_USB_I_DS_ROOT_SERVICE_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 0, Result, GetService, (sf::Out<sf::SharedPointer<usb::ds::IDsService>> out), (out))

AMS_SF_DEFINE_INTERFACE(ams::usb::ds, IDsRootService, AMS_USB_I_DS_ROOT_SERVICE_INTERFACE_INFO)

