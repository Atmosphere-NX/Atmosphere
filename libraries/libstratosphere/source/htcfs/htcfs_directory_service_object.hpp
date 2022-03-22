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
#include <stratosphere.hpp>

namespace ams::htcfs {

    class DirectoryServiceObject {
        private:
            s32 m_handle;
        public:
            explicit DirectoryServiceObject(s32 handle);
            ~DirectoryServiceObject();
        public:
            Result GetEntryCount(ams::sf::Out<s64> out);
            Result Read(ams::sf::Out<s64> out, const ams::sf::OutMapAliasArray<fs::DirectoryEntry> &out_entries);
            Result SetPriorityForDirectory(s32 priority);
            Result GetPriorityForDirectory(ams::sf::Out<s32> out);
    };
    static_assert(tma::IsIDirectoryAccessor<DirectoryServiceObject>);

}
