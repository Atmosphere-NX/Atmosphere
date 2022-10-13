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

    namespace {

        constexpr const lr::Path EmptyPath = {};

        template<size_t N>
        Result ResolveAddOnContentPathImpl(Path *out, RedirectionAttributes *out_attr, const RegisteredStorages<ncm::DataId, N> &storages, ncm::DataId id) {
            /* Find a storage that contains the given program id. */
            ncm::StorageId storage_id = ncm::StorageId::None;
            R_UNLESS(storages.Find(std::addressof(storage_id), id), lr::ResultAddOnContentNotFound());

            /* Obtain a content meta database for the storage id. */
            ncm::ContentMetaDatabase content_meta_database;
            R_TRY(ncm::OpenContentMetaDatabase(std::addressof(content_meta_database), storage_id));

            /* Find the latest data content info for the given program id. */
            ncm::ContentInfo data_content_info;
            R_TRY(content_meta_database.GetLatestData(std::addressof(data_content_info), id));

            /* Obtain a content storage for the storage id. */
            ncm::ContentStorage content_storage;
            R_TRY(ncm::OpenContentStorage(std::addressof(content_storage), storage_id));

            /* Get the path of the data content. */
            static_assert(sizeof(lr::Path) == sizeof(ncm::Path));
            content_storage.GetPath(reinterpret_cast<ncm::Path *>(out), data_content_info.GetId());

            /* Get the redirection attributes. */
            *out_attr = RedirectionAttributes::Make(data_content_info.GetContentAttributes());

            R_SUCCEED();
        }

    }

    Result AddOnContentLocationResolverImpl::ResolveAddOnContentPath(Path *out, RedirectionAttributes *out_attr, ncm::DataId id) {
        /* Try to resolve using our registered storages. */
        if (const auto result = ResolveAddOnContentPathImpl(out, out_attr, m_registered_storages, id); R_SUCCEEDED(result) || !lr::ResultAddOnContentNotFound::Includes(result)) {
            R_RETURN(result);
        }

        /* If we failed to find the add-on content by storage, we should check if there's a registered path. */
        auto * const found = m_registered_paths.Find(id);
        R_UNLESS(found != nullptr, lr::ResultAddOnContentNotFound());

        /* Set the output path. */
        *out      = found->redir_path.path;
        *out_attr = found->redir_path.attributes;
        R_SUCCEED();

    }

    Result AddOnContentLocationResolverImpl::ResolveAddOnContentPath(sf::Out<Path> out, ncm::DataId id) {
        RedirectionAttributes attr;
        R_RETURN(this->ResolveAddOnContentPath(out.GetPointer(), std::addressof(attr), id));
    }

    Result AddOnContentLocationResolverImpl::RegisterAddOnContentStorageDeprecated(ncm::DataId id, ncm::StorageId storage_id) {
        /* Register storage for the given program id. 2.0.0-8.1.0 did not require an owner application id. */
        R_UNLESS(m_registered_storages.Register(id, storage_id, ncm::InvalidApplicationId), lr::ResultTooManyRegisteredPaths());
        R_SUCCEED();
    }

    Result AddOnContentLocationResolverImpl::RegisterAddOnContentStorage(ncm::DataId id, ncm::ApplicationId application_id, ncm::StorageId storage_id) {
        /* Register storage for the given program id and owner application. */
        R_UNLESS(m_registered_storages.Register(id, storage_id, application_id), lr::ResultTooManyRegisteredPaths());
        R_SUCCEED();
    }

    Result AddOnContentLocationResolverImpl::UnregisterAllAddOnContentPath() {
        m_registered_storages.Clear();
        m_registered_paths.RemoveAll();
        m_registered_other_paths.RemoveAll();
        R_SUCCEED();
    }

    Result AddOnContentLocationResolverImpl::RefreshApplicationAddOnContent(const sf::InArray<ncm::ApplicationId> &ids) {
        /* Clear all registered storages excluding the provided program ids. */
        m_registered_storages.ClearExcluding(reinterpret_cast<const ncm::ProgramId *>(ids.GetPointer()), ids.GetSize());

        auto ShouldRefresh = [&ids](const ncm::DataId &, const OwnedPath &owned_path) {
            for (size_t i = 0; i < ids.GetSize(); ++i) {
                if (owned_path.owner_id == ids[i]) {
                    return false;
                }
            }

            return true;
        };

        m_registered_paths.RemoveIf(ShouldRefresh);
        m_registered_other_paths.RemoveIf(ShouldRefresh);

        R_SUCCEED();
    }

    Result AddOnContentLocationResolverImpl::UnregisterApplicationAddOnContent(ncm::ApplicationId id) {
        /* Remove entries belonging to the provided application. */
        m_registered_storages.UnregisterOwnerProgram(id);

        auto ShouldRefresh = [&id](const ncm::DataId &, const OwnedPath &owned_path) {
            return owned_path.owner_id == id;
        };

        m_registered_paths.RemoveIf(ShouldRefresh);
        m_registered_other_paths.RemoveIf(ShouldRefresh);

        R_SUCCEED();
    }

    Result AddOnContentLocationResolverImpl::GetRegisteredAddOnContentPaths(Path *out, RedirectionAttributes *out_attr, Path *out2, RedirectionAttributes *out_attr2, ncm::DataId id) {
        /* Find a registered path. */
        auto * const found = m_registered_paths.Find(id);
        if (found == nullptr) {
            /* We have no registered path, so perform a normal resolution. */
            R_TRY(ResolveAddOnContentPathImpl(out, out_attr, m_registered_storages, id));

            /* Clear the second output path. */
            *out2      = {};
            *out_attr2 = {};
            R_SUCCEED();
        }

        /* Set the output path. */
        *out      = found->redir_path.path;
        *out_attr = found->redir_path.attributes;

        /* If we have a second path, set it to output. */
        if (auto * const found2 = m_registered_other_paths.Find(id); found2 != nullptr) {
            *out2      = found2->redir_path.path;
            *out_attr2 = found2->redir_path.attributes;
        } else {
            *out2      = {};
            *out_attr2 = {};
        }

        R_SUCCEED();
    }

    Result AddOnContentLocationResolverImpl::GetRegisteredAddOnContentPaths(sf::Out<PathByMapAlias> out, sf::Out<PathByMapAlias> out2, ncm::DataId id) {
        RedirectionAttributes attr, attr2;
        R_RETURN(this->GetRegisteredAddOnContentPaths(out.GetPointer(), std::addressof(attr), out2.GetPointer(), std::addressof(attr2), id));
    }

    Result AddOnContentLocationResolverImpl::RegisterAddOnContentPath(ncm::DataId id, ncm::ApplicationId application_id, const lr::PathByMapAlias &path) {
        R_RETURN(this->RegisterAddOnContentPaths(id, application_id, path, DefaultRedirectionAttributes, EmptyPath, DefaultRedirectionAttributes));
    }

    Result AddOnContentLocationResolverImpl::RegisterAddOnContentPaths(ncm::DataId id, ncm::ApplicationId application_id, const lr::PathByMapAlias &path, const lr::PathByMapAlias &path2) {
        R_RETURN(this->RegisterAddOnContentPaths(id, application_id, path, DefaultRedirectionAttributes, path2, DefaultRedirectionAttributes));
    }

    Result AddOnContentLocationResolverImpl::RegisterAddOnContentPaths(ncm::DataId id, ncm::ApplicationId application_id, const Path &path, const RedirectionAttributes &attr, const Path &path2, const RedirectionAttributes &attr2) {
        /* Check that it's possible for us to register the path. */
        /* NOTE: This check is technically incorrect, if the id is already registered. */
        R_UNLESS(!m_registered_paths.IsFull(), lr::ResultTooManyRegisteredPaths());

        /* Check that the input path isn't empty. */
        R_UNLESS(path.str[0] != '\x00', lr::ResultInvalidPath());

        /* Insert the path. */
        m_registered_paths.InsertOrAssign(id, OwnedPath{ { path, attr }, application_id });

        /* If we have a second path, insert it. */
        if (path2.str[0] != '\x00') {
            m_registered_other_paths.InsertOrAssign(id, OwnedPath { { path2, attr2 }, application_id });
        } else {
            m_registered_other_paths.Remove(id);
        }

        R_SUCCEED();
    }

}
