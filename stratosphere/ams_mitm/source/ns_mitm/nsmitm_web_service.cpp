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

#include <mutex>
#include <switch.h>
#include <stratosphere.hpp>
#include "nsmitm_web_service.hpp"

void NsWebMitmService::PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx) {
    /* Nothing to do here */
}

Result NsWebMitmService::GetDocumentInterface(Out<std::shared_ptr<NsDocumentService>> out_intf) {
    /* Open a document interface. */
    NsDocumentInterface doc;
    R_TRY(nsGetDocumentInterfaceFwd(this->forward_service.get(), &doc));

    /* Set output interface. */
    out_intf.SetValue(std::move(std::make_shared<NsDocumentService>(static_cast<u64>(this->title_id), doc)));
    if (out_intf.IsDomain()) {
        out_intf.ChangeObjectId(doc.s.object_id);
    }

    return ResultSuccess;
}

Result NsDocumentService::GetApplicationContentPath(OutBuffer<u8> out_path, u64 app_id, u8 storage_type) {
    return nswebGetApplicationContentPath(this->srv.get(), out_path.buffer, out_path.num_elements, app_id, static_cast<FsStorageId>(storage_type));
}

Result NsDocumentService::ResolveApplicationContentPath(u64 title_id, u8 storage_type) {
    /* Always succeed for web applet asking about HBL. */
    if (Utils::IsWebAppletTid(static_cast<u64>(this->title_id)) && Utils::IsHblTid(title_id)) {
        nswebResolveApplicationContentPath(this->srv.get(), title_id, static_cast<FsStorageId>(storage_type));
        return ResultSuccess;
    }

    return nswebResolveApplicationContentPath(this->srv.get(), title_id, static_cast<FsStorageId>(storage_type));
}

Result NsDocumentService::GetRunningApplicationProgramId(Out<u64> out_tid, u64 app_id) {
    return nswebGetRunningApplicationProgramId(this->srv.get(), out_tid.GetPointer(), app_id);
}