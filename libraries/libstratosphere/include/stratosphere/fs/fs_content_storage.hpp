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
#pragma once
#include <stratosphere/fs/fs_common.hpp>

namespace ams::fs {

    enum class ContentStorageId : u32 {
        System = 0,
        User   = 1,
        SdCard = 2,
    };

    constexpr inline const char * const ContentStorageDirectoryName = "Contents";

    const char *GetContentStorageMountName(ContentStorageId id);

    Result MountContentStorage(ContentStorageId id);
    Result MountContentStorage(const char *name, ContentStorageId id);

}
