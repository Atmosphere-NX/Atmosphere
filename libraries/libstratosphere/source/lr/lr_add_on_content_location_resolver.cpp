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

/* TODO: Properly integrate NCM api into libstratosphere to avoid linker hack. */
namespace ams::ncm::impl {

    Result OpenContentMetaDatabase(std::shared_ptr<ncm::IContentMetaDatabase> *, ncm::StorageId);
    Result OpenContentStorage(std::shared_ptr<ncm::IContentStorage> *, ncm::StorageId);

}

namespace ams::lr {

    Result AddOnContentLocationResolverInterface::ResolveAddOnContentPath(sf::Out<Path> out, ncm::ProgramId id) {
        /* Find a storage that contains the given program id. */
        ncm::StorageId storage_id = ncm::StorageId::None;
        R_UNLESS(this->registered_storages.Find(&storage_id, id), lr::ResultAddOnContentNotFound());

        /* Obtain a Content Meta Database for the storage id. */
        std::shared_ptr<ncm::IContentMetaDatabase> content_meta_database;
        R_TRY(ncm::impl::OpenContentMetaDatabase(&content_meta_database, storage_id));

        /* Find the latest data content id for the given program id. */
        ncm::ContentId data_content_id;
        R_TRY(content_meta_database->GetLatestData(&data_content_id, id));

        /* Obtain a Content Storage for the storage id. */
        std::shared_ptr<ncm::IContentStorage> content_storage;
        R_TRY(ncm::impl::OpenContentStorage(&content_storage, storage_id));

        /* Get the path of the data content. */
        R_ABORT_UNLESS(content_storage->GetPath(out.GetPointer(), data_content_id));

        return ResultSuccess();
    }

    Result AddOnContentLocationResolverInterface::RegisterAddOnContentStorageDeprecated(ncm::StorageId storage_id, ncm::ProgramId id) {
        /* Register storage for the given program id. 2.0.0-8.1.0 did not require an owner application id. */
        R_UNLESS(this->registered_storages.Register(id, storage_id, ncm::ProgramId::Invalid), lr::ResultTooManyRegisteredPaths());
        return ResultSuccess();
    }

    Result AddOnContentLocationResolverInterface::RegisterAddOnContentStorage(ncm::StorageId storage_id, ncm::ProgramId id, ncm::ProgramId application_id) {
        /* Register storage for the given program id and owner application. */
        R_UNLESS(this->registered_storages.Register(id, storage_id, application_id), lr::ResultTooManyRegisteredPaths());
        return ResultSuccess();
    }

    Result AddOnContentLocationResolverInterface::UnregisterAllAddOnContentPath() {
        this->registered_storages.Clear();
        return ResultSuccess();
    }

    Result AddOnContentLocationResolverInterface::RefreshApplicationAddOnContent(const sf::InArray<ncm::ProgramId> &ids) {
        if (ids.GetSize() == 0) {
            /* Clear all registered storages. */
            this->registered_storages.Clear();
        } else {
            /* Clear all registered storages excluding the provided program ids. */
            this->registered_storages.ClearExcluding(ids.GetPointer(), ids.GetSize());
        }

        return ResultSuccess();
    }

    Result AddOnContentLocationResolverInterface::UnregisterApplicationAddOnContent(ncm::ProgramId id) {
        /* Remove entries belonging to the provided application. */
        this->registered_storages.UnregisterOwnerProgram(id);
        return ResultSuccess();
    }

}
