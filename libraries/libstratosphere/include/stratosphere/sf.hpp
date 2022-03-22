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

#include <stratosphere/sf/sf_common.hpp>
#include <stratosphere/sf/sf_allocation_policies.hpp>
#include <stratosphere/sf/sf_lmem_utility.hpp>
#include <stratosphere/sf/sf_mem_utility.hpp>
#include <stratosphere/sf/sf_exp_heap_allocator.hpp>
#include <stratosphere/sf/sf_standard_allocation_policy.hpp>
#include <stratosphere/sf/sf_std_allocation_policy.hpp>
#include <stratosphere/sf/sf_shared_object.hpp>
#include <stratosphere/sf/sf_service_object.hpp>
#include <stratosphere/sf/hipc/sf_hipc_server_session_manager.hpp>

#include <stratosphere/sf/cmif/sf_cmif_inline_context.hpp>
#include <stratosphere/sf/sf_fs_inline_context.hpp>

#include <stratosphere/sf/sf_out.hpp>
#include <stratosphere/sf/sf_native_handle.hpp>
#include <stratosphere/sf/sf_buffers.hpp>
#include <stratosphere/sf/impl/sf_impl_command_serialization.hpp>
#include <stratosphere/sf/impl/sf_impl_autogen_interface_macros.hpp>
#include <stratosphere/sf/impl/sf_impl_autogen_impl_macros.hpp>
#include <stratosphere/sf/impl/sf_impl_template_base.hpp>
#include <stratosphere/sf/sf_object_factory.hpp>

#include <stratosphere/sf/hipc/sf_hipc_server_manager.hpp>

#include <stratosphere/sf/sf_mitm_dispatch.h>
