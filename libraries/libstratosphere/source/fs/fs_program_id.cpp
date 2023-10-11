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
#include <stratosphere/fs/fs_rights_id.hpp>
#include "impl/fs_file_system_proxy_service_object.hpp"

namespace ams::fs {

    Result GetProgramId(ncm::ProgramId *out, const char *path, fs::ContentAttributes attr) {
        AMS_FS_R_UNLESS(out != nullptr,  fs::ResultNullptrArgument());
        AMS_FS_R_UNLESS(path != nullptr, fs::ResultNullptrArgument());

        /* Convert the path for fsp. */
        fssrv::sf::FspPath sf_path;
        R_TRY(fs::ConvertToFspPath(std::addressof(sf_path), path));

        auto fsp = impl::GetFileSystemProxyServiceObject();
        AMS_FS_R_TRY(fsp->GetProgramId(out, sf_path, attr));

        R_SUCCEED();
    }

}