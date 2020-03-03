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

namespace ams::ncm {

    class ContentManagerImpl final : public IContentManager {
        public:
            virtual Result CreateContentStorage(StorageId storage_id) override;
            virtual Result CreateContentMetaDatabase(StorageId storage_id) override;
            virtual Result VerifyContentStorage(StorageId storage_id) override;
            virtual Result VerifyContentMetaDatabase(StorageId storage_id) override;
            virtual Result OpenContentStorage(sf::Out<std::shared_ptr<IContentStorage>> out, StorageId storage_id) override;
            virtual Result OpenContentMetaDatabase(sf::Out<std::shared_ptr<IContentMetaDatabase>> out, StorageId storage_id) override;
            virtual Result CloseContentStorageForcibly(StorageId storage_id) override;
            virtual Result CloseContentMetaDatabaseForcibly(StorageId storage_id) override;
            virtual Result CleanupContentMetaDatabase(StorageId storage_id) override;
            virtual Result ActivateContentStorage(StorageId storage_id) override;
            virtual Result InactivateContentStorage(StorageId storage_id) override;
            virtual Result ActivateContentMetaDatabase(StorageId storage_id) override;
            virtual Result InactivateContentMetaDatabase(StorageId storage_id) override;
            virtual Result InvalidateRightsIdCache() override;
    };

}
