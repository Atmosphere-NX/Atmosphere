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

#include <string.h>
#include <switch.h>
#include "ns_shim.h"

/* Command forwarders. */
Result nsGetDocumentInterfaceFwd(Service* s, NsDocumentInterface* out) {
    return serviceDispatch(s, 7999,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}

static Result _nsGetApplicationContentPath(Service *s, void* out, size_t out_size, u64 app_id, NcmContentType content_type) {
    const struct {
        u8 content_type;
        u64 app_id;
    } in = { content_type, app_id };
    return serviceDispatchIn(s, 21, in,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { out, out_size } },
    );
}


static Result _nsResolveApplicationContentPath(Service* s, u64 app_id, NcmContentType content_type) {
    const struct {
        u8 content_type;
        u64 app_id;
    } in = { content_type, app_id };
    return serviceDispatchIn(s, 23, in);
}

static Result _nsGetRunningApplicationProgramId(Service* s, u64* out_program_id, u64 app_id) {
    if (hosversionBefore(6, 0, 0)) {
        return MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer);
    }
    return serviceDispatchInOut(s, 92, app_id, *out_program_id);
}

/* Application Manager forwarders. */
Result nsamGetApplicationContentPathFwd(Service* s, void* out, size_t out_size, u64 app_id, NcmContentType content_type) {
    return _nsGetApplicationContentPath(s, out, out_size, app_id, content_type);
}

Result nsamResolveApplicationContentPathFwd(Service* s, u64 app_id, NcmContentType content_type) {
    return _nsResolveApplicationContentPath(s, app_id, content_type);
}

Result nsamGetRunningApplicationProgramIdFwd(Service* s, u64* out_program_id, u64 app_id) {
    return _nsGetRunningApplicationProgramId(s, out_program_id, app_id);
}

/* Web forwarders */
Result nswebGetApplicationContentPath(NsDocumentInterface* doc, void* out, size_t out_size, u64 app_id, NcmContentType content_type) {
    return _nsGetApplicationContentPath(&doc->s, out, out_size, app_id, content_type);
}

Result nswebResolveApplicationContentPath(NsDocumentInterface* doc, u64 app_id, NcmContentType content_type) {
    return _nsResolveApplicationContentPath(&doc->s, app_id, content_type);
}

Result nswebGetRunningApplicationProgramId(NsDocumentInterface* doc, u64* out_program_id, u64 app_id) {
    return _nsGetRunningApplicationProgramId(&doc->s, out_program_id, app_id);
}

void nsDocumentInterfaceClose(NsDocumentInterface* doc) {
    serviceClose(&doc->s);
}

