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
#include <stratosphere/lr/lr_types.hpp>

namespace ams::lr {

    class IAddOnContentLocationResolver : public sf::IServiceObject {
        protected:
            enum class CommandId {
                ResolveAddOnContentPath                  = 0,
                RegisterAddOnContentStorageDeprecated    = 1,
                RegisterAddOnContentStorage              = 1,
                UnregisterAllAddOnContentPath            = 2,
                RefreshApplicationAddOnContent           = 3,
                UnregisterApplicationAddOnContent        = 4,
            };
        public:
            /* Actual commands. */
            virtual Result ResolveAddOnContentPath(sf::Out<Path> out, ncm::DataId id) = 0;
            virtual Result RegisterAddOnContentStorageDeprecated(ncm::DataId id, ncm::StorageId storage_id) = 0;
            virtual Result RegisterAddOnContentStorage(ncm::DataId id, ncm::ApplicationId application_id, ncm::StorageId storage_id) = 0;
            virtual Result UnregisterAllAddOnContentPath() = 0;
            virtual Result RefreshApplicationAddOnContent(const sf::InArray<ncm::ApplicationId> &ids) = 0;
            virtual Result UnregisterApplicationAddOnContent(ncm::ApplicationId id) = 0;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ResolveAddOnContentPath,               hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(RegisterAddOnContentStorageDeprecated, hos::Version_2_0_0, hos::Version_8_1_0),
                MAKE_SERVICE_COMMAND_META(RegisterAddOnContentStorage,           hos::Version_9_0_0),
                MAKE_SERVICE_COMMAND_META(UnregisterAllAddOnContentPath,         hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(RefreshApplicationAddOnContent,        hos::Version_9_0_0),
                MAKE_SERVICE_COMMAND_META(UnregisterApplicationAddOnContent,     hos::Version_9_0_0),
            };
    };


}
