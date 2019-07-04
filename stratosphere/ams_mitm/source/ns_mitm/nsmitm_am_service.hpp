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

#include "../utils.hpp"

class NsAmMitmService : public IMitmServiceObject {
    private:
        enum class CommandId {
            GetApplicationContentPath      = 21,
            ResolveApplicationContentPath  = 23,
            GetRunningApplicationProgramId = 92,
        };
    public:
        NsAmMitmService(std::shared_ptr<Service> s, u64 pid, sts::ncm::TitleId tid) : IMitmServiceObject(s, pid, tid) {
            /* ... */
        }

        static bool ShouldMitm(u64 pid, sts::ncm::TitleId tid) {
            /* We will mitm:
             * - web applets, to facilitate hbl web browser launching.
             */
            return Utils::IsWebAppletTid(static_cast<u64>(tid));
        }

        static void PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx);

    protected:
        /* Overridden commands. */
        Result GetApplicationContentPath(OutBuffer<u8> out_path, u64 app_id, u8 storage_type);
        Result ResolveApplicationContentPath(u64 title_id, u8 storage_type);
        Result GetRunningApplicationProgramId(Out<u64> out_tid, u64 app_id);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MAKE_SERVICE_COMMAND_META(NsAmMitmService, GetApplicationContentPath),
            MAKE_SERVICE_COMMAND_META(NsAmMitmService, ResolveApplicationContentPath),
            MAKE_SERVICE_COMMAND_META(NsAmMitmService, GetRunningApplicationProgramId, FirmwareVersion_600),
        };
};
