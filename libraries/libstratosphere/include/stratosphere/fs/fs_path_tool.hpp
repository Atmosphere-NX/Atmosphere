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
#include "fs_common.hpp"
#include "../fssrv/fssrv_sf_path.hpp"

namespace ams::fs {

    namespace StringTraits {

        constexpr inline char DirectorySeparator = '/';
        constexpr inline char DriveSeparator     = ':';
        constexpr inline char Dot                = '.';
        constexpr inline char NullTerminator     = '\x00';

        constexpr inline char AlternateDirectorySeparator = '\\';
    }

    class PathTool {
        public:
            static constexpr const char RootPath[] = "/";
        public:
            static constexpr inline bool IsAlternateSeparator(char c) {
                return c == StringTraits::AlternateDirectorySeparator;
            }

            static constexpr inline bool IsSeparator(char c) {
                return c == StringTraits::DirectorySeparator;
            }

            static constexpr inline bool IsAnySeparator(char c) {
                return IsSeparator(c) || IsAlternateSeparator(c);
            }

            static constexpr inline bool IsNullTerminator(char c) {
                return c == StringTraits::NullTerminator;
            }

            static constexpr inline bool IsDot(char c) {
                return c == StringTraits::Dot;
            }

            static constexpr inline bool IsWindowsDriveCharacter(char c) {
                return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
            }

            static constexpr inline bool IsDriveSeparator(char c) {
                return c == StringTraits::DriveSeparator;
            }

            static constexpr inline bool IsWindowsAbsolutePath(const char *p) {
                return IsWindowsDriveCharacter(p[0]) && IsDriveSeparator(p[1]);
            }

            static constexpr inline bool IsUnc(const char *p) {
                return (IsSeparator(p[0]) && IsSeparator(p[1])) || (IsAlternateSeparator(p[0]) && IsAlternateSeparator(p[1]));
            }

            static constexpr inline bool IsCurrentDirectory(const char *p) {
                return IsDot(p[0]) && (IsSeparator(p[1]) || IsNullTerminator(p[1]));
            }

            static constexpr inline bool IsParentDirectory(const char *p) {
                return IsDot(p[0]) && IsDot(p[1]) && (IsSeparator(p[2]) || IsNullTerminator(p[2]));
            }

            static Result Normalize(char *out, size_t *out_len, const char *src, size_t max_out_size, bool unc_preserved = false);
            static Result IsNormalized(bool *out, const char *path);

            static bool IsSubPath(const char *lhs, const char *rhs);
    };

}
