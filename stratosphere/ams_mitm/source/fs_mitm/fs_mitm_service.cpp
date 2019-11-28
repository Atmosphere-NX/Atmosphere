/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "fs_shim.h"
#include "fs_mitm_service.hpp"
#include "fsmitm_boot0storage.hpp"

namespace ams::mitm::fs {

    using namespace ams::fs;

    namespace {

        bool GetSettingsItemBooleanValue(const char *name, const char *key) {
            u8 tmp = 0;
            AMS_ASSERT(settings::fwdbg::GetSettingsItemValue(&tmp, sizeof(tmp), name, key) == sizeof(tmp));
            return (tmp != 0);
        }

    }

    Result FsMitmService::OpenBisStorage(sf::Out<std::shared_ptr<IStorageInterface>> out, u32 _bis_partition_id) {
        const ::FsBisPartitionId bis_partition_id = static_cast<::FsBisPartitionId>(_bis_partition_id);

        /* Try to open a storage for the partition. */
        FsStorage bis_storage;
        R_TRY(fsOpenBisStorageFwd(this->forward_service.get(), &bis_storage, bis_partition_id));
        const sf::cmif::DomainObjectId target_object_id{bis_storage.s.object_id};

        const bool is_sysmodule = ncm::IsSystemProgramId(this->client_info.program_id);
        const bool is_hbl = this->client_info.override_status.IsHbl();
        const bool can_write_bis = is_sysmodule || (is_hbl && GetSettingsItemBooleanValue("atmosphere", "enable_hbl_bis_write"));
        const bool can_read_cal  = is_sysmodule || (is_hbl && GetSettingsItemBooleanValue("atmosphere", "enable_hbl_cal_read"));

        /* Allow HBL to write to boot1 (safe firm) + package2. */
        /* This is needed to not break compatibility with ChoiDujourNX, which does not check for write access before beginning an update. */
        /* TODO: get fixed so that this can be turned off without causing bricks :/ */
        const bool is_package2 = (FsBisPartitionId_BootConfigAndPackage2Part1 <= bis_partition_id && bis_partition_id <= FsBisPartitionId_BootConfigAndPackage2Part6);
        const bool is_boot1    = bis_partition_id == FsBisPartitionId_BootPartition2Root;
        const bool can_write_bis_for_choi_support = is_hbl && (is_package2 || is_boot1);

        /* Set output storage. */
        if (bis_partition_id == FsBisPartitionId_BootPartition1Root) {
            out.SetValue(std::make_shared<IStorageInterface>(new Boot0Storage(bis_storage, this->client_info)), target_object_id);
        } else if (bis_partition_id == FsBisPartitionId_CalibrationBinary) {
            /* PRODINFO should *never* be writable. */
            /* If we have permissions, create a read only storage. */
            if (can_read_cal) {
                out.SetValue(std::make_shared<IStorageInterface>(new ReadOnlyStorageAdapter(new RemoteStorage(bis_storage))), target_object_id);
            } else {
                /* If we can't read cal, return permission denied. */
                fsStorageClose(&bis_storage);
                return fs::ResultPermissionDenied();
            }
        } else {
            if (can_write_bis || can_write_bis_for_choi_support) {
                /* We can write, so create a writable storage. */
                out.SetValue(std::make_shared<IStorageInterface>(new RemoteStorage(bis_storage)), target_object_id);
            } else {
                /* We can only read, so create a readable storage. */
                out.SetValue(std::make_shared<IStorageInterface>(new ReadOnlyStorageAdapter(new RemoteStorage(bis_storage))), target_object_id);
            }
        }

        return ResultSuccess();
    }

}
