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

/* NOTE: Minimum firmware version not enforced for any commands. */
#define AMS_TMA_I_HTC_MANAGER_INTERFACE_INFO(C, H)                                                                                                                               \
    AMS_SF_METHOD_INFO(C, H,  0, Result, GetEnvironmentVariable,             (sf::Out<s32> out_size, const sf::OutBuffer &out, const sf::InBuffer &name), (out_size, out, name)) \
    AMS_SF_METHOD_INFO(C, H,  1, Result, GetEnvironmentVariableLength,       (sf::Out<s32> out_size, const sf::InBuffer &name),                           (out_size, name))      \
    AMS_SF_METHOD_INFO(C, H,  2, Result, GetHostConnectionEvent,             (sf::OutCopyHandle out),                                                     (out))                 \
    AMS_SF_METHOD_INFO(C, H,  3, Result, GetHostDisconnectionEvent,          (sf::OutCopyHandle out),                                                     (out))                 \
    AMS_SF_METHOD_INFO(C, H,  4, Result, GetHostConnectionEventForSystem,    (sf::OutCopyHandle out),                                                     (out))                 \
    AMS_SF_METHOD_INFO(C, H,  5, Result, GetHostDisconnectionEventForSystem, (sf::OutCopyHandle out),                                                     (out))                 \
    AMS_SF_METHOD_INFO(C, H,  6, Result, GetBridgeIpAddress,                 (const sf::OutBuffer &out),                                                  (out))                 \
    AMS_SF_METHOD_INFO(C, H,  7, Result, GetBridgePort,                      (const sf::OutBuffer &out),                                                  (out))                 \
    AMS_SF_METHOD_INFO(C, H,  8, Result, SetCradleAttached,                  (bool attached),                                                             (attached))            \
    AMS_SF_METHOD_INFO(C, H,  9, Result, GetBridgeSubnetMask,                (const sf::OutBuffer &out),                                                  (out))                 \
    AMS_SF_METHOD_INFO(C, H, 10, Result, GetBridgeMacAddress,                (const sf::OutBuffer &out),                                                  (out))                 \
    AMS_SF_METHOD_INFO(C, H, 11, Result, GetWorkingDirectoryPath,            (const sf::OutBuffer &out, s32 max_len),                                     (out, max_len))        \
    AMS_SF_METHOD_INFO(C, H, 12, Result, GetWorkingDirectoryPathSize,        (sf::Out<s32> out_size),                                                     (out_size))            \
    AMS_SF_METHOD_INFO(C, H, 13, Result, RunOnHostStart,                     (sf::Out<u32> out_id, sf::OutCopyHandle out, const sf::InBuffer &args),      (out_id, out, args))   \
    AMS_SF_METHOD_INFO(C, H, 14, Result, RunOnHostResults,                   (sf::Out<s32> out_result, u32 id),                                           (out_result, id))      \
    AMS_SF_METHOD_INFO(C, H, 15, Result, SetBridgeIpAddress,                 (const sf::InBuffer &arg),                                                   (arg))                 \
    AMS_SF_METHOD_INFO(C, H, 16, Result, SetBridgeSubnetMask,                (const sf::InBuffer &arg),                                                   (arg))                 \
    AMS_SF_METHOD_INFO(C, H, 17, Result, SetBridgePort,                      (const sf::InBuffer &arg),                                                   (arg))

AMS_SF_DEFINE_INTERFACE(ams::tma, IHtcManager, AMS_TMA_I_HTC_MANAGER_INTERFACE_INFO)
