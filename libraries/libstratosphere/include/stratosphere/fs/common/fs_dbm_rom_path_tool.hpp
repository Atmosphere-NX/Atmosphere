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
#include <stratosphere/fs/common/fs_dbm_rom_types.hpp>

namespace ams::fs::RomPathTool {

    constexpr inline u32 MaxPathLength = 0x300;

    struct RomEntryName {
        size_t length;
        const RomPathChar *path;
    };
    static_assert(util::is_pod<RomEntryName>::value);

    constexpr void InitEntryName(RomEntryName *entry) {
        AMS_ASSERT(entry != nullptr);
        entry->length = 0;
    }

    constexpr inline bool IsSeparator(RomPathChar c) {
        return c == RomStringTraits::DirectorySeparator;
    }

    constexpr inline bool IsNullTerminator(RomPathChar c) {
        return c == RomStringTraits::NullTerminator;
    }

    constexpr inline bool IsDot(RomPathChar c) {
        return c == RomStringTraits::Dot;
    }

    constexpr inline bool IsCurrentDirectory(const RomEntryName &name) {
        return name.length == 1 && IsDot(name.path[0]);
    }

    constexpr inline bool IsCurrentDirectory(const RomPathChar *p, size_t length) {
        AMS_ASSERT(p != nullptr);
        return length == 1 && IsDot(p[0]);
    }

    constexpr inline bool IsCurrentDirectory(const RomPathChar *p) {
        AMS_ASSERT(p != nullptr);
        return IsDot(p[0]) && IsNullTerminator(p[1]);
    }

    constexpr inline bool IsParentDirectory(const RomEntryName &name) {
        return name.length == 2 && IsDot(name.path[0]) && IsDot(name.path[1]);
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
            const RomPathChar *prev_path_start;
            const RomPathChar *prev_path_end;
            const RomPathChar *next_path;
            bool finished;
        public:
            constexpr PathParser() : prev_path_start(), prev_path_end(), next_path(), finished() { /* ... */ }

            Result Initialize(const RomPathChar *path);
            void Finalize();

            bool IsParseFinished() const;
            bool IsDirectoryPath() const;

            Result GetAsDirectoryName(RomEntryName *out) const;
            Result GetAsFileName(RomEntryName *out) const;

            Result GetNextDirectoryName(RomEntryName *out);
    };

}
