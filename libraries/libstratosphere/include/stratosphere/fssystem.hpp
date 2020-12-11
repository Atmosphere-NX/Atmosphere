/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <stratosphere/fssystem/fssystem_allocator_utility.hpp>
#include <stratosphere/fssystem/fssystem_utility.hpp>
#include <stratosphere/fssystem/fssystem_speed_emulation_configuration.hpp>
#include <stratosphere/fssystem/fssystem_external_code.hpp>
#include <stratosphere/fssystem/fssystem_partition_file_system.hpp>
#include <stratosphere/fssystem/fssystem_partition_file_system_meta.hpp>
#include <stratosphere/fssystem/fssystem_thread_priority_changer.hpp>
#include <stratosphere/fssystem/fssystem_aes_ctr_storage.hpp>
#include <stratosphere/fssystem/fssystem_aes_xts_storage.hpp>
#include <stratosphere/fssystem/fssystem_subdirectory_filesystem.hpp>
#include <stratosphere/fssystem/fssystem_directory_redirection_filesystem.hpp>
#include <stratosphere/fssystem/fssystem_directory_savedata_filesystem.hpp>
#include <stratosphere/fssystem/fssystem_romfs_file_system.hpp>
#include <stratosphere/fssystem/fssystem_bucket_tree.hpp>
#include <stratosphere/fssystem/fssystem_bucket_tree_template_impl.hpp>
#include <stratosphere/fssystem/fssystem_indirect_storage.hpp>
#include <stratosphere/fssystem/fssystem_indirect_storage_template_impl.hpp>
#include <stratosphere/fssystem/fssystem_sparse_storage.hpp>
#include <stratosphere/fssystem/fssystem_nca_header.hpp>
#include <stratosphere/fssystem/fssystem_nca_file_system_driver.hpp>
#include <stratosphere/fssystem/fssystem_nca_file_system_driver_impl.hpp>
#include <stratosphere/fssystem/fssystem_crypto_configuration.hpp>
#include <stratosphere/fssystem/fssystem_aes_ctr_counter_extended_storage.hpp>
#include <stratosphere/fssystem/buffers/fssystem_buffer_manager_utils.hpp>
#include <stratosphere/fssystem/buffers/fssystem_file_system_buffer_manager.hpp>
#include <stratosphere/fssystem/fssystem_pooled_buffer.hpp>
#include <stratosphere/fssystem/fssystem_alignment_matching_storage_impl.hpp>
#include <stratosphere/fssystem/fssystem_alignment_matching_storage.hpp>
#include <stratosphere/fssystem/save/fssystem_buffered_storage.hpp>
#include <stratosphere/fssystem/save/fssystem_hierarchical_integrity_verification_storage.hpp>
#include <stratosphere/fssystem/fssystem_integrity_romfs_storage.hpp>
#include <stratosphere/fssystem/fssystem_file_system_proxy_api.hpp>