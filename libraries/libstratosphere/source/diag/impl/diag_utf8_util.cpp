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
#include <stratosphere.hpp>

namespace ams::diag::impl {

    namespace {

        bool IsHeadOfCharacter(u8 c) {
            return (c & 0xC0) != 0x80;
        }

        size_t GetCharacterSize(u8 c) {
            if ((c & 0x80) == 0) {
                return 1;
            } else if ((c & 0xE0) == 0xC0) {
                return 2;
            } else if ((c & 0xF0) == 0xE0) {
                return 3;
            } else if ((c & 0xF8) == 0xF0) {
                return 4;
            }

            return 0;
        }

        const char *FindLastCharacterPointer(const char *str, size_t len) {
            /* Find the head of the last character. */
            const char *cur;
            for (cur = str + len - 1; cur >= str && !IsHeadOfCharacter(*reinterpret_cast<const u8 *>(cur)); --cur) {
                /* ... */
            }

            /* Return the last character. */
            if (AMS_LIKELY(cur >= str)) {
                return cur;
            } else {
                return nullptr;
            }
        }

    }

    int GetValidSizeAsUtf8String(const char *str, size_t len) {
        /* Check pre-condition. */
        AMS_ASSERT(str != nullptr);

        /* Check if we have no data. */
        if (len == 0) {
            return 0;
        }

        /* Get the last character pointer. */
        const auto *last_char_ptr = FindLastCharacterPointer(str, len);
        if (last_char_ptr == nullptr) {
            return -1;
        }

        /* Get sizes. */
        const size_t actual_size    = (str + len) - last_char_ptr;
        const size_t last_char_size = GetCharacterSize(*reinterpret_cast<const u8 *>(last_char_ptr));
        if (last_char_size == 0) {
            return -1;
        } else if (actual_size >= last_char_size) {
            if (actual_size == last_char_size) {
                return len;
            } else {
                return -1;
            }
        } else if (actual_size >= len) {
            AMS_ASSERT(actual_size == len);
            return -1;
        } else {
            return static_cast<int>(len - actual_size);
        }
    }

}
