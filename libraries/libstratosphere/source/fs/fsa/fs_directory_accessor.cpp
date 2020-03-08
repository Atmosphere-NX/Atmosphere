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
#include "fs_directory_accessor.hpp"
#include "fs_filesystem_accessor.hpp"

namespace ams::fs::impl {

    DirectoryAccessor::DirectoryAccessor(std::unique_ptr<fsa::IDirectory>&& d, FileSystemAccessor &p) : impl(std::move(d)), parent(p) {
        /* ... */
    }

    DirectoryAccessor::~DirectoryAccessor() {
        this->impl.reset();
        this->parent.NotifyCloseDirectory(this);
    }

    Result DirectoryAccessor::Read(s64 *out_count, DirectoryEntry *out_entries, s64 max_entries) {
        return this->impl->Read(out_count, out_entries, max_entries);
    }

    Result DirectoryAccessor::GetEntryCount(s64 *out) {
        return this->impl->GetEntryCount(out);
    }

}
