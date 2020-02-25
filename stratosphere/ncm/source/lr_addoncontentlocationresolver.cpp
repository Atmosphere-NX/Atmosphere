/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphere-NX
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

    Result AddOnContentLocationResolverInterface::ResolveAddOnContentPath(sf::Out<Path> out, ncm::ProgramId id) {
        ncm::StorageId storage_id = ncm::StorageId::None;
        R_UNLESS(this->registered_storages.Find(&storage_id, id), lr::ResultAddOnContentNotFound());

        std::shared_ptr<ncm::IContentMetaDatabase> content_meta_database;
        ncm::ContentId data_content_id;
        R_TRY(ncm::impl::OpenContentMetaDatabase(&content_meta_database, storage_id));
        R_TRY(content_meta_database->GetLatestData(&data_content_id, id));

        std::shared_ptr<ncm::IContentStorage> content_storage;
        R_TRY(ncm::impl::OpenContentStorage(&content_storage, storage_id));
        R_ABORT_UNLESS(content_storage->GetPath(out.GetPointer(), data_content_id));
        
        return ResultSuccess();
    }

    Result AddOnContentLocationResolverInterface::RegisterAddOnContentStorageDeprecated(ncm::StorageId storage_id, ncm::ProgramId id) {
        R_UNLESS(this->registered_storages.Register(id, storage_id, ncm::ProgramId::Invalid), lr::ResultTooManyRegisteredPaths());
        return ResultSuccess();
    }

    Result AddOnContentLocationResolverInterface::RegisterAddOnContentStorage(ncm::StorageId storage_id, ncm::ProgramId id, ncm::ProgramId application_id) {
        R_UNLESS(this->registered_storages.Register(id, storage_id, application_id), lr::ResultTooManyRegisteredPaths());
        return ResultSuccess();
    }

    Result AddOnContentLocationResolverInterface::UnregisterAllAddOnContentPath() {
        this->registered_storages.Clear();
        return ResultSuccess();
    }

    Result AddOnContentLocationResolverInterface::RefreshApplicationAddOnContent(const sf::InArray<ncm::ProgramId> &ids) {
        if (ids.GetSize() == 0) {
            this->registered_storages.Clear();
        } else {
            this->registered_storages.ClearExcluding(ids.GetPointer(), ids.GetSize());
        }

        return ResultSuccess();
    }

    Result AddOnContentLocationResolverInterface::UnregisterApplicationAddOnContent(ncm::ProgramId id) {
        this->registered_storages.UnregisterOwnerProgram(id);
        return ResultSuccess();
    }

}
