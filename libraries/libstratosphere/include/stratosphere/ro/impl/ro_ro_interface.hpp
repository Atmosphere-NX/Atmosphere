/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <stratosphere/ro/ro_types.hpp>
#include <stratosphere/sf.hpp>

namespace ams::ro::impl {

    #define AMS_RO_I_RO_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                                              \
        AMS_SF_METHOD_INFO(C, H,  0, Result, MapManualLoadModuleMemory,   (sf::Out<u64> out_load_address, const sf::ClientProcessId &client_pid, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size))                     \
        AMS_SF_METHOD_INFO(C, H,  1, Result, UnmapManualLoadModuleMemory, (const sf::ClientProcessId &client_pid, u64 nro_address))                                                                                                 \
        AMS_SF_METHOD_INFO(C, H,  2, Result, RegisterModuleInfo,          (const sf::ClientProcessId &client_pid, u64 nrr_address, u64 nrr_size))                                                                                   \
        AMS_SF_METHOD_INFO(C, H,  3, Result, UnregisterModuleInfo,        (const sf::ClientProcessId &client_pid, u64 nrr_address))                                                                                                 \
        AMS_SF_METHOD_INFO(C, H,  4, Result, RegisterProcessHandle,       (const sf::ClientProcessId &client_pid, sf::CopyHandle process_h))                                                                                        \
        AMS_SF_METHOD_INFO(C, H, 10, Result, RegisterProcessModuleInfo,   (const sf::ClientProcessId &client_pid, u64 nrr_address, u64 nrr_size, sf::CopyHandle process_h),                                     hos::Version_7_0_0)

    AMS_SF_DEFINE_INTERFACE(IRoInterface, AMS_RO_I_RO_INTERFACE_INTERFACE_INFO)

}
