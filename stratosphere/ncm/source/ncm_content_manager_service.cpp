/*
 * Copyright (c) 2019 Adubbz
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

#include "impl/ncm_content_manager.hpp"
#include "ncm_content_manager_service.hpp"

#include "debug.hpp"

namespace sts::ncm {

    Result ContentManagerService::CreateContentStorage(StorageId storage_id) {
        R_DEBUG_START
        return impl::CreateContentStorage(storage_id);
        R_DEBUG_END
    }

    Result ContentManagerService::CreateContentMetaDatabase(StorageId storage_id) {
        R_DEBUG_START
        return impl::CreateContentMetaDatabase(storage_id);
        R_DEBUG_END
    }

    Result ContentManagerService::VerifyContentStorage(StorageId storage_id) {
        R_DEBUG_START
        return impl::VerifyContentStorage(storage_id);
        R_DEBUG_END
    }

    Result ContentManagerService::VerifyContentMetaDatabase(StorageId storage_id) {
        R_DEBUG_START
        return impl::VerifyContentMetaDatabase(storage_id);
        R_DEBUG_END
    }

    Result ContentManagerService::OpenContentStorage(Out<std::shared_ptr<IContentStorage>> out, StorageId storage_id) {
        R_DEBUG_START
        D_LOG("storage id: 0x%x\n", storage_id);
        std::shared_ptr<IContentStorage> content_storage;
        R_TRY(impl::OpenContentStorage(&content_storage, storage_id));
        out.SetValue(std::move(content_storage));
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentManagerService::OpenContentMetaDatabase(Out<std::shared_ptr<IContentMetaDatabase>> out, StorageId storage_id) {
        R_DEBUG_START
        D_LOG("storage id: 0x%x\n", storage_id);
        std::shared_ptr<IContentMetaDatabase> content_meta_database;
        R_TRY(impl::OpenContentMetaDatabase(&content_meta_database, storage_id));
        out.SetValue(std::move(content_meta_database));
        return ResultSuccess;
        R_DEBUG_END
    }

    Result ContentManagerService::CloseContentStorageForcibly(StorageId storage_id) {
        R_DEBUG_START
        return impl::CloseContentStorageForcibly(storage_id);
        R_DEBUG_END
    }

    Result ContentManagerService::CloseContentMetaDatabaseForcibly(StorageId storage_id) {
        R_DEBUG_START
        return impl::CloseContentMetaDatabaseForcibly(storage_id);
        R_DEBUG_END
    }

    Result ContentManagerService::CleanupContentMetaDatabase(StorageId storage_id) {
        R_DEBUG_START
        return impl::CleanupContentMetaDatabase(storage_id);
        R_DEBUG_END
    }

    Result ContentManagerService::ActivateContentStorage(StorageId storage_id) {
        R_DEBUG_START
        return impl::ActivateContentStorage(storage_id);
        R_DEBUG_END
    }

    Result ContentManagerService::InactivateContentStorage(StorageId storage_id) {
        R_DEBUG_START
        return impl::InactivateContentStorage(storage_id);
        R_DEBUG_END
    }

    Result ContentManagerService::ActivateContentMetaDatabase(StorageId storage_id) {
        R_DEBUG_START
        return impl::ActivateContentMetaDatabase(storage_id);
        R_DEBUG_END
    }

    Result ContentManagerService::InactivateContentMetaDatabase(StorageId storage_id) {
        R_DEBUG_START
        return impl::InactivateContentMetaDatabase(storage_id);
        R_DEBUG_END
    }

}