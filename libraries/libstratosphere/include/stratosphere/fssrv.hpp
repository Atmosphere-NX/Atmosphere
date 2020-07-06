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
#include <stratosphere/fssrv/sf/fssrv_sf_path.hpp>
#include <stratosphere/fssrv/sf/fssrv_sf_ifile.hpp>
#include <stratosphere/fssrv/fssrv_path_normalizer.hpp>
#include <stratosphere/fssrv/fssrv_nca_crypto_configuration.hpp>
#include <stratosphere/fssrv/fssrv_memory_resource_from_standard_allocator.hpp>
#include <stratosphere/fssrv/fssrv_memory_resource_from_exp_heap.hpp>
#include <stratosphere/fssrv/fssrv_i_file_system_creator.hpp>
#include <stratosphere/fssrv/fscreator/fssrv_partition_file_system_creator.hpp>
#include <stratosphere/fssrv/fscreator/fssrv_rom_file_system_creator.hpp>
#include <stratosphere/fssrv/fscreator/fssrv_storage_on_nca_creator.hpp>
#include <stratosphere/fssrv/fssrv_file_system_proxy_api.hpp>
