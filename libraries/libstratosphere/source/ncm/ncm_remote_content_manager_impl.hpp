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
#include <stratosphere.hpp>
#include "ncm_remote_content_storage_impl.hpp"
#include "ncm_remote_content_meta_database_impl.hpp"

namespace ams::ncm {

    class RemoteContentManagerImpl final : public IContentManager {
        public:
            RemoteContentManagerImpl() { /* ... */ }

            ~RemoteContentManagerImpl() { /* ... */ }
        public:
            virtual Result CreateContentStorage(StorageId storage_id) override {
                return ::ncmCreateContentStorage(static_cast<NcmStorageId>(storage_id));
            }

            virtual Result CreateContentMetaDatabase(StorageId storage_id) override {
                return ::ncmCreateContentMetaDatabase(static_cast<NcmStorageId>(storage_id));
            }

            virtual Result VerifyContentStorage(StorageId storage_id) override {
                return ::ncmVerifyContentStorage(static_cast<NcmStorageId>(storage_id));
            }

            virtual Result VerifyContentMetaDatabase(StorageId storage_id) override {
                return ::ncmVerifyContentMetaDatabase(static_cast<NcmStorageId>(storage_id));
            }

            virtual Result OpenContentStorage(sf::Out<std::shared_ptr<IContentStorage>> out, StorageId storage_id) override {
                NcmContentStorage cs;
                R_TRY(::ncmOpenContentStorage(std::addressof(cs), static_cast<NcmStorageId>(storage_id)));

                out.SetValue(std::make_shared<RemoteContentStorageImpl>(cs));
                return ResultSuccess();
            }

            virtual Result OpenContentMetaDatabase(sf::Out<std::shared_ptr<IContentMetaDatabase>> out, StorageId storage_id) override {
                NcmContentMetaDatabase db;
                R_TRY(::ncmOpenContentMetaDatabase(std::addressof(db), static_cast<NcmStorageId>(storage_id)));

                out.SetValue(std::make_shared<RemoteContentMetaDatabaseImpl>(db));
                return ResultSuccess();
            }

            virtual Result CloseContentStorageForcibly(StorageId storage_id) override {
                return ::ncmCloseContentStorageForcibly(static_cast<NcmStorageId>(storage_id));
            }

            virtual Result CloseContentMetaDatabaseForcibly(StorageId storage_id) override {
                return ::ncmCloseContentMetaDatabaseForcibly(static_cast<NcmStorageId>(storage_id));
            }

            virtual Result CleanupContentMetaDatabase(StorageId storage_id) override {
                return ::ncmCleanupContentMetaDatabase(static_cast<NcmStorageId>(storage_id));
            }

            virtual Result ActivateContentStorage(StorageId storage_id) override {
                return ::ncmActivateContentStorage(static_cast<NcmStorageId>(storage_id));
            }

            virtual Result InactivateContentStorage(StorageId storage_id) override {
                return ::ncmInactivateContentStorage(static_cast<NcmStorageId>(storage_id));
            }

            virtual Result ActivateContentMetaDatabase(StorageId storage_id) override {
                return ::ncmActivateContentMetaDatabase(static_cast<NcmStorageId>(storage_id));
            }

            virtual Result InactivateContentMetaDatabase(StorageId storage_id) override {
                return ::ncmInactivateContentMetaDatabase(static_cast<NcmStorageId>(storage_id));
            }

            virtual Result InvalidateRightsIdCache() override {
                return ::ncmInvalidateRightsIdCache();
            }

            virtual Result GetMemoryReport(sf::Out<MemoryReport> out) override {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }
    };

}
