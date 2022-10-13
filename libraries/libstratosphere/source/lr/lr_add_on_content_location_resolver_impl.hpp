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
#include "lr_location_redirector.hpp"
#include "lr_registered_data.hpp"

namespace ams::lr {

    class AddOnContentLocationResolverImpl {
        private:
            struct OwnedPath {
                RedirectionPath redir_path;
                ncm::ApplicationId owner_id;
            };
        private:
            /* Storage for RegisteredData entries by data id. */
            RegisteredStorages<ncm::DataId, 0x800> m_registered_storages;
            ncm::BoundedMap<ncm::DataId, OwnedPath, 8> m_registered_paths;
            ncm::BoundedMap<ncm::DataId, OwnedPath, 8> m_registered_other_paths;
        private:
            static ALWAYS_INLINE size_t GetStorageCapacity() {
                const auto version = hos::GetVersion();
                if (version >= hos::Version_12_0_0) {
                    return 0x8;
                } else if (version >= hos::Version_9_0_0) {
                    return 0x2;
                } else {
                    return 0x800;
                }
            }
        public:
            AddOnContentLocationResolverImpl() : m_registered_storages(GetStorageCapacity()), m_registered_paths{}, m_registered_other_paths{} { /* ... */ }

            /* Actual commands. */
            Result ResolveAddOnContentPath(sf::Out<Path> out, ncm::DataId id);
            Result RegisterAddOnContentStorageDeprecated(ncm::DataId id, ncm::StorageId storage_id);
            Result RegisterAddOnContentStorage(ncm::DataId id, ncm::ApplicationId application_id, ncm::StorageId storage_id);
            Result UnregisterAllAddOnContentPath();
            Result RefreshApplicationAddOnContent(const sf::InArray<ncm::ApplicationId> &ids);
            Result UnregisterApplicationAddOnContent(ncm::ApplicationId id);
            Result GetRegisteredAddOnContentPaths(sf::Out<PathByMapAlias> out, sf::Out<PathByMapAlias> out2, ncm::DataId id);
            Result RegisterAddOnContentPath(ncm::DataId id, ncm::ApplicationId application_id, const PathByMapAlias &path);
            Result RegisterAddOnContentPaths(ncm::DataId id, ncm::ApplicationId application_id, const PathByMapAlias &path, const PathByMapAlias &path2);
        private:
            Result ResolveAddOnContentPath(Path *out, RedirectionAttributes *out_attr, ncm::DataId id);
            Result GetRegisteredAddOnContentPaths(Path *out, RedirectionAttributes *out_attr, Path *out2, RedirectionAttributes *out_attr2, ncm::DataId id);
            Result RegisterAddOnContentPaths(ncm::DataId id, ncm::ApplicationId application_id, const Path &path, const RedirectionAttributes &attr, const Path &path2, const RedirectionAttributes &attr2);
    };
    static_assert(lr::IsIAddOnContentLocationResolver<AddOnContentLocationResolverImpl>);

}
