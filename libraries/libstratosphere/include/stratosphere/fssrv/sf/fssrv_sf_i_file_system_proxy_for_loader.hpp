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
#include <stratosphere/fssrv/sf/fssrv_sf_ifilesystem.hpp>
#include <stratosphere/fs/fs_code_verification_data.hpp>
#include <stratosphere/fs/fs_content_attributes.hpp>

/* ACCURATE_TO_VERSION: 17.5.0.0 */
#define AMS_FSSRV_I_FILE_SYSTEM_PROXY_FOR_LOADER_INTERFACE_INFO(C, H)                                                                                                                                                                                                                                                                                                            \
    AMS_SF_METHOD_INFO(C, H, 0, Result, OpenCodeFileSystemDeprecated,  (ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const fssrv::sf::Path &path, ncm::ProgramId program_id),                                                                               (out_fs, path, program_id),                        hos::Version_Min,    hos::Version_9_2_0)  \
    AMS_SF_METHOD_INFO(C, H, 0, Result, OpenCodeFileSystemDeprecated2, (ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, ams::sf::Out<fs::CodeVerificationData> out_verif, const fssrv::sf::Path &path, ncm::ProgramId program_id),                             (out_fs, out_verif, path, program_id),             hos::Version_10_0_0, hos::Version_15_0_1) \
    AMS_SF_METHOD_INFO(C, H, 0, Result, OpenCodeFileSystemDeprecated3, (ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, ams::sf::Out<fs::CodeVerificationData> out_verif, const fssrv::sf::Path &path, fs::ContentAttributes attr, ncm::ProgramId program_id), (out_fs, out_verif, path, attr, program_id),       hos::Version_16_0_0, hos::Version_16_1_0) \
    AMS_SF_METHOD_INFO(C, H, 0, Result, OpenCodeFileSystemDeprecated4, (ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const ams::sf::OutBuffer &out_verif, const fssrv::sf::Path &path, fs::ContentAttributes attr, ncm::ProgramId program_id),              (out_fs, out_verif, path, attr, program_id),       hos::Version_17_0_0, hos::Version_19_0_1) \
    AMS_SF_METHOD_INFO(C, H, 0, Result, OpenCodeFileSystem,            (ams::sf::Out<ams::sf::SharedPointer<fssrv::sf::IFileSystem>> out_fs, const ams::sf::OutBuffer &out_verif, fs::ContentAttributes attr, ncm::ProgramId program_id, ncm::StorageId storage_id),                (out_fs, out_verif, attr, program_id, storage_id), hos::Version_20_0_0)                      \
    AMS_SF_METHOD_INFO(C, H, 1, Result, IsArchivedProgram,             (ams::sf::Out<bool> out, u64 process_id),                                                                                                                                                                    (out, process_id))                                                                           \
    AMS_SF_METHOD_INFO(C, H, 2, Result, SetCurrentProcess,             (const ams::sf::ClientProcessId &client_pid),                                                                                                                                                                (client_pid),                                      hos::Version_4_0_0)

AMS_SF_DEFINE_INTERFACE(ams::fssrv::sf, IFileSystemProxyForLoader, AMS_FSSRV_I_FILE_SYSTEM_PROXY_FOR_LOADER_INTERFACE_INFO, 0xDC92EE15)
