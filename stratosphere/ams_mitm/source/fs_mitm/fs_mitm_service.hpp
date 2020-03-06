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
#include <stratosphere.hpp>
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>

namespace ams::mitm::fs {

    using IStorageInterface    = ams::fssrv::impl::StorageInterfaceAdapter;
    using IFileSystemInterface = ams::fssrv::impl::FileSystemInterfaceAdapter;

    /* TODO: Consider re-enabling the mitm flag logic. */

    class FsMitmService  : public sf::IMitmServiceObject {
        private:
            enum class CommandId {
                OpenFileSystemDeprecated        = 0,

                SetCurrentProcess               = 1,
                OpenFileSystemWithPatch         = 7,
                OpenFileSystemWithId            = 8,

                OpenSdCardFileSystem            = 18,

                OpenSaveDataFileSystem          = 51,

                OpenBisStorage                  = 12,
                OpenDataStorageByCurrentProcess = 200,
                OpenDataStorageByDataId         = 202,
            };
        public:
            NX_CONSTEXPR bool ShouldMitmProgramId(const ncm::ProgramId program_id) {
                /* We want to mitm everything that isn't a system-module. */
                if (!ncm::IsSystemProgramId(program_id)) {
                    return true;
                }

                /* We want to mitm ns, to intercept SD card requests. */
                if (program_id == ncm::SystemProgramId::Ns) {
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
            SF_MITM_SERVICE_OBJECT_CTOR(FsMitmService) { /* ... */ }
        protected:
            /* Overridden commands. */
            Result OpenFileSystemWithPatch(sf::Out<std::shared_ptr<IFileSystemInterface>> out, ncm::ProgramId program_id, u32 _filesystem_type);
            Result OpenFileSystemWithId(sf::Out<std::shared_ptr<IFileSystemInterface>> out, const fssrv::sf::Path &path, ncm::ProgramId program_id, u32 _filesystem_type);
            Result OpenSdCardFileSystem(sf::Out<std::shared_ptr<IFileSystemInterface>> out);
            Result OpenSaveDataFileSystem(sf::Out<std::shared_ptr<IFileSystemInterface>> out, u8 space_id, const FsSaveDataAttribute &attribute);
            Result OpenBisStorage(sf::Out<std::shared_ptr<IStorageInterface>> out, u32 bis_partition_id);
            Result OpenDataStorageByCurrentProcess(sf::Out<std::shared_ptr<IStorageInterface>> out);
            Result OpenDataStorageByDataId(sf::Out<std::shared_ptr<IStorageInterface>> out, ncm::DataId data_id, u8 storage_id);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(OpenFileSystemWithPatch, hos::Version_200),
                MAKE_SERVICE_COMMAND_META(OpenFileSystemWithId,    hos::Version_200),
                MAKE_SERVICE_COMMAND_META(OpenSdCardFileSystem),
                MAKE_SERVICE_COMMAND_META(OpenSaveDataFileSystem),
                MAKE_SERVICE_COMMAND_META(OpenBisStorage),
                MAKE_SERVICE_COMMAND_META(OpenDataStorageByCurrentProcess),
                MAKE_SERVICE_COMMAND_META(OpenDataStorageByDataId),
            };
    };

}
