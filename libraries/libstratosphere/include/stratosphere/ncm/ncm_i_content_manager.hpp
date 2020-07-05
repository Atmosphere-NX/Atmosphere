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

    #define AMS_NCM_I_CONTENT_MANAGER_INTERFACE_INFO(C, H)                                                                                                                                         \
        AMS_SF_METHOD_INFO(C, H,  0, Result, CreateContentStorage,             (StorageId storage_id))                                                                                             \
        AMS_SF_METHOD_INFO(C, H,  1, Result, CreateContentMetaDatabase,        (StorageId storage_id))                                                                                             \
        AMS_SF_METHOD_INFO(C, H,  2, Result, VerifyContentStorage,             (StorageId storage_id))                                                                                             \
        AMS_SF_METHOD_INFO(C, H,  3, Result, VerifyContentMetaDatabase,        (StorageId storage_id))                                                                                             \
        AMS_SF_METHOD_INFO(C, H,  4, Result, OpenContentStorage,               (sf::Out<std::shared_ptr<IContentStorage>> out, StorageId storage_id))                                              \
        AMS_SF_METHOD_INFO(C, H,  5, Result, OpenContentMetaDatabase,          (sf::Out<std::shared_ptr<IContentMetaDatabase>> out, StorageId storage_id))                                         \
        AMS_SF_METHOD_INFO(C, H,  6, Result, CloseContentStorageForcibly,      (StorageId storage_id),                                                     hos::Version_1_0_0, hos::Version_1_0_0) \
        AMS_SF_METHOD_INFO(C, H,  7, Result, CloseContentMetaDatabaseForcibly, (StorageId storage_id),                                                     hos::Version_1_0_0, hos::Version_1_0_0) \
        AMS_SF_METHOD_INFO(C, H,  8, Result, CleanupContentMetaDatabase,       (StorageId storage_id))                                                                                             \
        AMS_SF_METHOD_INFO(C, H,  9, Result, ActivateContentStorage,           (StorageId storage_id),                                                     hos::Version_2_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 10, Result, InactivateContentStorage,         (StorageId storage_id),                                                     hos::Version_2_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 11, Result, ActivateContentMetaDatabase,      (StorageId storage_id),                                                     hos::Version_2_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 12, Result, InactivateContentMetaDatabase,    (StorageId storage_id),                                                     hos::Version_2_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 13, Result, InvalidateRightsIdCache,          (),                                                                         hos::Version_9_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 14, Result, GetMemoryReport,                  (sf::Out<MemoryReport> out),                                                hos::Version_10_0_0)

    AMS_SF_DEFINE_INTERFACE(IContentManager, AMS_NCM_I_CONTENT_MANAGER_INTERFACE_INFO);

}
