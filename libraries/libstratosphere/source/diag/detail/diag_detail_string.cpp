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
#include <stratosphere.hpp>

namespace ams::diag::detail {

    void PrintDebugString(const char *str, size_t len) {
        AMS_ASSERT(str && len);
        svc::OutputDebugString(str, len);
    }

    ssize_t GetValidSizeAsUtf8String(const char *str, size_t len) {
        AMS_ABORT_UNLESS(str != nullptr);
        if (len == 0) {
            return 0;
        }

        auto str_nul_end = str + len;
        auto str_end = str + len - 1;

        char some_c = '\0';
        do {
            if (str_end < str) {
                return -1;
            }
            some_c = *str_end--;
        } while ((some_c & 0xC0) == 0x80);

        size_t tmp_len = 0;
        if (!(some_c & 0x80)) {
            tmp_len = static_cast<size_t>(str_nul_end - (str_end + 1));
            if (tmp_len < 1) {
                return len;
            }
            if (tmp_len == 1) {
                return len;
            }
            return 1;
        }


        if ((some_c & 0xE0) == 0xC0) {
            tmp_len = static_cast<size_t>(str_nul_end - (str_end + 1));
            if (tmp_len >= 2) {
                if (tmp_len == 2) {
                    return len;
                }
                return 1;
            }
        }
        else {
            if ((some_c & 0xF0) != 0xE0) {
                if ((some_c & 0xF8) != 0xF0) {
                    return -1;
                }
                tmp_len = static_cast<size_t>(str_nul_end - (str_end + 1));
                if (tmp_len < 4) {
                    if (tmp_len < len) {
                        return len - tmp_len;
                    }
                    AMS_ABORT_UNLESS(tmp_len == len);
                    return -1;
                }
                if (tmp_len == 4) {
                    return len;
                }
                return 1;
            }
            tmp_len = static_cast<size_t>(str_nul_end - (str_end + 1));
            if (tmp_len >= 3) {
                if (tmp_len == 3) {
                    return len;
                }
                return 1;
            }
        }

        if (tmp_len < len) {
            return len - tmp_len;
        }
        AMS_ABORT_UNLESS(tmp_len == len);
        return -1;
    }

}
