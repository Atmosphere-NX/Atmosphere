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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_slab_helpers.hpp>

namespace ams::kern {

    class KEventInfo : public KSlabAllocated<KEventInfo>, public util::IntrusiveListBaseNode<KEventInfo> {
        public:
            struct InfoCreateThread {
                u32 thread_id;
                uintptr_t tls_address;
            };

            struct InfoExitProcess {
                ams::svc::ProcessExitReason reason;
            };

            struct InfoExitThread {
                ams::svc::ThreadExitReason reason;
            };

            struct InfoException {
                ams::svc::DebugException exception_type;
                s32 exception_data_count;
                uintptr_t exception_address;
                uintptr_t exception_data[4];
            };

            struct InfoSystemCall {
                s64 tick;
                s32 id;
            };
        public:
            ams::svc::DebugEvent event;
            u32 thread_id;
            u32 flags;
            bool is_attached;
            bool continue_flag;
            bool ignore_continue;
            bool close_once;
            union {
                InfoCreateThread create_thread;
                InfoExitProcess exit_process;
                InfoExitThread exit_thread;
                InfoException exception;
                InfoSystemCall system_call;
            } info;
            KThread *debug_thread;
        public:
            explicit KEventInfo() : is_attached(), continue_flag(), ignore_continue() { /* ... */ }
            ~KEventInfo() { /* ... */ }
    };

}
