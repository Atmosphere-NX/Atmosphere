/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include <stratosphere/ncm/ncm_storage_id.hpp>
#include <stratosphere/ncm/ncm_content_meta_id.hpp>

namespace ams::ncm {

    class StorageList {
        public:
            static constexpr s32 MaxCount = 10;
        private:
            StorageId ids[MaxCount];
            s32 count;
        public:
            constexpr StorageList() : ids(), count() { /* ... */ }

            void Push(StorageId storage_id) {
                AMS_ABORT_UNLESS(this->count < MaxCount);

                for (s32 i = 0; i < MaxCount; i++) {
                    if (this->ids[i] == storage_id) {
                        return;
                    }
                }

                this->ids[this->count++] = storage_id;
            }

            s32 Count() const {
                return this->count;
            }

            StorageId operator[](s32 i) const {
                AMS_ABORT_UNLESS(i < this->count);
                return this->ids[i];
            }
    };

    constexpr StorageList GetStorageList(StorageId storage_id) {
        StorageList list;
        switch (storage_id) {
            case StorageId::BuiltInSystem:
            case StorageId::BuiltInUser:
            case StorageId::SdCard:
                list.Push(storage_id);
                break;
            case StorageId::Any:
                list.Push(StorageId::SdCard);
                list.Push(StorageId::BuiltInUser);
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        return list;
    }

    Result SelectDownloadableStorage(StorageId *out_storage_id, StorageId storage_id, s64 required_size);
    Result SelectPatchStorage(StorageId *out_storage_id, StorageId storage_id, PatchId patch_id);
    const char *GetStorageIdString(StorageId storage_id);
    const char *GetStorageIdStringForPlayReport(StorageId storage_id);

}
