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

namespace ams::lr {

    Result AddOnContentLocationResolverInterface::ResolveAddOnContentPath(sf::Out<Path> out, ncm::ProgramId tid) {
        ncm::StorageId storage_id = ncm::StorageId::None;

        if (!this->registered_storages.Find(&storage_id, tid)) {
            return ResultAddOnContentNotFound();
        }

        std::shared_ptr<ncm::IContentMetaDatabase> content_meta_database;
    
        ncm::ContentId data_content_id;
        R_TRY(ncm::impl::OpenContentMetaDatabase(&content_meta_database, storage_id));
        R_TRY(content_meta_database->GetLatestData(&data_content_id, tid));

        std::shared_ptr<ncm::IContentStorage> content_storage;
        R_TRY(ncm::impl::OpenContentStorage(&content_storage, storage_id));
        R_ASSERT(content_storage->GetPath(out.GetPointer(), data_content_id));
        
        return ResultSuccess();
    }

    Result AddOnContentLocationResolverInterface::RegisterAddOnContentStorageDeprecated(ncm::StorageId storage_id, ncm::ProgramId tid) {
        if (!this->registered_storages.Register(tid, storage_id, ncm::ProgramId::Invalid)) {
            return ResultTooManyRegisteredPaths();
        }

        return ResultSuccess();
    }

    Result AddOnContentLocationResolverInterface::RegisterAddOnContentStorage(ncm::StorageId storage_id, ncm::ProgramId tid, ncm::ProgramId application_tid) {
        if (!this->registered_storages.Register(tid, storage_id, application_tid)) {
            return ResultTooManyRegisteredPaths();
        }

        return ResultSuccess();
    }

    Result AddOnContentLocationResolverInterface::UnregisterAllAddOnContentPath() {
        this->registered_storages.Clear();
        return ResultSuccess();
    }

    Result AddOnContentLocationResolverInterface::RefreshApplicationAddOnContent(const sf::InArray<ncm::ProgramId> &tids) {
        if (tids.GetSize() == 0) {
            this->registered_storages.Clear();
            return ResultSuccess();
        }

        this->registered_storages.ClearExcluding(tids.GetPointer(), tids.GetSize());
        return ResultSuccess();
    }

    Result AddOnContentLocationResolverInterface::UnregisterApplicationAddOnContent(ncm::ProgramId tid) {
        this->registered_storages.UnregisterOwnerTitle(tid);
        return ResultSuccess();
    }

}
