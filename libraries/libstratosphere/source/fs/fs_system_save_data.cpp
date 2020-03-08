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
#include "fsa/fs_mount_utils.hpp"

namespace ams::fs {

    namespace {

        Result MountSystemSaveDataImpl(const char *name, SaveDataSpaceId space_id, SystemSaveDataId id, UserId user_id, SaveDataType type) {
            /* Validate the mount name. */
            R_TRY(impl::CheckMountName(name));

            /* Create the attribute. */
            const auto attribute = SaveDataAttribute::Make(ncm::InvalidProgramId, type, user_id, id);
            static_assert(sizeof(attribute) == sizeof(::FsSaveDataAttribute));

            /* Open the filesystem, use libnx bindings. */
            ::FsFileSystem fs;
            R_TRY(fsOpenSaveDataFileSystemBySystemSaveDataId(std::addressof(fs), static_cast<::FsSaveDataSpaceId>(space_id), reinterpret_cast<const ::FsSaveDataAttribute *>(std::addressof(attribute))));

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<RemoteFileSystem>(fs);
            R_UNLESS(fsa != nullptr, fs::ResultAllocationFailureInSystemSaveDataA());

            /* Register. */
            return fsa::Register(name, std::move(fsa));
        }

    }

    Result MountSystemSaveData(const char *name, SystemSaveDataId id) {
        return MountSystemSaveData(name, id, InvalidUserId);
    }

    Result MountSystemSaveData(const char *name, SaveDataSpaceId space_id, SystemSaveDataId id) {
        return MountSystemSaveData(name, space_id, id, InvalidUserId);
    }

    Result MountSystemSaveData(const char *name, SystemSaveDataId id, UserId user_id) {
        return MountSystemSaveData(name, SaveDataSpaceId::System, id, user_id);
    }

    Result MountSystemSaveData(const char *name, SaveDataSpaceId space_id, SystemSaveDataId id, UserId user_id) {
        return MountSystemSaveDataImpl(name, space_id, id, user_id, SaveDataType::System);
    }

}
