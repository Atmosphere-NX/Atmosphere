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
#include <stratosphere/sf.hpp>
#include <stratosphere/fs/fs_error_info.hpp>
#include <stratosphere/fs/fs_game_card.hpp>

/* TODO */
/* ACCURATE_TO_VERSION: 13.4.0.0 */
#define AMS_FSSRV_I_DEVICE_OPERATOR_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H,   0, Result, IsSdCardInserted,           (ams::sf::Out<bool> out),                                                                                       (out))                              \
    AMS_SF_METHOD_INFO(C, H,   1, Result, GetSdCardSpeedMode,         (ams::sf::Out<s64> out),                                                                                        (out))                              \
    AMS_SF_METHOD_INFO(C, H,   2, Result, GetSdCardCid,               (ams::sf::OutBuffer out, s64 size),                                                                             (out, size))                        \
    AMS_SF_METHOD_INFO(C, H,   3, Result, GetSdCardUserAreaSize,      (ams::sf::Out<s64> out),                                                                                        (out))                              \
    AMS_SF_METHOD_INFO(C, H,   4, Result, GetSdCardProtectedAreaSize, (ams::sf::Out<s64> out),                                                                                        (out))                              \
    AMS_SF_METHOD_INFO(C, H,   5, Result, GetAndClearSdCardErrorInfo, (ams::sf::Out<fs::StorageErrorInfo> out_sei, ams::sf::Out<s64> out_size, ams::sf::OutBuffer out_buf, s64 size), (out_sei, out_size, out_buf, size)) \
    AMS_SF_METHOD_INFO(C, H, 100, Result, GetMmcCid,                  (ams::sf::OutBuffer out, s64 size),                                                                             (out, size))                        \
    AMS_SF_METHOD_INFO(C, H, 101, Result, GetMmcSpeedMode,            (ams::sf::Out<s64> out),                                                                                        (out))                              \
    AMS_SF_METHOD_INFO(C, H, 112, Result, GetMmcPatrolCount,          (ams::sf::Out<u32> out),                                                                                        (out))                              \
    AMS_SF_METHOD_INFO(C, H, 113, Result, GetAndClearMmcErrorInfo,    (ams::sf::Out<fs::StorageErrorInfo> out_sei, ams::sf::Out<s64> out_size, ams::sf::OutBuffer out_buf, s64 size), (out_sei, out_size, out_buf, size)) \
    AMS_SF_METHOD_INFO(C, H, 114, Result, GetMmcExtendedCsd,          (ams::sf::OutBuffer out, s64 size),                                                                             (out, size))                        \
    AMS_SF_METHOD_INFO(C, H, 200, Result, IsGameCardInserted,         (ams::sf::Out<bool> out),                                                                                       (out))                              \
    AMS_SF_METHOD_INFO(C, H, 202, Result, GetGameCardHandle,          (ams::sf::Out<u32> out),                                                                                        (out))                              \
    AMS_SF_METHOD_INFO(C, H, 208, Result, GetGameCardIdSet,           (ams::sf::OutBuffer out, s64 size),                                                                             (out, size))                        \
    AMS_SF_METHOD_INFO(C, H, 217, Result, GetGameCardErrorReportInfo, (ams::sf::Out<fs::GameCardErrorReportInfo> out),                                                                (out))                              \
    AMS_SF_METHOD_INFO(C, H, 218, Result, GetGameCardDeviceId,        (ams::sf::OutBuffer out, s64 size),                                                                             (out, size))

AMS_SF_DEFINE_INTERFACE(ams::fssrv::sf, IDeviceOperator, AMS_FSSRV_I_DEVICE_OPERATOR_INTERFACE_INFO, 0x1484E21C)
