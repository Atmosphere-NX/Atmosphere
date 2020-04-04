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
#include <vapours.hpp>
#include <stratosphere/ncm/ncm_content_meta_key.hpp>

namespace ams::ncm {

    constexpr inline s64 MaxClusterSize = 256_KB;

    s64 CalculateRequiredSize(s64 file_size, s64 cluster_size = MaxClusterSize);
    s64 CalculateRequiredSizeForExtension(s64 file_size, s64 cluster_size = MaxClusterSize);

    class ContentMetaDatabase;

    Result EstimateRequiredSize(s64 *out_size, const ContentMetaKey &key, ContentMetaDatabase *db);

}
