/*
 * Copyright (c) 2018 Atmosphère-NX
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
    std::shared_ptr<NsDocumentService> intf = nullptr;
    u32 out_domain_id = 0;
    Result rc = ResultSuccess;
    
    ON_SCOPE_EXIT {
        if (R_SUCCEEDED(rc)) {
            out_intf.SetValue(std::move(intf));
            if (out_intf.IsDomain()) {
                out_intf.ChangeObjectId(out_domain_id);
            }
        }
    };
    
    /* Open a document interface. */
    NsDocumentInterface doc;
    rc = nsGetDocumentInterfaceFwd(this->forward_service.get(), &doc);
    if (R_SUCCEEDED(rc)) {
        intf = std::make_shared<NsDocumentService>(this->title_id, doc);
        if (out_intf.IsDomain()) {
            out_domain_id = doc.s.object_id;
        }
    }

    return rc;
}

Result NsDocumentService::GetApplicationContentPath(OutBuffer<u8> out_path, u64 app_id, u8 storage_type) {
    return nswebGetApplicationContentPath(this->srv.get(), out_path.buffer, out_path.num_elements, app_id, static_cast<FsStorageId>(storage_type));
}

Result NsDocumentService::ResolveApplicationContentPath(u64 title_id, u8 storage_type) {
    Result rc = nswebResolveApplicationContentPath(this->srv.get(), title_id, static_cast<FsStorageId>(storage_type));

    /* Always succeed for web applet asking about HBL. */
    return (Utils::IsWebAppletTid(this->title_id) && Utils::IsHblTid(title_id)) ? 0 : rc;
}

Result NsDocumentService::GetRunningApplicationProgramId(Out<u64> out_tid, u64 app_id) {
    return nswebGetRunningApplicationProgramId(this->srv.get(), out_tid.GetPointer(), app_id);
}