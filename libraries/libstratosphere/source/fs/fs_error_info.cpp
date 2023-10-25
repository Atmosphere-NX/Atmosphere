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
#include <stratosphere.hpp>
#include "fsa/fs_mount_utils.hpp"
#include "impl/fs_file_system_proxy_service_object.hpp"
#include "impl/fs_file_system_service_object_adapter.hpp"

namespace ams::fs {

    Result GetAndClearFileSystemProxyErrorInfo(FileSystemProxyErrorInfo *out) {
        /* Check pre-conditions. */
        AMS_FS_R_UNLESS(out != nullptr, fs::ResultNullptrArgument());

        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Get the error info. */
        AMS_FS_R_TRY(fsp->GetAndClearErrorInfo(out));

        R_SUCCEED();
    }

}
