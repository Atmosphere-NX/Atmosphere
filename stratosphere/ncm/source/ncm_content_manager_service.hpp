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
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::ncm {

    class ContentManagerService final : public sf::IServiceObject {
        protected:
            enum class CommandId {
                CreateContentStorage                = 0,
                CreateContentMetaDatabase           = 1,
                VerifyContentStorage                = 2,
                VerifyContentMetaDatabase           = 3,
                OpenContentStorage                  = 4,
                OpenContentMetaDatabase             = 5,
                CloseContentStorageForcibly         = 6,
                CloseContentMetaDatabaseForcibly    = 7,
                CleanupContentMetaDatabase          = 8,
                ActivateContentStorage              = 9,
                InactivateContentStorage            = 10,
                ActivateContentMetaDatabase         = 11,
                InactivateContentMetaDatabase       = 12,
                InvalidateRightsIdCache             = 13,
            };
        public:
            Result CreateContentStorage(StorageId storage_id);
            Result CreateContentMetaDatabase(StorageId storage_id);
            Result VerifyContentStorage(StorageId storage_id);
            Result VerifyContentMetaDatabase(StorageId storage_id);
            Result OpenContentStorage(sf::Out<std::shared_ptr<IContentStorage>> out, StorageId storage_id);
            Result OpenContentMetaDatabase(sf::Out<std::shared_ptr<IContentMetaDatabase>> out, StorageId storage_id);
            Result CloseContentStorageForcibly(StorageId storage_id);
            Result CloseContentMetaDatabaseForcibly(StorageId storage_id);
            Result CleanupContentMetaDatabase(StorageId storage_id);
            Result ActivateContentStorage(StorageId storage_id);
            Result InactivateContentStorage(StorageId storage_id);
            Result ActivateContentMetaDatabase(StorageId storage_id);
            Result InactivateContentMetaDatabase(StorageId storage_id);
            Result InvalidateRightsIdCache();
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
