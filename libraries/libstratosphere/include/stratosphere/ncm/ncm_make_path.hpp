/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include <stratosphere/ncm/ncm_content_id.hpp>
#include <stratosphere/ncm/ncm_placeholder_id.hpp>
#include <stratosphere/ncm/ncm_path_string.hpp>

namespace ams::ncm {

    using MakeContentPathFunction        = void (*)(PathString *out, ContentId content_id, const char *root_path);
    using MakePlaceHolderPathFunction    = void (*)(PathString *out, PlaceHolderId placeholder_id,const char *root_path);

    void MakeFlatContentFilePath(PathString *out, ContentId content_id, const char *root_path);
    void MakeSha256HierarchicalContentFilePath_ForFat4KCluster(PathString *out, ContentId content_id, const char *root_path);
    void MakeSha256HierarchicalContentFilePath_ForFat16KCluster(PathString *out, ContentId content_id, const char *root_path);
    void MakeSha256HierarchicalContentFilePath_ForFat32KCluster(PathString *out, ContentId content_id, const char *root_path);

    size_t GetHierarchicalContentDirectoryDepth(MakeContentPathFunction func);

    void MakeFlatPlaceHolderFilePath(PathString *out, PlaceHolderId placeholder_id, const char *root_path);
    void MakeSha256HierarchicalPlaceHolderFilePath_ForFat16KCluster(PathString *out, PlaceHolderId placeholder_id, const char *root_pathroot);

    size_t GetHierarchicalPlaceHolderDirectoryDepth(MakePlaceHolderPathFunction func);

}
