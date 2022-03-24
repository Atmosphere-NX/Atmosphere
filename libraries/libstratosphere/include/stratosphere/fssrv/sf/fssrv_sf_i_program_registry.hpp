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
#include <stratosphere/fs/fs_code.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_ifilesystem.hpp>

/* ACCURATE_TO_VERSION: 13.4.0.0 */
#define AMS_FSSRV_I_PROGRAM_REGISTRY_INTERFACE_INFO(C, H)                                                                                                                                                                                                                                     \
    AMS_SF_METHOD_INFO(C, H,   0, Result, RegisterProgram,               (u64 process_id, u64 program_id, u8 storage_id, const ams::sf::InBuffer &data, s64 data_size, const ams::sf::InBuffer &desc, s64 desc_size), (process_id, program_id, storage_id, data, data_size, desc, desc_size)) \
    AMS_SF_METHOD_INFO(C, H,   1, Result, UnregisterProgram,             (u64 process_id),                                                                                                                            (process_id))                                                           \
    AMS_SF_METHOD_INFO(C, H,   2, Result, SetCurrentProcess,             (const ams::sf::ClientProcessId &client_pid),                                                                                                (client_pid))                                                           \
    AMS_SF_METHOD_INFO(C, H, 256, Result, SetEnabledProgramVerification, (bool en),                                                                                                                                   (en))

AMS_SF_DEFINE_INTERFACE(ams::fssrv::sf, IProgramRegistry, AMS_FSSRV_I_PROGRAM_REGISTRY_INTERFACE_INFO, 0xDA73738C)

