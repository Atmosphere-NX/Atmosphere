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
#include <stratosphere/ncm/ncm_i_content_storage.hpp>
#include <stratosphere/ncm/ncm_i_content_meta_database.hpp>
#include <stratosphere/ncm/ncm_memory_report.hpp>

namespace ams::ncm {

    class IContentManager : public sf::IServiceObject {
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
                GetMemoryReport                     = 14,
            };
        public:
            virtual Result CreateContentStorage(StorageId storage_id) = 0;
            virtual Result CreateContentMetaDatabase(StorageId storage_id) = 0;
            virtual Result VerifyContentStorage(StorageId storage_id) = 0;
            virtual Result VerifyContentMetaDatabase(StorageId storage_id) = 0;
            virtual Result OpenContentStorage(sf::Out<std::shared_ptr<IContentStorage>> out, StorageId storage_id) = 0;
            virtual Result OpenContentMetaDatabase(sf::Out<std::shared_ptr<IContentMetaDatabase>> out, StorageId storage_id) = 0;
            virtual Result CloseContentStorageForcibly(StorageId storage_id) = 0;
            virtual Result CloseContentMetaDatabaseForcibly(StorageId storage_id) = 0;
            virtual Result CleanupContentMetaDatabase(StorageId storage_id) = 0;
            virtual Result ActivateContentStorage(StorageId storage_id) = 0;
            virtual Result InactivateContentStorage(StorageId storage_id) = 0;
            virtual Result ActivateContentMetaDatabase(StorageId storage_id) = 0;
            virtual Result InactivateContentMetaDatabase(StorageId storage_id) = 0;
            virtual Result InvalidateRightsIdCache() = 0;
            virtual Result GetMemoryReport(sf::Out<MemoryReport> out) = 0;
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(CreateContentStorage),
                MAKE_SERVICE_COMMAND_META(CreateContentMetaDatabase),
                MAKE_SERVICE_COMMAND_META(VerifyContentStorage),
                MAKE_SERVICE_COMMAND_META(VerifyContentMetaDatabase),
                MAKE_SERVICE_COMMAND_META(OpenContentStorage),
                MAKE_SERVICE_COMMAND_META(OpenContentMetaDatabase),
                MAKE_SERVICE_COMMAND_META(CloseContentStorageForcibly,      hos::Version_1_0_0, hos::Version_1_0_0),
                MAKE_SERVICE_COMMAND_META(CloseContentMetaDatabaseForcibly, hos::Version_1_0_0, hos::Version_1_0_0),
                MAKE_SERVICE_COMMAND_META(CleanupContentMetaDatabase),
                MAKE_SERVICE_COMMAND_META(ActivateContentStorage,           hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(InactivateContentStorage,         hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(ActivateContentMetaDatabase,      hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(InactivateContentMetaDatabase,    hos::Version_2_0_0),
                MAKE_SERVICE_COMMAND_META(InvalidateRightsIdCache,          hos::Version_9_0_0),
                MAKE_SERVICE_COMMAND_META(GetMemoryReport,                  hos::Version_10_0_0),
            };
    };

}
