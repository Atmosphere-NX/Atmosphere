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
#include <vapours.hpp>

namespace ams::util {

    namespace {

        constexpr inline const u8 CodePointByteLengthTable[0x100] = {
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
            4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        };

        constexpr ALWAYS_INLINE size_t GetCodePointByteLength(u8 c) {
            return CodePointByteLengthTable[c];
        }

        constexpr ALWAYS_INLINE bool IsValidTail(u8 c) {
            return (c & 0xC0) == 0x80;
        }

        constexpr inline bool VerifyCode(const u8 *code, size_t size) {
            switch (size) {
                case 1:
                    break;
                case 2:
                    if (!IsValidTail(code[1])) {
                        return false;
                    }
                    break;
                case 3:
                    if (code[0] == 0xE0 && (code[1] & 0x20) == 0x00) {
                        return false;
                    }
                    if (code[0] == 0xED && (code[1] & 0x20) != 0x00) {
                        return false;
                    }
                    if (!IsValidTail(code[1]) || !IsValidTail(code[2])) {
                        return false;
                    }
                    break;
                case 4:
                    if (code[0] == 0xF0 && (code[1] & 0x30) == 0x00) {
                        return false;
                    }
                    if (code[0] == 0xF4 && (code[1] & 0x30) != 0x00) {
                        return false;
                    }
                    if (!IsValidTail(code[1]) || !IsValidTail(code[2]) || !IsValidTail(code[3])) {
                        return false;
                    }
                    break;
                default:
                    return false;
            }

            return true;
        }

    }

    bool VerifyUtf8String(const char *str, size_t size) {
        return GetCodePointCountOfUtf8String(str, size) != -1;
    }

    int GetCodePointCountOfUtf8String(const char *str, size_t size) {
        /* Check pre-conditions. */
        AMS_ASSERT(str != nullptr);
        AMS_ASSERT(size > 0);

        /* Parse codepoints. */
        int count = 0;

        while (size > 0) {
            /* Get and check the current codepoint. */
            const u8 *code         = reinterpret_cast<const u8 *>(str);
            const size_t code_size = GetCodePointByteLength(code[0]);

            if (code_size > size || !VerifyCode(code, code_size)) {
                return -1;
            }

            /* Advance. */
            str += code_size;
            size -= code_size;

            /* Increment count. */
            ++count;
        }

        return count;
    }

}
