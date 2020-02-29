/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

namespace ams::fs {

    Result GetRightsId(RightsId *out, const char *path) {
        static_assert(sizeof(RightsId) == sizeof(::FsRightsId));
        return fsGetRightsIdByPath(path, reinterpret_cast<::FsRightsId *>(out));
    }

    Result GetRightsId(RightsId *out, u8 *out_key_generation, const char *path) {
        static_assert(sizeof(RightsId) == sizeof(::FsRightsId));
        return fsGetRightsIdAndKeyGenerationByPath(path, out_key_generation, reinterpret_cast<::FsRightsId *>(out));
    }

}