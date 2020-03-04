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

namespace ams::fs {

    Result MountSdCard(const char *name) {
        /* Open the SD card. This uses libnx bindings. */
        FsFileSystem fs;
        R_TRY(fsOpenSdCardFileSystem(std::addressof(fs)));

        /* Allocate a new filesystem wrapper. */
        std::unique_ptr<fsa::IFileSystem> fsa(new RemoteFileSystem(fs));
        R_UNLESS(fsa != nullptr, fs::ResultAllocationFailureInSdCardA());

        /* Register. */
        return fsa::Register(name, std::move(fsa));
    }

}
