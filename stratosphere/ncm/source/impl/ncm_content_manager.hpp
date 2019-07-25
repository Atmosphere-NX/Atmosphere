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

#include "../ncm_icontentmetadatabase.hpp"
#include "../ncm_icontentstorage.hpp"

namespace sts::ncm::impl {

    /* Initialization/Finalization. */
    Result InitializeContentManager();
    void FinalizeContentManager();

    /* Content Storage Management. */
    Result CreateContentStorage(StorageId storage_id);
    Result VerifyContentStorage(StorageId storage_id);
    Result OpenContentStorage(std::shared_ptr<IContentStorage>* out, StorageId storage_id);
    Result CloseContentStorageForcibly(StorageId storage_id);
    Result ActivateContentStorage(StorageId storage_id);
    Result InactivateContentStorage(StorageId storage_id);

    /* Content Meta Database Management. */
    Result CreateContentMetaDatabase(StorageId storage_id);
    Result VerifyContentMetaDatabase(StorageId storage_id);
    Result OpenContentMetaDatabase(std::shared_ptr<IContentMetaDatabase>* out, StorageId storage_id);
    Result CloseContentMetaDatabaseForcibly(StorageId storage_id);
    Result CleanupContentMetaDatabase(StorageId storage_id);
    Result ActivateContentMetaDatabase(StorageId storage_id);
    Result InactivateContentMetaDatabase(StorageId storage_id);

}