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

namespace ams::fs {

    namespace {

        class PathVerifier {
            private:
                u32 invalid_chars[6];
                u32 separators[2];
            public:
                PathVerifier() {
                    /* Convert all invalid characters. */
                    u32 *dst_invalid = this->invalid_chars;
                    for (const char *cur = ":*?<>|"; *cur != '\x00'; ++cur) {
                        AMS_ASSERT(dst_invalid < std::end(this->invalid_chars));
                        const auto result = util::ConvertCharacterUtf8ToUtf32(dst_invalid, cur);
                        AMS_ASSERT(result == util::CharacterEncodingResult_Success);
                        ++dst_invalid;
                    }
                    AMS_ASSERT(dst_invalid == std::end(this->invalid_chars));

                    /* Convert all separators. */
                    u32 *dst_sep = this->separators;
                    for (const char *cur = "/\\"; *cur != '\x00'; ++cur) {
                        AMS_ASSERT(dst_sep < std::end(this->separators));
                        const auto result = util::ConvertCharacterUtf8ToUtf32(dst_sep, cur);
                        AMS_ASSERT(result == util::CharacterEncodingResult_Success);
                        ++dst_sep;
                    }
                    AMS_ASSERT(dst_sep == std::end(this->separators));
                }

                Result Verify(const char *path, int max_path_len, int max_name_len) const {
                    AMS_ASSERT(path != nullptr);

                    auto cur      = path;
                    auto name_len = 0;

                    for (auto path_len = 0; path_len <= max_path_len && name_len <= max_name_len; ++path_len) {
                        /* We're done, if the path is terminated. */
                        R_SUCCEED_IF(*cur == '\x00');

                        /* Get the current utf-8 character. */
                        util::CharacterEncodingResult result;
                        char char_buf[4] = {};
                        result = util::PickOutCharacterFromUtf8String(char_buf, std::addressof(cur));
                        R_UNLESS(result == util::CharacterEncodingResult_Success, fs::ResultInvalidCharacter());

                        /* Convert the current utf-8 character to utf-32. */
                        u32 path_char = 0;
                        result = util::ConvertCharacterUtf8ToUtf32(std::addressof(path_char), char_buf);
                        R_UNLESS(result == util::CharacterEncodingResult_Success, fs::ResultInvalidCharacter());

                        /* Check if the character is invalid. */
                        for (const auto invalid : this->invalid_chars) {
                            R_UNLESS(path_char != invalid, fs::ResultInvalidCharacter());
                        }

                        /* Increment name length. */
                        ++name_len;

                        /* Check for separator. */
                        for (const auto sep : this->separators) {
                            if (path_char == sep) {
                                name_len = 0;
                                break;
                            }
                        }
                    }

                    /* The path was too long. */
                    return fs::ResultTooLongPath();
                }
        };

        PathVerifier g_path_verifier;

    }

    Result VerifyPath(const char *path, int max_path_len, int max_name_len) {
        return g_path_verifier.Verify(path, max_path_len, max_name_len);
    }

    bool IsSubPath(const char *lhs, const char *rhs) {
        AMS_ASSERT(lhs != nullptr);
        AMS_ASSERT(rhs != nullptr);

        /* Special case certain paths. */
        if (IsUnc(lhs) && !IsUnc(rhs)) {
            return false;
        }
        if (!IsUnc(lhs) && IsUnc(rhs)) {
            return false;
        }
        if (PathNormalizer::IsSeparator(lhs[0]) && PathNormalizer::IsNullTerminator(lhs[1]) && PathNormalizer::IsSeparator(rhs[0]) && !PathNormalizer::IsNullTerminator(rhs[1])) {
            return true;
        }
        if (PathNormalizer::IsSeparator(rhs[0]) && PathNormalizer::IsNullTerminator(rhs[1]) && PathNormalizer::IsSeparator(lhs[0]) && !PathNormalizer::IsNullTerminator(lhs[1])) {
            return true;
        }

        /* Check subpath. */
        for (size_t i = 0; /* No terminating condition */; i++) {
            if (PathNormalizer::IsNullTerminator(lhs[i])) {
                return PathNormalizer::IsSeparator(rhs[i]);
            } else if (PathNormalizer::IsNullTerminator(rhs[i])) {
                return PathNormalizer::IsSeparator(lhs[i]);
            } else if (lhs[i] != rhs[i]) {
                return false;
            }
        }
    }

}
