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

#include "fs_path_utils.hpp"
#include "fs_ifilesystem.hpp"

class FsSaveUtils {
    private:
        static Result GetSaveDataSpaceIdString(const char **out_str, u8 space_id);
        static Result GetSaveDataTypeString(const char **out_str, u8 save_data_type);
    public:
        static Result GetSaveDataDirectoryPath(FsPath &out_path, u8 space_id, u8 save_data_type, u64 title_id, u128 user_id, u64 save_id);
};
