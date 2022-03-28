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
#include <stratosphere/fs/common/fs_dbm_rom_types.hpp>

namespace ams::fs::RomPathTool {

    /* ACCURATE_TO_VERSION: 14.3.0.0 */

    constexpr inline u32 MaxPathLength = 0x300;

    constexpr ALWAYS_INLINE bool IsSeparator(RomPathChar c) {
        return c == RomStringTraits::DirectorySeparator;
    }

    constexpr ALWAYS_INLINE bool IsNullTerminator(RomPathChar c) {
        return c == RomStringTraits::NullTerminator;
    }

    constexpr ALWAYS_INLINE bool IsDot(RomPathChar c) {
        return c == RomStringTraits::Dot;
    }

    class RomEntryName {
        private:
            const RomPathChar *m_path;
            size_t m_length;
        public:
            constexpr RomEntryName() : m_path(nullptr), m_length(0) {
                /* ... */
            }

            constexpr void Initialize(const RomPathChar *p, size_t len) {
                m_path   = p;
                m_length = len;
            }

            constexpr bool IsCurrentDirectory() const {
                return m_length == 1 && IsDot(m_path[0]);
            }

            constexpr bool IsParentDirectory() const {
                return m_length == 2 && IsDot(m_path[0]) && IsDot(m_path[1]);
            }

            constexpr bool IsRootDirectory() const {
                return m_length == 0;
            }

            constexpr const RomPathChar *begin() const {
                return m_path;
            }

            constexpr const RomPathChar *end() const {
                return m_path + m_length;
            }

            constexpr size_t length() const {
                return m_length;
            }
    };

    constexpr inline bool IsCurrentDirectory(const RomPathChar *p, size_t length) {
        AMS_ASSERT(p != nullptr);
        return length == 1 && IsDot(p[0]);
    }

    constexpr inline bool IsCurrentDirectory(const RomPathChar *p) {
        AMS_ASSERT(p != nullptr);
        return IsDot(p[0]) && IsNullTerminator(p[1]);
    }

    constexpr inline bool IsParentDirectory(const RomPathChar *p) {
        AMS_ASSERT(p != nullptr);
        return IsDot(p[0]) && IsDot(p[1]) && IsNullTerminator(p[2]);
    }

    constexpr inline bool IsParentDirectory(const RomPathChar *p, size_t length) {
        AMS_ASSERT(p != nullptr);
        return length == 2 && IsDot(p[0]) && IsDot(p[1]);
    }

    constexpr inline bool IsEqualPath(const RomPathChar *lhs, const RomPathChar *rhs, size_t length) {
        AMS_ASSERT(lhs != nullptr);
        AMS_ASSERT(rhs != nullptr);
        return std::strncmp(lhs, rhs, length) == 0;
    }

    Result GetParentDirectoryName(RomEntryName *out, const RomEntryName &cur, const RomPathChar *p);

    class PathParser {
        private:
            const RomPathChar *m_prev_path_start;
            const RomPathChar *m_prev_path_end;
            const RomPathChar *m_next_path;
            bool m_finished;
        public:
            constexpr PathParser() : m_prev_path_start(), m_prev_path_end(), m_next_path(), m_finished() { /* ... */ }

            Result Initialize(const RomPathChar *path);
            void Finalize();

            bool IsParseFinished() const;
            bool IsDirectoryPath() const;

            Result GetAsDirectoryName(RomEntryName *out) const;
            Result GetAsFileName(RomEntryName *out) const;

            Result GetNextDirectoryName(RomEntryName *out);
    };

}
