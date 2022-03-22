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
#include "fsa/fs_mount_utils.hpp"

namespace ams::fs {

    Result MountImageDirectory(const char *name, ImageDirectoryId id) {
        /* Validate the mount name. */
        R_TRY(impl::CheckMountName(name));

        /* Open the image directory. This uses libnx bindings. */
        FsFileSystem fs;
        R_TRY(fsOpenImageDirectoryFileSystem(std::addressof(fs), static_cast<::FsImageDirectoryId>(id)));

        /* Allocate a new filesystem wrapper. */
        auto fsa = std::make_unique<RemoteFileSystem>(fs);
        R_UNLESS(fsa != nullptr, fs::ResultAllocationFailureInImageDirectoryA());

        /* Register. */
        return fsa::Register(name, std::move(fsa));
    }

}
