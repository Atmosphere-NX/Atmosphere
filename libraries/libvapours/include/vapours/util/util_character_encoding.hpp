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

#pragma once
#include <vapours/common.hpp>
#include <vapours/assert.hpp>

namespace ams::util {

    enum CharacterEncodingResult {
        CharacterEncodingResult_Success            = 0,
        CharacterEncodingResult_InsufficientLength = 1,
        CharacterEncodingResult_InvalidFormat      = 2,
    };

    namespace impl {

        class CharacterEncodingHelper {
            public:
                static constexpr int8_t Utf8NBytesInnerTable[0x100 + 1] = {
                    -1,
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
                    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 8,
                };

                static constexpr ALWAYS_INLINE char GetUtf8NBytes(size_t i) {
                    return static_cast<char>(Utf8NBytesInnerTable[1 + i]);
                }
        };

    }

    constexpr inline CharacterEncodingResult ConvertCharacterUtf8ToUtf32(u32 *dst, const char *src) {
        /* Check pre-conditions. */
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(src != nullptr);

        /* Perform the conversion. */
        const auto *p = src;
        switch (impl::CharacterEncodingHelper::GetUtf8NBytes(static_cast<unsigned char>(p[0]))) {
            case 1:
                *dst = static_cast<u32>(p[0]);
                return CharacterEncodingResult_Success;
            case 2:
                if ((static_cast<u32>(p[0]) & 0x1E) != 0) {
                    if (impl::CharacterEncodingHelper::GetUtf8NBytes(static_cast<unsigned char>(p[1])) == 0) {
                        *dst = (static_cast<u32>(p[0] & 0x1F) << 6) | (static_cast<u32>(p[1] & 0x3F) << 0);
                        return CharacterEncodingResult_Success;
                    }
                }
                break;
            case 3:
                if (impl::CharacterEncodingHelper::GetUtf8NBytes(static_cast<unsigned char>(p[1])) == 0 && impl::CharacterEncodingHelper::GetUtf8NBytes(static_cast<unsigned char>(p[2])) == 0) {
                    const u32 c = (static_cast<u32>(p[0] & 0xF) << 12) | (static_cast<u32>(p[1] & 0x3F) << 6) | (static_cast<u32>(p[2] & 0x3F) << 0);
                    if ((c & 0xF800) != 0 && (c & 0xF800) != 0xD800) {
                        *dst = c;
                        return CharacterEncodingResult_Success;
                    }
                }
                return CharacterEncodingResult_InvalidFormat;
            case 4:
                if (impl::CharacterEncodingHelper::GetUtf8NBytes(static_cast<unsigned char>(p[1])) == 0 && impl::CharacterEncodingHelper::GetUtf8NBytes(static_cast<unsigned char>(p[2])) == 0 && impl::CharacterEncodingHelper::GetUtf8NBytes(static_cast<unsigned char>(p[3])) == 0) {
                    const u32 c = (static_cast<u32>(p[0] & 0x7) << 18) | (static_cast<u32>(p[1] & 0x3F) << 12) | (static_cast<u32>(p[2] & 0x3F) << 6) | (static_cast<u32>(p[3] & 0x3F) << 0);
                    if (c >= 0x10000 && c < 0x110000) {
                        *dst = c;
                        return CharacterEncodingResult_Success;
                    }
                }
                return CharacterEncodingResult_InvalidFormat;
            default:
                break;
        }

        /* We failed to convert. */
        return CharacterEncodingResult_InvalidFormat;
    }

    constexpr inline CharacterEncodingResult PickOutCharacterFromUtf8String(char *dst, const char **str) {
        /* Check pre-conditions. */
        AMS_ASSERT(dst != nullptr);
        AMS_ASSERT(str != nullptr);
        AMS_ASSERT(*str != nullptr);

        /* Clear the output. */
        dst[0] = 0;
        dst[1] = 0;
        dst[2] = 0;
        dst[3] = 0;

        /* Perform the conversion. */
        const auto *p = *str;
        u32 c = static_cast<u32>(*p);
        switch (impl::CharacterEncodingHelper::GetUtf8NBytes(c)) {
            case 1:
                dst[0] = (*str)[0];
                ++(*str);
                break;
            case 2:
                if ((p[0] & 0x1E) != 0) {
                    if (impl::CharacterEncodingHelper::GetUtf8NBytes(static_cast<unsigned char>(p[1])) == 0) {
                        c = (static_cast<u32>(p[0] & 0x1F) << 6) | (static_cast<u32>(p[1] & 0x3F) << 0);
                        dst[0] = (*str)[0];
                        dst[1] = (*str)[1];
                        (*str) += 2;
                        break;
                    }
                }
                return CharacterEncodingResult_InvalidFormat;
            case 3:
                if (impl::CharacterEncodingHelper::GetUtf8NBytes(static_cast<unsigned char>(p[1])) == 0 && impl::CharacterEncodingHelper::GetUtf8NBytes(static_cast<unsigned char>(p[2])) == 0) {
                    c = (static_cast<u32>(p[0] & 0xF) << 12) | (static_cast<u32>(p[1] & 0x3F) << 6) | (static_cast<u32>(p[2] & 0x3F) << 0);
                    if ((c & 0xF800) != 0 && (c & 0xF800) != 0xD800) {
                        dst[0] = (*str)[0];
                        dst[1] = (*str)[1];
                        dst[2] = (*str)[2];
                        (*str) += 3;
                        break;
                    }
                }
                return CharacterEncodingResult_InvalidFormat;
            case 4:
                if (impl::CharacterEncodingHelper::GetUtf8NBytes(static_cast<unsigned char>(p[1])) == 0 && impl::CharacterEncodingHelper::GetUtf8NBytes(static_cast<unsigned char>(p[2])) == 0 && impl::CharacterEncodingHelper::GetUtf8NBytes(static_cast<unsigned char>(p[3])) == 0) {
                    c = (static_cast<u32>(p[0] & 0x7) << 18) | (static_cast<u32>(p[1] & 0x3F) << 12) | (static_cast<u32>(p[2] & 0x3F) << 6) | (static_cast<u32>(p[3] & 0x3F) << 0);
                    if (c >= 0x10000 && c < 0x110000) {
                        dst[0] = (*str)[0];
                        dst[1] = (*str)[1];
                        dst[2] = (*str)[2];
                        dst[3] = (*str)[3];
                        (*str) += 4;
                        break;
                    }
                }
                return CharacterEncodingResult_InvalidFormat;
            default:
                return CharacterEncodingResult_InvalidFormat;
        }

        return CharacterEncodingResult_Success;
    }

}
