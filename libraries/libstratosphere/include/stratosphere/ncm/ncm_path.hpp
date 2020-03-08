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
#include <stratosphere/fs/fs_directory.hpp>
#include <stratosphere/sf/sf_buffer_tags.hpp>

namespace ams::ncm {

    struct alignas(4) Path : ams::sf::LargeData {
        char str[fs::EntryNameLengthMax];

        static constexpr Path Encode(const char *p) {
            Path path = {};
            /* Copy C string to path, terminating when a null byte is found. */
            for (size_t i = 0; i < sizeof(path) - 1; i++) {
                path.str[i] = p[i];
                if (p[i] == '\x00') {
                    break;
                }
            }
            return path;
        }
    };

}
