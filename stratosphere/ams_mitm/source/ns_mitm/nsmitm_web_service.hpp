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

#include "ns_shim.h"

class NsDocumentService : public IServiceObject {
    private:
        enum class CommandId {
            GetApplicationContentPath      = 21,
            ResolveApplicationContentPath  = 23,
            GetRunningApplicationProgramId = 92,
        };
    private:
        u64 title_id;
        std::unique_ptr<NsDocumentInterface> srv;
    public:
        NsDocumentService(u64 t, NsDocumentInterface *s) : title_id(t), srv(s) {
            /* ... */
        }

        NsDocumentService(u64 t, std::unique_ptr<NsDocumentInterface> s) : title_id(t), srv(std::move(s)) {
            /* ... */
        }

        NsDocumentService(u64 t, NsDocumentInterface s) : title_id(t) {
            srv = std::make_unique<NsDocumentInterface>(s);
        }

        virtual ~NsDocumentService() {
            serviceClose(&srv->s);
        }
    private:
        /* Actual command API. */
        Result GetApplicationContentPath(OutBuffer<u8> out_path, u64 app_id, u8 storage_type);
        Result ResolveApplicationContentPath(u64 title_id, u8 storage_type);
        Result GetRunningApplicationProgramId(Out<u64> out_tid, u64 app_id);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MAKE_SERVICE_COMMAND_META(NsDocumentService, GetApplicationContentPath),
            MAKE_SERVICE_COMMAND_META(NsDocumentService, ResolveApplicationContentPath),
            MAKE_SERVICE_COMMAND_META(NsDocumentService, GetRunningApplicationProgramId, FirmwareVersion_600),
        };
};

class NsWebMitmService : public IMitmServiceObject {
    private:
        enum class CommandId {
            GetDocumentInterface = 7999,
        };
    public:
        NsWebMitmService(std::shared_ptr<Service> s, u64 pid, sts::ncm::TitleId tid) : IMitmServiceObject(s, pid, tid) {
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
        Result GetDocumentInterface(Out<std::shared_ptr<NsDocumentService>> out_intf);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MAKE_SERVICE_COMMAND_META(NsWebMitmService, GetDocumentInterface, FirmwareVersion_300),
        };
};
