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

#include <stratosphere/os/os_common_types.hpp>
#include <stratosphere/os/os_tick.hpp>
#include <stratosphere/os/os_memory_common.hpp>
#include <stratosphere/os/os_memory_fence.hpp>
#include <stratosphere/os/os_memory_permission.hpp>
#include <stratosphere/os/os_memory_heap_api.hpp>
#include <stratosphere/os/os_memory_virtual_address_api.hpp>
#include <stratosphere/os/os_native_handle.hpp>
#include <stratosphere/os/os_process_handle_api.hpp>
#include <stratosphere/os/os_random.hpp>
#include <stratosphere/os/os_mutex.hpp>
#include <stratosphere/os/os_condition_variable.hpp>
#include <stratosphere/os/os_sdk_mutex.hpp>
#include <stratosphere/os/os_sdk_recursive_mutex.hpp>
#include <stratosphere/os/os_sdk_condition_variable.hpp>
#include <stratosphere/os/os_busy_mutex.hpp>
#include <stratosphere/os/os_rw_busy_mutex.hpp>
#include <stratosphere/os/os_rw_lock.hpp>
#include <stratosphere/os/os_shared_memory.hpp>
#include <stratosphere/os/os_transfer_memory.hpp>
#include <stratosphere/os/os_semaphore.hpp>
#include <stratosphere/os/os_event.hpp>
#include <stratosphere/os/os_system_event.hpp>
#include <stratosphere/os/os_interrupt_event.hpp>
#include <stratosphere/os/os_timer_event.hpp>
#include <stratosphere/os/os_thread_local_storage.hpp>
#include <stratosphere/os/os_sdk_thread_local_storage.hpp>
#include <stratosphere/os/os_sdk_reply_and_receive.hpp>
#include <stratosphere/os/os_thread.hpp>
#include <stratosphere/os/os_message_queue.hpp>
#include <stratosphere/os/os_light_event.hpp>
#include <stratosphere/os/os_light_message_queue.hpp>
#include <stratosphere/os/os_light_semaphore.hpp>
#include <stratosphere/os/os_barrier.hpp>
#include <stratosphere/os/os_io_region.hpp>
#include <stratosphere/os/os_multiple_wait.hpp>
#include <stratosphere/os/os_argument.hpp>
#include <stratosphere/os/os_cache.hpp>
