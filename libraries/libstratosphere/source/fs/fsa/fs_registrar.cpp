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
#include "fs_filesystem_accessor.hpp"
#include "fs_user_mount_table.hpp"

namespace ams::fs::fsa {

    Result Register(const char *name, std::unique_ptr<IFileSystem> &&fs) {
        auto accessor = std::make_unique<impl::FileSystemAccessor>(name, std::move(fs));
        R_UNLESS(accessor != nullptr, fs::ResultAllocationFailureInRegisterA());

        return impl::Register(std::move(accessor));
    }

    Result Register(const char *name, std::unique_ptr<IFileSystem> &&fs, std::unique_ptr<ICommonMountNameGenerator> &&generator) {
        auto accessor = std::make_unique<impl::FileSystemAccessor>(name, std::move(fs), std::move(generator));
        R_UNLESS(accessor != nullptr, fs::ResultAllocationFailureInRegisterB());

        return impl::Register(std::move(accessor));
    }

    Result Register(const char *name, std::unique_ptr<IFileSystem> &&fs, std::unique_ptr<ICommonMountNameGenerator> &&generator, bool use_data_cache, bool use_path_cache, bool support_multi_commit) {
        auto accessor = std::make_unique<impl::FileSystemAccessor>(name, std::move(fs), std::move(generator));
        R_UNLESS(accessor != nullptr, fs::ResultAllocationFailureInRegisterB());

        accessor->SetFileDataCacheAttachable(use_data_cache);
        accessor->SetPathBasedFileDataCacheAttachable(use_path_cache);
        accessor->SetMultiCommitSupported(support_multi_commit);

        return impl::Register(std::move(accessor));
    }

    void Unregister(const char *name) {
        impl::Unregister(name);
    }

}
