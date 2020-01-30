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

/* All kernel code should have access to libvapours. */
#include <vapours.hpp>

/* First, pull in core macros (panic, etc). */
#include "mesosphere/kern_panic.hpp"

/* Primitive types. */
#include "mesosphere/kern_k_typed_address.hpp"
#include "mesosphere/kern_initial_process.hpp"

/* Core pre-initialization includes. */
#include "mesosphere/kern_select_cpu.hpp"
#include "mesosphere/kern_select_k_system_control.hpp"

/* Initialization headers. */
#include "mesosphere/init/kern_init_elf.hpp"
#include "mesosphere/init/kern_init_layout.hpp"
#include "mesosphere/init/kern_init_page_table_select.hpp"
#include "mesosphere/init/kern_init_arguments_select.hpp"
#include "mesosphere/kern_k_memory_layout.hpp"

/* Core functionality. */
#include "mesosphere/kern_select_interrupt_manager.hpp"
#include "mesosphere/kern_k_spin_lock.hpp"
#include "mesosphere/kern_k_page_heap.hpp"
#include "mesosphere/kern_k_memory_manager.hpp"
#include "mesosphere/kern_k_interrupt_task_manager.hpp"
#include "mesosphere/kern_k_core_local_region.hpp"
#include "mesosphere/kern_k_slab_heap.hpp"
#include "mesosphere/kern_k_light_lock.hpp"
#include "mesosphere/kern_kernel.hpp"

/* Auto Objects. */
#include "mesosphere/kern_k_auto_object.hpp"

/* Supervisor Calls. */
#include "mesosphere/kern_svc.hpp"

/* Main functionality. */
#include "mesosphere/kern_main.hpp"
