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
#include <stratosphere.hpp>
#include "ncm_remote_content_manager_impl.hpp"

namespace ams::ncm {

    namespace {

        std::shared_ptr<IContentManager> g_content_manager;

    }

    void Initialize() {
        AMS_ASSERT(g_content_manager == nullptr);
        R_ABORT_UNLESS(ncmInitialize());
        g_content_manager = sf::MakeShared<IContentManager, RemoteContentManagerImpl>();
    }

    void Finalize() {
        AMS_ASSERT(g_content_manager != nullptr);
        g_content_manager.reset();
        ncmExit();
    }

    void InitializeWithObject(std::shared_ptr<IContentManager> manager_object) {
        AMS_ASSERT(g_content_manager == nullptr);
        g_content_manager = manager_object;
        AMS_ASSERT(g_content_manager != nullptr);
    }

    /* Service API. */
    Result CreateContentStorage(StorageId storage_id) {
        return g_content_manager->CreateContentStorage(storage_id);
    }

    Result CreateContentMetaDatabase(StorageId storage_id) {
        return g_content_manager->CreateContentMetaDatabase(storage_id);
    }

    Result VerifyContentStorage(StorageId storage_id) {
        return g_content_manager->VerifyContentStorage(storage_id);
    }

    Result VerifyContentMetaDatabase(StorageId storage_id) {
        return g_content_manager->VerifyContentMetaDatabase(storage_id);
    }

    Result OpenContentStorage(ContentStorage *out, StorageId storage_id) {
        sf::cmif::ServiceObjectHolder object_holder;
        R_TRY(g_content_manager->OpenContentStorage(std::addressof(object_holder), storage_id));

        *out = ContentStorage(object_holder.GetServiceObject<IContentStorage>());
        return ResultSuccess();
    }

    Result OpenContentMetaDatabase(ContentMetaDatabase *out, StorageId storage_id) {
        sf::cmif::ServiceObjectHolder object_holder;
        R_TRY(g_content_manager->OpenContentMetaDatabase(std::addressof(object_holder), storage_id));

        *out = ContentMetaDatabase(object_holder.GetServiceObject<IContentMetaDatabase>());
        return ResultSuccess();
    }

    Result CleanupContentMetaDatabase(StorageId storage_id) {
        return g_content_manager->CleanupContentMetaDatabase(storage_id);
    }

    Result ActivateContentStorage(StorageId storage_id) {
        return g_content_manager->ActivateContentStorage(storage_id);
    }

    Result InactivateContentStorage(StorageId storage_id) {
        return g_content_manager->InactivateContentStorage(storage_id);
    }

    Result ActivateContentMetaDatabase(StorageId storage_id) {
        /* On < 2.0.0, this command doesn't exist, and databases are activated as needed on open. */
        R_SUCCEED_IF(hos::GetVersion() < hos::Version_2_0_0);
        return g_content_manager->ActivateContentMetaDatabase(storage_id);
    }

    Result InactivateContentMetaDatabase(StorageId storage_id) {
        /* On < 2.0.0, this command doesn't exist. */
        R_SUCCEED_IF(hos::GetVersion() < hos::Version_2_0_0);
        return g_content_manager->InactivateContentMetaDatabase(storage_id);
    }

    Result InvalidateRightsIdCache() {
        return g_content_manager->InvalidateRightsIdCache();
    }

    /* Deprecated API. */
    Result CloseContentStorageForcibly(StorageId storage_id) {
        AMS_ABORT_UNLESS(hos::GetVersion() == hos::Version_1_0_0);
        return g_content_manager->CloseContentStorageForcibly(storage_id);
    }

    Result CloseContentMetaDatabaseForcibly(StorageId storage_id) {
        AMS_ABORT_UNLESS(hos::GetVersion() == hos::Version_1_0_0);
        return g_content_manager->CloseContentMetaDatabaseForcibly(storage_id);
    }
}
