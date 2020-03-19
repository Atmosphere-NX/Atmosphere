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

        constexpr inline SaveDataSpaceId DeviceSaveDataSpaceId = SaveDataSpaceId::User;

        Result MountDeviceSaveDataImpl(const char *name, const SaveDataAttribute &attribute) {
            /* Validate the mount name. */
            R_TRY(impl::CheckMountName(name));

            /* Open the filesystem, use libnx bindings. */
            ::FsFileSystem fs;
            R_TRY(fsOpenSaveDataFileSystem(std::addressof(fs), static_cast<::FsSaveDataSpaceId>(DeviceSaveDataSpaceId), reinterpret_cast<const ::FsSaveDataAttribute *>(std::addressof(attribute))));

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<RemoteFileSystem>(fs);
            R_UNLESS(fsa != nullptr, fs::ResultAllocationFailureInDeviceSaveDataA());

            /* Register. */
            return fsa::Register(name, std::move(fsa));
        }

    }

    Result MountDeviceSaveData(const char *name) {
        return MountDeviceSaveDataImpl(name, SaveDataAttribute::Make(ncm::InvalidProgramId, SaveDataType::Device, InvalidUserId, InvalidSystemSaveDataId));
    }

    Result MountDeviceSaveData(const char *name, const ncm::ApplicationId application_id) {
        return MountDeviceSaveDataImpl(name, SaveDataAttribute::Make(application_id, SaveDataType::Device, InvalidUserId, InvalidSystemSaveDataId));
    }

}
