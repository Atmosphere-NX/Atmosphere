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
#include "lr_addoncontentlocationresolver.hpp"

namespace sts::lr {

    Result AddOnContentLocationResolverInterface::ResolveAddOnContentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        Path path;
        ncm::StorageId storage_id = ncm::StorageId::None;

        if (!this->registered_storages.Find(&storage_id, tid)) {
            return ResultLrAddOnContentNotFound;
        }

        std::shared_ptr<ncm::IContentMetaDatabase> content_meta_database;
    
        ncm::ContentId data_content_id;
        R_TRY(ncm::impl::OpenContentMetaDatabase(&content_meta_database, storage_id));
        R_TRY(content_meta_database->GetLatestData(&data_content_id, tid));

        std::shared_ptr<ncm::IContentStorage> content_storage;
        R_TRY(ncm::impl::OpenContentStorage(&content_storage, storage_id));
        R_ASSERT(content_storage->GetPath(&path, data_content_id));
        *out.pointer = path;
        
        return ResultSuccess;
    }

    Result AddOnContentLocationResolverInterface::RegisterAddOnContentStorage(ncm::StorageId storage_id, ncm::TitleId tid) {
        if (!this->registered_storages.Register(tid, storage_id)) {
            return ResultLrTooManyRegisteredPaths;
        }

        return ResultSuccess;
    }

    Result AddOnContentLocationResolverInterface::UnregisterAllAddOnContentPath() {
        this->registered_storages.Clear();
        return ResultSuccess;
    }

}
