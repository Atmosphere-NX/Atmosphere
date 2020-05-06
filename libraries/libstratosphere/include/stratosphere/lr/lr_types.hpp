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

namespace ams::lr {

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

        constexpr inline size_t GetLength() const {
            /* Determine length from the first null byte occurence. */
            size_t len = 0;
            for (size_t i = 0; i < sizeof(this->str) - 1 && this->str[i] != '\x00'; i++) {
                len++;
            }
            return len;
        }

        constexpr inline bool IsValid() const {
            /* Determine validity by presence of a terminating null byte. */
            for (size_t i = 0; i < sizeof(this->str); i++) {
                if (this->str[i] == '\x00') {
                    return true;
                }
            }
            return false;
        }
    };

    static_assert(util::is_pod<Path>::value && sizeof(Path) == fs::EntryNameLengthMax);

}
