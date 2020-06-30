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
#include <stratosphere.hpp>

namespace ams::fs::impl {

    class FileSystemAccessor;

    class DirectoryAccessor : public util::IntrusiveListBaseNode<DirectoryAccessor>, public Newable {
        NON_COPYABLE(DirectoryAccessor);
        private:
            std::unique_ptr<fsa::IDirectory> impl;
            FileSystemAccessor &parent;
        public:
            DirectoryAccessor(std::unique_ptr<fsa::IDirectory>&& d, FileSystemAccessor &p);
            ~DirectoryAccessor();

            Result Read(s64 *out_count, DirectoryEntry *out_entries, s64 max_entries);
            Result GetEntryCount(s64 *out);

            FileSystemAccessor *GetParent() const { return std::addressof(this->parent); }
    };

}
