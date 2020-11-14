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

    class RemoteContentManagerImpl final {
        public:
            RemoteContentManagerImpl() { /* ... */ }

            ~RemoteContentManagerImpl() { /* ... */ }
        public:
            Result CreateContentStorage(StorageId storage_id) {
                return ::ncmCreateContentStorage(static_cast<NcmStorageId>(storage_id));
            }

            Result CreateContentMetaDatabase(StorageId storage_id) {
                return ::ncmCreateContentMetaDatabase(static_cast<NcmStorageId>(storage_id));
            }

            Result VerifyContentStorage(StorageId storage_id) {
                return ::ncmVerifyContentStorage(static_cast<NcmStorageId>(storage_id));
            }

            Result VerifyContentMetaDatabase(StorageId storage_id) {
                return ::ncmVerifyContentMetaDatabase(static_cast<NcmStorageId>(storage_id));
            }

            Result OpenContentStorage(sf::Out<std::shared_ptr<IContentStorage>> out, StorageId storage_id) {
                NcmContentStorage cs;
                R_TRY(::ncmOpenContentStorage(std::addressof(cs), static_cast<NcmStorageId>(storage_id)));

                out.SetValue(sf::MakeShared<IContentStorage, RemoteContentStorageImpl>(cs));
                return ResultSuccess();
            }

            Result OpenContentMetaDatabase(sf::Out<std::shared_ptr<IContentMetaDatabase>> out, StorageId storage_id) {
                NcmContentMetaDatabase db;
                R_TRY(::ncmOpenContentMetaDatabase(std::addressof(db), static_cast<NcmStorageId>(storage_id)));

                out.SetValue(sf::MakeShared<IContentMetaDatabase, RemoteContentMetaDatabaseImpl>(db));
                return ResultSuccess();
            }

            Result CloseContentStorageForcibly(StorageId storage_id) {
                return ::ncmCloseContentStorageForcibly(static_cast<NcmStorageId>(storage_id));
            }

            Result CloseContentMetaDatabaseForcibly(StorageId storage_id) {
                return ::ncmCloseContentMetaDatabaseForcibly(static_cast<NcmStorageId>(storage_id));
            }

            Result CleanupContentMetaDatabase(StorageId storage_id) {
                return ::ncmCleanupContentMetaDatabase(static_cast<NcmStorageId>(storage_id));
            }

            Result ActivateContentStorage(StorageId storage_id) {
                return ::ncmActivateContentStorage(static_cast<NcmStorageId>(storage_id));
            }

            Result InactivateContentStorage(StorageId storage_id) {
                return ::ncmInactivateContentStorage(static_cast<NcmStorageId>(storage_id));
            }

            Result ActivateContentMetaDatabase(StorageId storage_id) {
                return ::ncmActivateContentMetaDatabase(static_cast<NcmStorageId>(storage_id));
            }

            Result InactivateContentMetaDatabase(StorageId storage_id) {
                return ::ncmInactivateContentMetaDatabase(static_cast<NcmStorageId>(storage_id));
            }

            Result InvalidateRightsIdCache() {
                return ::ncmInvalidateRightsIdCache();
            }

            Result GetMemoryReport(sf::Out<MemoryReport> out) {
                /* TODO: libnx bindings */
                AMS_ABORT();
            }
    };
    static_assert(ncm::IsIContentManager<RemoteContentManagerImpl>);

}
