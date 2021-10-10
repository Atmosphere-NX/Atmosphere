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
#include <stratosphere.hpp>
#include "lr_add_on_content_location_resolver_impl.hpp"

namespace ams::lr {

    Result AddOnContentLocationResolverImpl::ResolveAddOnContentPath(sf::Out<Path> out, ncm::DataId id) {
        /* Find a storage that contains the given program id. */
        ncm::StorageId storage_id = ncm::StorageId::None;
        R_UNLESS(m_registered_storages.Find(std::addressof(storage_id), id), lr::ResultAddOnContentNotFound());

        /* Obtain a content meta database for the storage id. */
        ncm::ContentMetaDatabase content_meta_database;
        R_TRY(ncm::OpenContentMetaDatabase(std::addressof(content_meta_database), storage_id));

        /* Find the latest data content id for the given program id. */
        ncm::ContentId data_content_id;
        R_TRY(content_meta_database.GetLatestData(std::addressof(data_content_id), id));

        /* Obtain a content storage for the storage id. */
        ncm::ContentStorage content_storage;
        R_TRY(ncm::OpenContentStorage(std::addressof(content_storage), storage_id));

        /* Get the path of the data content. */
        static_assert(sizeof(lr::Path) == sizeof(ncm::Path));
        content_storage.GetPath(reinterpret_cast<ncm::Path *>(out.GetPointer()), data_content_id);

        return ResultSuccess();
    }

    Result AddOnContentLocationResolverImpl::RegisterAddOnContentStorageDeprecated(ncm::DataId id, ncm::StorageId storage_id) {
        /* Register storage for the given program id. 2.0.0-8.1.0 did not require an owner application id. */
        R_UNLESS(m_registered_storages.Register(id, storage_id, ncm::InvalidApplicationId), lr::ResultTooManyRegisteredPaths());
        return ResultSuccess();
    }

    Result AddOnContentLocationResolverImpl::RegisterAddOnContentStorage(ncm::DataId id, ncm::ApplicationId application_id, ncm::StorageId storage_id) {
        /* Register storage for the given program id and owner application. */
        R_UNLESS(m_registered_storages.Register(id, storage_id, application_id), lr::ResultTooManyRegisteredPaths());
        return ResultSuccess();
    }

    Result AddOnContentLocationResolverImpl::UnregisterAllAddOnContentPath() {
        m_registered_storages.Clear();
        return ResultSuccess();
    }

    Result AddOnContentLocationResolverImpl::RefreshApplicationAddOnContent(const sf::InArray<ncm::ApplicationId> &ids) {
        if (ids.GetSize() == 0) {
            /* Clear all registered storages. */
            m_registered_storages.Clear();
        } else {
            /* Clear all registered storages excluding the provided program ids. */
            m_registered_storages.ClearExcluding(reinterpret_cast<const ncm::ProgramId *>(ids.GetPointer()), ids.GetSize());
        }

        return ResultSuccess();
    }

    Result AddOnContentLocationResolverImpl::UnregisterApplicationAddOnContent(ncm::ApplicationId id) {
        /* Remove entries belonging to the provided application. */
        m_registered_storages.UnregisterOwnerProgram(id);
        return ResultSuccess();
    }

}
