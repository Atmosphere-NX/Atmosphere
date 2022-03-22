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

#define AMS_USB_I_DS_ENDPOINT_INTERFACE_INFO(C, H)                                                                                             \
    AMS_SF_METHOD_INFO(C, H,     0, Result, PostBufferAsync,    (sf::Out<u32> out_urb_id, u64 address, u32 size), (out_urb_id, address, size)) \
    AMS_SF_METHOD_INFO(C, H,     1, Result, Cancel,             (),                                               ())                          \
    AMS_SF_METHOD_INFO(C, H,     2, Result, GetCompletionEvent, (sf::OutCopyHandle out),                          (out))                       \
    AMS_SF_METHOD_INFO(C, H,     3, Result, GetUrbReport,       (sf::Out<usb::UrbReport> out),                    (out))                       \
    AMS_SF_METHOD_INFO(C, H,     4, Result, Stall,              (),                                               ())                          \
    AMS_SF_METHOD_INFO(C, H,     5, Result, SetZlt,             (bool zlt),                                       (zlt))

/* TODO: Deprecated interface? */

AMS_SF_DEFINE_INTERFACE(ams::usb::ds, IDsEndpoint, AMS_USB_I_DS_ENDPOINT_INTERFACE_INFO)

