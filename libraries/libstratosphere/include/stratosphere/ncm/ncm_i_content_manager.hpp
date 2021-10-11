/*
 * Copyright (c) Atmosphère-NX
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

#define AMS_NCM_I_CONTENT_MANAGER_INTERFACE_INFO(C, H)                                                                                                                                                                        \
    AMS_SF_METHOD_INFO(C, H,  0, Result, CreateContentStorage,             (ncm::StorageId storage_id),                                                            (storage_id))                                              \
    AMS_SF_METHOD_INFO(C, H,  1, Result, CreateContentMetaDatabase,        (ncm::StorageId storage_id),                                                            (storage_id))                                              \
    AMS_SF_METHOD_INFO(C, H,  2, Result, VerifyContentStorage,             (ncm::StorageId storage_id),                                                            (storage_id))                                              \
    AMS_SF_METHOD_INFO(C, H,  3, Result, VerifyContentMetaDatabase,        (ncm::StorageId storage_id),                                                            (storage_id))                                              \
    AMS_SF_METHOD_INFO(C, H,  4, Result, OpenContentStorage,               (sf::Out<sf::SharedPointer<ncm::IContentStorage>> out, ncm::StorageId storage_id),      (out, storage_id))                                         \
    AMS_SF_METHOD_INFO(C, H,  5, Result, OpenContentMetaDatabase,          (sf::Out<sf::SharedPointer<ncm::IContentMetaDatabase>> out, ncm::StorageId storage_id), (out, storage_id))                                         \
    AMS_SF_METHOD_INFO(C, H,  6, Result, CloseContentStorageForcibly,      (ncm::StorageId storage_id),                                                            (storage_id),      hos::Version_1_0_0, hos::Version_1_0_0) \
    AMS_SF_METHOD_INFO(C, H,  7, Result, CloseContentMetaDatabaseForcibly, (ncm::StorageId storage_id),                                                            (storage_id),      hos::Version_1_0_0, hos::Version_1_0_0) \
    AMS_SF_METHOD_INFO(C, H,  8, Result, CleanupContentMetaDatabase,       (ncm::StorageId storage_id),                                                            (storage_id))                                              \
    AMS_SF_METHOD_INFO(C, H,  9, Result, ActivateContentStorage,           (ncm::StorageId storage_id),                                                            (storage_id),      hos::Version_2_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 10, Result, InactivateContentStorage,         (ncm::StorageId storage_id),                                                            (storage_id),      hos::Version_2_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 11, Result, ActivateContentMetaDatabase,      (ncm::StorageId storage_id),                                                            (storage_id),      hos::Version_2_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 12, Result, InactivateContentMetaDatabase,    (ncm::StorageId storage_id),                                                            (storage_id),      hos::Version_2_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 13, Result, InvalidateRightsIdCache,          (),                                                                                     (),                hos::Version_9_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 14, Result, GetMemoryReport,                  (sf::Out<ncm::MemoryReport> out),                                                       (out),             hos::Version_10_0_0)

AMS_SF_DEFINE_INTERFACE(ams::ncm, IContentManager, AMS_NCM_I_CONTENT_MANAGER_INTERFACE_INFO);
