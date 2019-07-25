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

namespace sts::ncm {

    class ContentManagerService final : public IServiceObject {
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
            };
        public:
            ~ContentManagerService();
        public:
            virtual Result CreateContentStorage(StorageId storage_id);
            virtual Result CreateContentMetaDatabase(StorageId storage_id);
            virtual Result VerifyContentStorage(StorageId storage_id);
            virtual Result VerifyContentMetaDatabase(StorageId storage_id);
            virtual Result OpenContentStorage(Out<std::shared_ptr<IContentStorage>> out, StorageId storage_id);
            virtual Result OpenContentMetaDatabase(Out<std::shared_ptr<IContentMetaDatabase>> out, StorageId storage_id);
            virtual Result CloseContentStorageForcibly(StorageId storage_id);
            virtual Result CloseContentMetaDatabaseForcibly(StorageId storage_id);
            virtual Result CleanupContentMetaDatabase(StorageId storage_id);
            virtual Result ActivateContentStorage(StorageId storage_id);
            virtual Result InactivateContentStorage(StorageId storage_id);
            virtual Result ActivateContentMetaDatabase(StorageId storage_id);
            virtual Result InactivateContentMetaDatabase(StorageId storage_id);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ContentManagerService, CreateContentStorage),
                MAKE_SERVICE_COMMAND_META(ContentManagerService, CreateContentMetaDatabase),
                MAKE_SERVICE_COMMAND_META(ContentManagerService, VerifyContentStorage),
                MAKE_SERVICE_COMMAND_META(ContentManagerService, VerifyContentMetaDatabase),
                MAKE_SERVICE_COMMAND_META(ContentManagerService, OpenContentStorage),
                MAKE_SERVICE_COMMAND_META(ContentManagerService, OpenContentMetaDatabase),
                MAKE_SERVICE_COMMAND_META(ContentManagerService, CloseContentStorageForcibly,      FirmwareVersion_100, FirmwareVersion_100),
                MAKE_SERVICE_COMMAND_META(ContentManagerService, CloseContentMetaDatabaseForcibly, FirmwareVersion_100, FirmwareVersion_100),
                MAKE_SERVICE_COMMAND_META(ContentManagerService, CleanupContentMetaDatabase),
                MAKE_SERVICE_COMMAND_META(ContentManagerService, ActivateContentStorage,           FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(ContentManagerService, InactivateContentStorage,         FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(ContentManagerService, ActivateContentMetaDatabase,      FirmwareVersion_200),
                MAKE_SERVICE_COMMAND_META(ContentManagerService, InactivateContentMetaDatabase,    FirmwareVersion_200),
            };
    };

}