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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "impl/lr_registered_data.hpp"

namespace ams::lr {

    class AddOnContentLocationResolverInterface : public sf::IServiceObject {
        protected:
            enum class CommandId {
                ResolveAddOnContentPath = 0,
                RegisterAddOnContentStorageDeprecated = 1,
                RegisterAddOnContentStorage = 1,
                UnregisterAllAddOnContentPath = 2,
                RefreshApplicationAddOnContent = 3,
                UnregisterApplicationAddOnContent = 4,
            };
        private:
            impl::RegisteredStorages<ncm::ProgramId, 0x800> registered_storages;
        public:
            AddOnContentLocationResolverInterface() : registered_storages(hos::GetVersion() < hos::Version_900 ? 0x800 : 0x2) { /* ... */ }

            virtual Result ResolveAddOnContentPath(sf::Out<Path> out, ncm::ProgramId tid);
            virtual Result RegisterAddOnContentStorageDeprecated(ncm::StorageId storage_id, ncm::ProgramId tid);
            virtual Result RegisterAddOnContentStorage(ncm::StorageId storage_id, ncm::ProgramId tid, ncm::ProgramId application_tid);
            virtual Result UnregisterAllAddOnContentPath();
            virtual Result RefreshApplicationAddOnContent(const sf::InArray<ncm::ProgramId> &tids);
            virtual Result UnregisterApplicationAddOnContent(ncm::ProgramId tid);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ResolveAddOnContentPath,               hos::Version_200),
                MAKE_SERVICE_COMMAND_META(RegisterAddOnContentStorageDeprecated, hos::Version_200, hos::Version_810),
                MAKE_SERVICE_COMMAND_META(RegisterAddOnContentStorage,           hos::Version_900),
                MAKE_SERVICE_COMMAND_META(UnregisterAllAddOnContentPath,         hos::Version_200),
                MAKE_SERVICE_COMMAND_META(RefreshApplicationAddOnContent,        hos::Version_900),
                MAKE_SERVICE_COMMAND_META(UnregisterApplicationAddOnContent,     hos::Version_900),
            };
    };

}
