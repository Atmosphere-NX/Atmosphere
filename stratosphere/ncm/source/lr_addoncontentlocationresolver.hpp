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

namespace sts::lr {

    class AddOnContentLocationResolverInterface : public IServiceObject {
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
            impl::RegisteredStorages<ncm::TitleId, 0x800> registered_storages;
        public:
            AddOnContentLocationResolverInterface() : registered_storages(GetRuntimeFirmwareVersion() < FirmwareVersion_900 ? 0x800 : 0x2) { /* ... */ }

            virtual Result ResolveAddOnContentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid);
            virtual Result RegisterAddOnContentStorageDeprecated(ncm::StorageId storage_id, ncm::TitleId tid);
            virtual Result RegisterAddOnContentStorage(ncm::StorageId storage_id, ncm::TitleId tid, ncm::TitleId application_tid);
            virtual Result UnregisterAllAddOnContentPath();
            virtual Result RefreshApplicationAddOnContent(InBuffer<ncm::TitleId> tids);
            virtual Result UnregisterApplicationAddOnContent(ncm::TitleId tid);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(AddOnContentLocationResolverInterface, ResolveAddOnContentPath,               FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(AddOnContentLocationResolverInterface, RegisterAddOnContentStorageDeprecated, FirmwareVersion_200, FirmwareVersion_810),
                MAKE_SERVICE_COMMAND_META(AddOnContentLocationResolverInterface, RegisterAddOnContentStorage,           FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(AddOnContentLocationResolverInterface, UnregisterAllAddOnContentPath,         FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(AddOnContentLocationResolverInterface, RefreshApplicationAddOnContent,        FirmwareVersion_900),
                MAKE_SERVICE_COMMAND_META(AddOnContentLocationResolverInterface, UnregisterApplicationAddOnContent,     FirmwareVersion_900),
            };
    };

}
