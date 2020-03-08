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
#include <stratosphere/ncm/ncm_content_meta_database.hpp>
#include <stratosphere/ncm/ncm_content_storage.hpp>
#include <stratosphere/ncm/ncm_i_content_manager.hpp>

namespace ams::ncm {

    /* Management. */
    void Initialize();
    void Finalize();

    void InitializeWithObject(std::shared_ptr<IContentManager> manager_object);

    /* Service API. */
    Result CreateContentStorage(StorageId storage_id);
    Result CreateContentMetaDatabase(StorageId storage_id);

    Result VerifyContentStorage(StorageId storage_id);
    Result VerifyContentMetaDatabase(StorageId storage_id);

    Result OpenContentStorage(ContentStorage *out, StorageId storage_id);
    Result OpenContentMetaDatabase(ContentMetaDatabase *out, StorageId storage_id);

    Result CleanupContentMetaDatabase(StorageId storage_id);

    Result ActivateContentStorage(StorageId storage_id);
    Result InactivateContentStorage(StorageId storage_id);

    Result ActivateContentMetaDatabase(StorageId storage_id);
    Result InactivateContentMetaDatabase(StorageId storage_id);

    Result InvalidateRightsIdCache();

    /* Deprecated API. */
    Result CloseContentStorageForcibly(StorageId storage_id);
    Result CloseContentMetaDatabaseForcibly(StorageId storage_id);

}
