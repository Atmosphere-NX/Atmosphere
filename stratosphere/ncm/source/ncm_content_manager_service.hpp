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

#include "ncm_icontentmetadatabase.hpp"
#include "ncm_icontentstorage.hpp"

namespace ams::ncm {

    class ContentManagerService final : public sf::IServiceObject {
        protected:
            enum class CommandId {
                CreateContentStorage = 0,
                CreateContentMetaDatabase = 1,
                VerifyContentStorage = 2,
                VerifyContentMetaDatabase = 3,
                OpenContentStorage = 4,
                OpenContentMetaDatabase = 5,
                CloseContentStorageForcibly = 6,
                CloseContentMetaDatabaseForcibly = 7,
                CleanupContentMetaDatabase = 8,
                ActivateContentStorage = 9,
                InactivateContentStorage = 10,
                ActivateContentMetaDatabase = 11,
                InactivateContentMetaDatabase = 12,
                InvalidateRightsIdCache = 13,
            };
        public:
            virtual Result CreateContentStorage(StorageId storage_id);
            virtual Result CreateContentMetaDatabase(StorageId storage_id);
            virtual Result VerifyContentStorage(StorageId storage_id);
            virtual Result VerifyContentMetaDatabase(StorageId storage_id);
            virtual Result OpenContentStorage(sf::Out<std::shared_ptr<IContentStorage>> out, StorageId storage_id);
            virtual Result OpenContentMetaDatabase(sf::Out<std::shared_ptr<IContentMetaDatabase>> out, StorageId storage_id);
            virtual Result CloseContentStorageForcibly(StorageId storage_id);
            virtual Result CloseContentMetaDatabaseForcibly(StorageId storage_id);
            virtual Result CleanupContentMetaDatabase(StorageId storage_id);
            virtual Result ActivateContentStorage(StorageId storage_id);
            virtual Result InactivateContentStorage(StorageId storage_id);
            virtual Result ActivateContentMetaDatabase(StorageId storage_id);
            virtual Result InactivateContentMetaDatabase(StorageId storage_id);
            virtual Result InvalidateRightsIdCache();
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(CreateContentStorage),
                MAKE_SERVICE_COMMAND_META(CreateContentMetaDatabase),
                MAKE_SERVICE_COMMAND_META(VerifyContentStorage),
                MAKE_SERVICE_COMMAND_META(VerifyContentMetaDatabase),
                MAKE_SERVICE_COMMAND_META(OpenContentStorage),
                MAKE_SERVICE_COMMAND_META(OpenContentMetaDatabase),
                MAKE_SERVICE_COMMAND_META(CloseContentStorageForcibly,      hos::Version_100, hos::Version_100),
                MAKE_SERVICE_COMMAND_META(CloseContentMetaDatabaseForcibly, hos::Version_100, hos::Version_100),
                MAKE_SERVICE_COMMAND_META(CleanupContentMetaDatabase),
                MAKE_SERVICE_COMMAND_META(ActivateContentStorage,           hos::Version_200),
                MAKE_SERVICE_COMMAND_META(InactivateContentStorage,         hos::Version_200),
                MAKE_SERVICE_COMMAND_META(ActivateContentMetaDatabase,      hos::Version_200),
                MAKE_SERVICE_COMMAND_META(InactivateContentMetaDatabase,    hos::Version_200),
                MAKE_SERVICE_COMMAND_META(InvalidateRightsIdCache,          hos::Version_900),
            };
    };

}
