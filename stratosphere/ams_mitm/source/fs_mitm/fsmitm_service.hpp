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

class FsMitmService : public IMitmServiceObject {
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
    private:
        static constexpr const char *AtmosphereHblWebContentDir = "/atmosphere/hbl_html";
    private:
        bool has_initialized = false;
        bool should_override_contents;
    public:
        FsMitmService(std::shared_ptr<Service> s, u64 pid, sts::ncm::TitleId tid) : IMitmServiceObject(s, pid, tid) {
            if (Utils::HasSdDisableMitMFlag(static_cast<u64>(this->title_id))) {
                this->should_override_contents = false;
            } else {
                this->should_override_contents = (this->title_id >= sts::ncm::TitleId::ApplicationStart || Utils::HasSdMitMFlag(static_cast<u64>(this->title_id))) && Utils::HasOverrideButton(static_cast<u64>(this->title_id));
            }
        }

        static bool ShouldMitm(u64 pid, sts::ncm::TitleId tid) {
            /* Don't Mitm KIPs */
            if (pid < 0x50) {
                return false;
            }

            static std::atomic_bool has_launched_qlaunch = false;

            /* TODO: intercepting everything seems to cause issues with sleep mode, for some reason. */
            /* Figure out why, and address it. */
            if (tid == sts::ncm::TitleId::AppletQlaunch || tid == sts::ncm::TitleId::AppletMaintenanceMenu) {
                has_launched_qlaunch = true;
            }

            return has_launched_qlaunch || tid == sts::ncm::TitleId::Ns || tid >= sts::ncm::TitleId::ApplicationStart || Utils::HasSdMitMFlag(static_cast<u64>(tid));
        }

        static void PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx);
    private:
        Result OpenHblWebContentFileSystem(Out<std::shared_ptr<IFileSystemInterface>> &out);
    protected:
        /* Overridden commands. */
        Result OpenFileSystemWithPatch(Out<std::shared_ptr<IFileSystemInterface>> out, u64 title_id, u32 filesystem_type);
        Result OpenFileSystemWithId(Out<std::shared_ptr<IFileSystemInterface>> out, InPointer<char> path, u64 title_id, u32 filesystem_type);
        Result OpenSdCardFileSystem(Out<std::shared_ptr<IFileSystemInterface>> out);
        Result OpenSaveDataFileSystem(Out<std::shared_ptr<IFileSystemInterface>> out, u8 space_id, FsSave save_struct);
        Result OpenBisStorage(Out<std::shared_ptr<IStorageInterface>> out, u32 bis_partition_id);
        Result OpenDataStorageByCurrentProcess(Out<std::shared_ptr<IStorageInterface>> out);
        Result OpenDataStorageByDataId(Out<std::shared_ptr<IStorageInterface>> out, u64 data_id, u8 storage_id);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            /* TODO MAKE_SERVICE_COMMAND_META(FsMitmService, OpenFileSystemDeprecated), */
            MAKE_SERVICE_COMMAND_META(FsMitmService, OpenFileSystemWithPatch, FirmwareVersion_200),
            MAKE_SERVICE_COMMAND_META(FsMitmService, OpenFileSystemWithId, FirmwareVersion_200),
            MAKE_SERVICE_COMMAND_META(FsMitmService, OpenSdCardFileSystem),
            MAKE_SERVICE_COMMAND_META(FsMitmService, OpenSaveDataFileSystem),
            MAKE_SERVICE_COMMAND_META(FsMitmService, OpenBisStorage),
            MAKE_SERVICE_COMMAND_META(FsMitmService, OpenDataStorageByCurrentProcess),
            MAKE_SERVICE_COMMAND_META(FsMitmService, OpenDataStorageByDataId),
        };
};
