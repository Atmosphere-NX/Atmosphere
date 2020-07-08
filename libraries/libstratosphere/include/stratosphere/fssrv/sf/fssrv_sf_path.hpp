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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/fs_directory.hpp>
#include <stratosphere/sf/sf_buffer_tags.hpp>

namespace ams::fssrv::sf {

    struct Path : ams::sf::LargeData {
        char str[fs::EntryNameLengthMax + 1];

        static constexpr Path Encode(const char *p) {
            Path path = {};
            for (size_t i = 0; i < sizeof(path) - 1; i++) {
                path.str[i] = p[i];
                if (p[i] == '\x00') {
                    break;
                }
            }
            return path;
        }

        static constexpr size_t GetPathLength(const Path &path) {
            size_t len = 0;
            for (size_t i = 0; i < sizeof(path) - 1 && path.str[i] != '\x00'; i++) {
                len++;
            }
            return len;
        }
    };

    static_assert(util::is_pod<Path>::value && sizeof(Path) == FS_MAX_PATH);

    using FspPath = Path;

}
