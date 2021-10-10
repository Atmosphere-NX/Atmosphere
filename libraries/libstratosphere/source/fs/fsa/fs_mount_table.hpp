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
#include "fs_filesystem_accessor.hpp"

namespace ams::fs::impl {

    class MountTable : public Newable {
        NON_COPYABLE(MountTable);
        NON_MOVEABLE(MountTable);
        private:
            using FileSystemList = util::IntrusiveListBaseTraits<FileSystemAccessor>::ListType;
        private:
            FileSystemList m_fs_list;
            os::SdkMutex m_mutex;
        public:
            constexpr MountTable() : m_fs_list(), m_mutex() { /* ... */ }
        private:
            bool CanAcceptMountName(const char *name);
        public:
            Result Mount(std::unique_ptr<FileSystemAccessor> &&fs);
            Result Find(FileSystemAccessor **out, const char *name);
            void Unmount(const char *name);
    };

}
