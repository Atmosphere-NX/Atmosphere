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
#include "fsa/fs_filesystem_accessor.hpp"
#include "fsa/fs_mount_utils.hpp"
#include "impl/fs_file_system_proxy_service_object.hpp"

namespace ams::fs {

    namespace {

        Result GetSignedSystemPartitionOnSdCardValid(char *out, impl::FileSystemAccessor *accessor) {
            R_TRY_CATCH(accessor->QueryEntry(out, sizeof(*out), nullptr, 0, fsa::QueryId::IsSignedSystemPartitionOnSdCardValid, "/")) {
                /* If querying isn't supported, then the partition isn't valid. */
                R_CATCH(fs::ResultUnsupportedOperation) { *out = false; }
            } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
            R_SUCCEED();
        }

    }

    bool IsSignedSystemPartitionOnSdCardValid(const char *system_root_path) {
        /* Get the accessor for the system filesystem. */
        impl::FileSystemAccessor *accessor;
        const char *sub_path;
        AMS_FS_R_ABORT_UNLESS(impl::FindFileSystem(std::addressof(accessor), std::addressof(sub_path), system_root_path));

        char is_valid;
        AMS_FS_R_ABORT_UNLESS(GetSignedSystemPartitionOnSdCardValid(std::addressof(is_valid), accessor));
        return is_valid;
    }

    bool IsSignedSystemPartitionOnSdCardValidDeprecated() {
        /* Ensure we only call with correct version. */
        auto version = hos::GetVersion();
        AMS_ABORT_UNLESS(hos::Version_4_0_0 <= version && version < hos::Version_8_0_0);

        /* Check that the partition is valid. */
        auto fsp = impl::GetFileSystemProxyServiceObject();
        bool is_valid;
        AMS_FS_R_ABORT_UNLESS(fsp->IsSignedSystemPartitionOnSdCardValid(std::addressof(is_valid)));
        return is_valid;
    }

}
