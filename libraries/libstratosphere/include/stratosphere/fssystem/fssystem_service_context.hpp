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
#include <vapours.hpp>
#include <stratosphere/fs/fs_priority.hpp>

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */

    class ServiceContext {
        private:
            struct DeferredProcessContextForDeviceError {
                u64 process_id;
                bool is_process_deferred;
                bool is_invoke_deferral_requested;
            };

            struct DeferredProcessContextForPriority {
                int session_type;
                bool is_process_deferred;
                bool is_acquired;
            };
        private:
            fs::PriorityRaw m_priority;
            DeferredProcessContextForDeviceError m_deferred_process_context_for_device_error;
            DeferredProcessContextForPriority m_deferred_process_context_for_priority;
            int m_storage_flag;
            void *m_request_hook_context;
            bool m_enable_count_failed_ideal_pooled_buffer_allocations;
        public:
            ServiceContext() : m_priority(fs::PriorityRaw_Normal), m_storage_flag(0), m_request_hook_context(nullptr), m_enable_count_failed_ideal_pooled_buffer_allocations(false) {
                /* ... */
            }
    };

    void RegisterServiceContext(ServiceContext *context);
    void UnregisterServiceContext();

    ServiceContext *GetServiceContext();

}
