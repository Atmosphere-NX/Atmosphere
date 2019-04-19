/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>
#include <stratosphere.hpp>
#include "fs_istorage.hpp"
#include "fs_ifilesystem.hpp"
#include "../utils.hpp"

enum FspSrvCmd : u32 {
    FspSrvCmd_OpenFileSystemDeprecated = 0,

    FspSrvCmd_SetCurrentProcess = 1,

    FspSrvCmd_OpenFileSystemWithPatch = 7,
    FspSrvCmd_OpenFileSystemWithId = 8,

    FspSrvCmd_OpenSaveDataFileSystem = 51,

    FspSrvCmd_OpenBisStorage = 12,
    FspSrvCmd_OpenDataStorageByCurrentProcess = 200,
    FspSrvCmd_OpenDataStorageByDataId = 202,
};

class FsMitmService : public IMitmServiceObject {
    private:
        static constexpr const char *AtmosphereHblWebContentDir = "/atmosphere/hbl_html";
    private:
        bool has_initialized = false;
        bool should_override_contents;
    public:
        FsMitmService(std::shared_ptr<Service> s, u64 pid) : IMitmServiceObject(s, pid) {
            if (Utils::HasSdDisableMitMFlag(this->title_id)) {
                this->should_override_contents = false;
            } else {
                this->should_override_contents = (this->title_id >= TitleId_ApplicationStart || Utils::HasSdMitMFlag(this->title_id)) && Utils::HasOverrideButton(this->title_id);
            }
        }

        static bool ShouldMitm(u64 pid, u64 tid) {
            /* Don't Mitm KIPs */
            if (pid < 0x50) {
                return false;
            }

            static std::atomic_bool has_launched_qlaunch = false;

            /* TODO: intercepting everything seems to cause issues with sleep mode, for some reason. */
            /* Figure out why, and address it. */
            if (tid == TitleId_AppletQlaunch || tid == TitleId_AppletMaintenanceMenu) {
                has_launched_qlaunch = true;
            }

            return has_launched_qlaunch || tid == TitleId_Ns || tid >= TitleId_ApplicationStart || Utils::HasSdMitMFlag(tid);
        }

        static void PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx);
    private:
        Result OpenHblWebContentFileSystem(Out<std::shared_ptr<IFileSystemInterface>> &out);
    protected:
        /* Overridden commands. */
        Result OpenFileSystemWithPatch(Out<std::shared_ptr<IFileSystemInterface>> out, u64 title_id, u32 filesystem_type);
        Result OpenFileSystemWithId(Out<std::shared_ptr<IFileSystemInterface>> out, InPointer<char> path, u64 title_id, u32 filesystem_type);
        Result OpenSaveDataFileSystem(Out<std::shared_ptr<IFileSystemInterface>> out, u8 space_id, FsSave save_struct);
        Result OpenBisStorage(Out<std::shared_ptr<IStorageInterface>> out, u32 bis_partition_id);
        Result OpenDataStorageByCurrentProcess(Out<std::shared_ptr<IStorageInterface>> out);
        Result OpenDataStorageByDataId(Out<std::shared_ptr<IStorageInterface>> out, u64 data_id, u8 storage_id);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            /* TODO MakeServiceCommandMeta<FspSrvCmd_OpenFileSystemDeprecated, &FsMitmService::OpenFileSystemDeprecated>(), */
            MakeServiceCommandMeta<FspSrvCmd_OpenFileSystemWithPatch, &FsMitmService::OpenFileSystemWithPatch, FirmwareVersion_200>(),
            MakeServiceCommandMeta<FspSrvCmd_OpenFileSystemWithId, &FsMitmService::OpenFileSystemWithId, FirmwareVersion_200>(),
            MakeServiceCommandMeta<FspSrvCmd_OpenSaveDataFileSystem, &FsMitmService::OpenSaveDataFileSystem>(),
            MakeServiceCommandMeta<FspSrvCmd_OpenBisStorage, &FsMitmService::OpenBisStorage>(),
            MakeServiceCommandMeta<FspSrvCmd_OpenDataStorageByCurrentProcess, &FsMitmService::OpenDataStorageByCurrentProcess>(),
            MakeServiceCommandMeta<FspSrvCmd_OpenDataStorageByDataId, &FsMitmService::OpenDataStorageByDataId>(),
        };
};
