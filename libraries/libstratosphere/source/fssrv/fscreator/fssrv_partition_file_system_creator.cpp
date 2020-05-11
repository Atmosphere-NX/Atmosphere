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

namespace ams::fssrv::fscreator {

    Result PartitionFileSystemCreator::Create(std::shared_ptr<fs::fsa::IFileSystem> *out, std::shared_ptr<fs::IStorage> storage) {
        /* Allocate a filesystem. */
        std::shared_ptr fs = fssystem::AllocateShared<fssystem::PartitionFileSystem>();
        R_UNLESS(fs != nullptr, fs::ResultAllocationFailureInPartitionFileSystemCreatorA());

        /* Initialize the filesystem. */
        R_TRY(fs->Initialize(std::move(storage)));

        /* Set the output. */
        *out = std::move(fs);
        return ResultSuccess();
    }

}
