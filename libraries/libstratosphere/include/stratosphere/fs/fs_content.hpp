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
#pragma once
#include "fs_common.hpp"
#include <stratosphere/ncm/ncm_ids.hpp>

namespace ams::fs {

    enum ContentType {
        ContentType_Meta    = 0,
        ContentType_Control = 1,
        ContentType_Manual  = 2,
        ContentType_Logo    = 3,
        ContentType_Data    = 4,
    };

    Result MountContent(const char *name, const char *path, ContentType content_type);
    Result MountContent(const char *name, const char *path, ncm::ProgramId id, ContentType content_type);
    Result MountContent(const char *name, const char *path, ncm::DataId id, ContentType content_type);

}
