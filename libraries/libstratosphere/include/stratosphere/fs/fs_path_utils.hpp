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
#include <stratosphere/fssrv/sf/fssrv_sf_path.hpp>

namespace ams::fs {

    namespace StringTraits {

        constexpr inline char DirectorySeparator = '/';
        constexpr inline char DriveSeparator     = ':';
        constexpr inline char Dot                = '.';
        constexpr inline char NullTerminator     = '\x00';

        constexpr inline char AlternateDirectorySeparator = '\\';

    }

    /* Windows path utilities. */
    constexpr inline bool IsWindowsDrive(const char *path) {
        AMS_ASSERT(path != nullptr);

        const char c = path[0];
        return (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')) && path[1] == StringTraits::DriveSeparator;
    }

    constexpr inline bool IsUnc(const char *path) {
        return (path[0] == StringTraits::DirectorySeparator && path[1] == StringTraits::DirectorySeparator) ||
               (path[0] == StringTraits::AlternateDirectorySeparator && path[1] == StringTraits::AlternateDirectorySeparator);
    }

    constexpr inline s64 GetWindowsPathSkipLength(const char *path) {
        if (IsWindowsDrive(path)) {
            return 2;
        }
        if (IsUnc(path)) {
            for (s64 i = 2; path[i] != StringTraits::NullTerminator; ++i) {
                if (path[i] == '$' || path[i] == ':') {
                    return i + 1;
                }
            }
        }
        return 0;
    }

    /* Path utilities. */
    inline void Replace(char *dst, size_t dst_size, char old_char, char new_char) {
        AMS_ASSERT(dst != nullptr);
        for (char *cur = dst; cur < dst + dst_size && *cur != StringTraits::NullTerminator; ++cur) {
            if (*cur == old_char) {
                *cur = new_char;
            }
        }
    }

    Result FspPathPrintf(fssrv::sf::FspPath *dst, const char *format, ...) __attribute__((format(printf, 2, 3)));

    inline Result FspPathPrintf(fssrv::sf::FspPath *dst, const char *format, ...) {
        AMS_ASSERT(dst != nullptr);

        /* Format the path. */
        std::va_list va_list;
        va_start(va_list, format);
        const size_t len = std::vsnprintf(dst->str, sizeof(dst->str), format, va_list);
        va_end(va_list);

        /* Validate length. */
        R_UNLESS(len < sizeof(dst->str), fs::ResultTooLongPath());

        /* Fix slashes. */
        Replace(dst->str, sizeof(dst->str) - 1, '\\', '/');

        return ResultSuccess();
    }

    Result VerifyPath(const char *path, int max_path_len, int max_name_len);

    bool IsSubPath(const char *lhs, const char *rhs);

    /* Path normalization. */
    class PathNormalizer {
        public:
            static constexpr const char RootPath[] = "/";
        public:
            static constexpr inline bool IsSeparator(char c) {
                return c == StringTraits::DirectorySeparator;
            }

            static constexpr inline bool IsAnySeparator(char c) {
                return c == StringTraits::DirectorySeparator || c == StringTraits::AlternateDirectorySeparator;
            }

            static constexpr inline bool IsNullTerminator(char c) {
                return c == StringTraits::NullTerminator;
            }

            static constexpr inline bool IsCurrentDirectory(const char *p) {
                return p[0] == StringTraits::Dot && (IsSeparator(p[1]) || IsNullTerminator(p[1]));
            }

            static constexpr inline bool IsParentDirectory(const char *p) {
                return p[0] == StringTraits::Dot && p[1] == StringTraits::Dot && (IsSeparator(p[2]) || IsNullTerminator(p[2]));
            }

            static Result Normalize(char *out, size_t *out_len, const char *src, size_t max_out_size, bool unc_preserved = false, bool has_mount_name = false);
            static Result IsNormalized(bool *out, const char *path, bool unc_preserved = false, bool has_mount_name = false);
    };

}
