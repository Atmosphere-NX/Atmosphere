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
#include <stratosphere.hpp>
#include "fs_user_mount_table.hpp"
#include "fs_mount_table.hpp"
#include "fs_filesystem_accessor.hpp"

namespace ams::fs::impl {

    namespace {

        constinit MountTable g_mount_table;

    }

    Result Register(std::unique_ptr<FileSystemAccessor> &&fs) {
        return g_mount_table.Mount(std::move(fs));
    }

    Result Find(FileSystemAccessor **out, const char *name) {
        return g_mount_table.Find(out, name);
    }

    void Unregister(const char *name) {
        g_mount_table.Unmount(name);
    }

}
