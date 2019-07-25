/*
 * Copyright (c) 2019 Adubbz
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
#include <stratosphere.hpp>

#include "ncm_types.hpp"

namespace sts::ncm::path {

    void MakeContentPathFlat(char* out_path, ContentId content_id, const char* root);
    void MakeContentPathHashByteLayered(char* out_path, ContentId content_id, const char* root);
    void MakeContentPath10BitLayered(char* out_path, ContentId content_id, const char* root);
    void MakeContentPathDualLayered(char* out_path, ContentId content_id, const char* root);

    void MakePlaceHolderPathFlat(char* out_path, PlaceHolderId placeholder_id, const char* root);
    void MakePlaceHolderPathHashByteLayered(char* out_path, PlaceHolderId placeholder_id, const char* root);

}