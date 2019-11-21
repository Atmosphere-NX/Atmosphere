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
#include "nsmitm_am_service.hpp"
#include "ns_shim.h"

void NsAmMitmService::PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx) {
    /* Nothing to do here */
}

Result NsAmMitmService::GetApplicationContentPath(OutBuffer<u8> out_path, u64 app_id, u8 storage_type) {
    return nsamGetApplicationContentPathFwd(this->forward_service.get(), out_path.buffer, out_path.num_elements, app_id, static_cast<FsStorageId>(storage_type));
}

Result NsAmMitmService::ResolveApplicationContentPath(u64 program_id, u8 storage_type) {
    /* Always succeed for web applet asking about HBL. */
    if (Utils::IsWebAppletTid(static_cast<u64>(this->program_id)) && Utils::IsHblTid(program_id)) {
        nsamResolveApplicationContentPathFwd(this->forward_service.get(), program_id, static_cast<FsStorageId>(storage_type));
        return ResultSuccess();
    }

    return nsamResolveApplicationContentPathFwd(this->forward_service.get(), program_id, static_cast<FsStorageId>(storage_type));
}

Result NsAmMitmService::GetRunningApplicationProgramId(Out<u64> out_tid, u64 app_id) {
    return nsamGetRunningApplicationProgramIdFwd(this->forward_service.get(), out_tid.GetPointer(), app_id);
}