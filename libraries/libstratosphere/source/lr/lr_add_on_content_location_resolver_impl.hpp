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

#pragma once
#include <stratosphere.hpp>
#include "lr_location_redirector.hpp"
#include "lr_registered_data.hpp"

namespace ams::lr {

    class AddOnContentLocationResolverImpl : public IAddOnContentLocationResolver {
        private:
            /* Storage for RegisteredData entries by program id. */
            RegisteredStorages<ncm::ProgramId, 0x800> registered_storages;
        public:
            AddOnContentLocationResolverImpl() : registered_storages(hos::GetVersion() < hos::Version_900 ? 0x800 : 0x2) { /* ... */ }

            /* Actual commands. */
            virtual Result ResolveAddOnContentPath(sf::Out<Path> out, ncm::ProgramId id) override;
            virtual Result RegisterAddOnContentStorageDeprecated(ncm::StorageId storage_id, ncm::ProgramId id) override;
            virtual Result RegisterAddOnContentStorage(ncm::StorageId storage_id, ncm::ProgramId id, ncm::ProgramId application_id) override;
            virtual Result UnregisterAllAddOnContentPath() override;
            virtual Result RefreshApplicationAddOnContent(const sf::InArray<ncm::ProgramId> &ids) override;
            virtual Result UnregisterApplicationAddOnContent(ncm::ProgramId id) override;
    };

}
