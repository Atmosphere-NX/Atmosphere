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
#include <stratosphere.hpp>
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>

#define AMS_FS_MITM_INTERFACE_INFO(C, H)                                                                                                                                                                                                                                               \
    AMS_SF_METHOD_INFO(C, H,   7, Result, OpenFileSystemWithPatch,         (sf::Out<sf::SharedPointer<ams::fssrv::sf::IFileSystem>> out, ncm::ProgramId program_id, u32 _filesystem_type),                              (out, program_id, _filesystem_type),       hos::Version_2_0_0) \
    AMS_SF_METHOD_INFO(C, H,   8, Result, OpenFileSystemWithId,            (sf::Out<sf::SharedPointer<ams::fssrv::sf::IFileSystem>> out, const fssrv::sf::Path &path, ncm::ProgramId program_id, u32 _filesystem_type), (out, path, program_id, _filesystem_type), hos::Version_2_0_0) \
    AMS_SF_METHOD_INFO(C, H,  18, Result, OpenSdCardFileSystem,            (sf::Out<sf::SharedPointer<ams::fssrv::sf::IFileSystem>> out),                                                                               (out))                                                         \
    AMS_SF_METHOD_INFO(C, H,  51, Result, OpenSaveDataFileSystem,          (sf::Out<sf::SharedPointer<ams::fssrv::sf::IFileSystem>> out, u8 space_id, const ams::fs::SaveDataAttribute &attribute),                     (out, space_id, attribute))                                    \
    AMS_SF_METHOD_INFO(C, H,  12, Result, OpenBisStorage,                  (sf::Out<sf::SharedPointer<ams::fssrv::sf::IStorage>> out, u32 bis_partition_id),                                                            (out, bis_partition_id))                                       \
    AMS_SF_METHOD_INFO(C, H, 200, Result, OpenDataStorageByCurrentProcess, (sf::Out<sf::SharedPointer<ams::fssrv::sf::IStorage>> out),                                                                                  (out))                                                         \
    AMS_SF_METHOD_INFO(C, H, 202, Result, OpenDataStorageByDataId,         (sf::Out<sf::SharedPointer<ams::fssrv::sf::IStorage>> out, ncm::DataId data_id, u8 storage_id),                                              (out, data_id, storage_id))                                    \
    AMS_SF_METHOD_INFO(C, H, 205, Result, OpenDataStorageWithProgramIndex, (sf::Out<sf::SharedPointer<ams::fssrv::sf::IStorage>> out, u8 program_index),                                                                (out, program_index),                      hos::Version_7_0_0) \
    AMS_SF_METHOD_INFO(C, H, 810, Result, RegisterProgramIndexMapInfo,     (const sf::InBuffer &info_buffer, s32 info_count),                                                                                           (info_buffer, info_count),                 hos::Version_7_0_0)


AMS_SF_DEFINE_MITM_INTERFACE(ams::mitm::fs, IFsMitmInterface, AMS_FS_MITM_INTERFACE_INFO)

namespace ams::mitm::fs {

    class FsMitmService  : public sf::MitmServiceImplBase {
        public:
            using MitmServiceImplBase::MitmServiceImplBase;
        public:
            static constexpr ALWAYS_INLINE bool ShouldMitmProgramId(const ncm::ProgramId program_id) {
                /* We want to mitm everything that isn't a system-module. */
                if (!ncm::IsSystemProgramId(program_id)) {
                    return true;
                }

                /* We want to mitm ns, to intercept SD card requests and program index map info registration. */
                if (program_id == ncm::SystemProgramId::Ns) {
                    return true;
                }

                /* We want to mitm settings, to intercept CAL0. */
                if (program_id == ncm::SystemProgramId::Settings) {
                    return true;
                }

                /* We want to mitm sdb, to support sd-romfs redirection of common system archives (like system font, etc). */
                if (program_id == ncm::SystemProgramId::Sdb) {
                    return true;
                }

                return false;
            }

            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                static std::atomic_bool has_launched_qlaunch = false;

                /* TODO: intercepting everything seems to cause issues with sleep mode, for some reason. */
                /* Figure out why, and address it. */
                /* TODO: This may be because pre-rewrite code really mismanaged domain objects in a way that would cause bad things. */
                /* Need to verify if this is fixed now. */
                if (client_info.program_id == ncm::SystemAppletId::Qlaunch || client_info.program_id == ncm::SystemAppletId::MaintenanceMenu) {
                    has_launched_qlaunch = true;
                }

                return has_launched_qlaunch || ShouldMitmProgramId(client_info.program_id);
            }
        public:
            /* Overridden commands. */
            Result OpenFileSystemWithPatch(sf::Out<sf::SharedPointer<ams::fssrv::sf::IFileSystem>> out, ncm::ProgramId program_id, u32 _filesystem_type);
            Result OpenFileSystemWithId(sf::Out<sf::SharedPointer<ams::fssrv::sf::IFileSystem>> out, const fssrv::sf::Path &path, ncm::ProgramId program_id, u32 _filesystem_type);
            Result OpenSdCardFileSystem(sf::Out<sf::SharedPointer<ams::fssrv::sf::IFileSystem>> out);
            Result OpenSaveDataFileSystem(sf::Out<sf::SharedPointer<ams::fssrv::sf::IFileSystem>> out, u8 space_id, const ams::fs::SaveDataAttribute &attribute);
            Result OpenBisStorage(sf::Out<sf::SharedPointer<ams::fssrv::sf::IStorage>> out, u32 bis_partition_id);
            Result OpenDataStorageByCurrentProcess(sf::Out<sf::SharedPointer<ams::fssrv::sf::IStorage>> out);
            Result OpenDataStorageByDataId(sf::Out<sf::SharedPointer<ams::fssrv::sf::IStorage>> out, ncm::DataId data_id, u8 storage_id);
            Result OpenDataStorageWithProgramIndex(sf::Out<sf::SharedPointer<ams::fssrv::sf::IStorage>> out, u8 program_index);
            Result RegisterProgramIndexMapInfo(const sf::InBuffer &info_buffer, s32 info_count);
    };
    static_assert(IsIFsMitmInterface<FsMitmService>);

}
