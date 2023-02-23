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
#pragma once
#include <stratosphere.hpp>
#include "ncm_remote_content_storage_impl.hpp"
#include "ncm_remote_content_meta_database_impl.hpp"

namespace ams::ncm {

    #if defined(ATMOSPHERE_OS_HORIZON)
    class RemoteContentManagerImpl {
        private:
            /* TODO: sf::ProxyObjectAllocator */
            using ObjectFactory = sf::ObjectFactory<sf::StdAllocationPolicy<std::allocator>>;
        public:
            RemoteContentManagerImpl() { R_ABORT_UNLESS(::ncmInitialize()); }

            ~RemoteContentManagerImpl() { ::ncmExit(); }
        public:
            Result CreateContentStorage(StorageId storage_id) {
                R_RETURN(::ncmCreateContentStorage(static_cast<NcmStorageId>(storage_id)));
            }

            Result CreateContentMetaDatabase(StorageId storage_id) {
                R_RETURN(::ncmCreateContentMetaDatabase(static_cast<NcmStorageId>(storage_id)));
            }

            Result VerifyContentStorage(StorageId storage_id) {
                R_RETURN(::ncmVerifyContentStorage(static_cast<NcmStorageId>(storage_id)));
            }

            Result VerifyContentMetaDatabase(StorageId storage_id) {
                R_RETURN(::ncmVerifyContentMetaDatabase(static_cast<NcmStorageId>(storage_id)));
            }

            Result OpenContentStorage(sf::Out<sf::SharedPointer<IContentStorage>> out, StorageId storage_id) {
                NcmContentStorage cs;
                R_TRY(::ncmOpenContentStorage(std::addressof(cs), static_cast<NcmStorageId>(storage_id)));

                out.SetValue(ObjectFactory::CreateSharedEmplaced<IContentStorage, RemoteContentStorageImpl>(cs));
                R_SUCCEED();
            }

            Result OpenContentMetaDatabase(sf::Out<sf::SharedPointer<IContentMetaDatabase>> out, StorageId storage_id) {
                NcmContentMetaDatabase db;
                R_TRY(::ncmOpenContentMetaDatabase(std::addressof(db), static_cast<NcmStorageId>(storage_id)));

                out.SetValue(ObjectFactory::CreateSharedEmplaced<IContentMetaDatabase, RemoteContentMetaDatabaseImpl>(db));
                R_SUCCEED();
            }

            Result CloseContentStorageForcibly(StorageId storage_id) {
                R_RETURN(::ncmCloseContentStorageForcibly(static_cast<NcmStorageId>(storage_id)));
            }

            Result CloseContentMetaDatabaseForcibly(StorageId storage_id) {
                R_RETURN(::ncmCloseContentMetaDatabaseForcibly(static_cast<NcmStorageId>(storage_id)));
            }

            Result CleanupContentMetaDatabase(StorageId storage_id) {
                R_RETURN(::ncmCleanupContentMetaDatabase(static_cast<NcmStorageId>(storage_id)));
            }

            Result ActivateContentStorage(StorageId storage_id) {
                R_RETURN(::ncmActivateContentStorage(static_cast<NcmStorageId>(storage_id)));
            }

            Result InactivateContentStorage(StorageId storage_id) {
                R_RETURN(::ncmInactivateContentStorage(static_cast<NcmStorageId>(storage_id)));
            }

            Result ActivateContentMetaDatabase(StorageId storage_id) {
                R_RETURN(::ncmActivateContentMetaDatabase(static_cast<NcmStorageId>(storage_id)));
            }

            Result InactivateContentMetaDatabase(StorageId storage_id) {
                R_RETURN(::ncmInactivateContentMetaDatabase(static_cast<NcmStorageId>(storage_id)));
            }

            Result InvalidateRightsIdCache() {
                R_RETURN(::ncmInvalidateRightsIdCache());
            }

            Result GetMemoryReport(sf::Out<MemoryReport> out) {
                /* TODO: libnx bindings */
                AMS_UNUSED(out);
                AMS_ABORT();
            }

            Result ActivateFsContentStorage(fs::ContentStorageId fs_content_storage_id) {
                R_RETURN(::ncmActivateFsContentStorage(static_cast<::FsContentStorageId>(util::ToUnderlying(fs_content_storage_id))));
            }
    };
    static_assert(ncm::IsIContentManager<RemoteContentManagerImpl>);
    #endif

}
