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
#include <vapours.hpp>
#include <stratosphere/os.hpp>
#include <stratosphere/pm.hpp>
#include <stratosphere/ncm.hpp>

namespace ams::pgl {

    enum LaunchFlags : u8 {
        LaunchFlags_None                                     = 0,
        LaunchFlags_EnableDetailedCrashReport                = (1 << 0),
        LaunchFlags_EnableCrashReportScreenShotForProduction = (1 << 1),
        LaunchFlags_EnableCrashReportScreenShotForDevelop    = (1 << 2),
    };

    enum class SnapShotDumpType : u32 {
        None = 0,
        Auto = 1,
        Full = 2,
    };

    /* TODO: Is this really nn::ncm::Content<Something>Info? */
    struct ContentMetaInfo {
        u64 id;
        u32 version;
        ncm::ContentType content_type;
        u8 id_offset;
        u8 reserved_0E[2];

        static constexpr ContentMetaInfo Make(u64 id, u32 version, ncm::ContentType content_type, u8 id_offset) {
            return {
                .id           = id,
                .version      = version,
                .content_type = content_type,
                .id_offset    = id_offset,
            };
        }
    };
    static_assert(sizeof(ContentMetaInfo) == 0x10 && util::is_pod<ContentMetaInfo>::value);

}
