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
#include <mesosphere/kern_slab_helpers.hpp>

namespace ams::kern {


    class KThread : KAutoObjectWithSlabHeapAndContainer<KThread, /* TODO: KSynchronizationObject */ KAutoObjectWithList> {
        public:
            struct StackParameters {
                alignas(0x10) u8 svc_permission[0x10];
                std::atomic<u8> dpc_flags;
                u8 current_svc_id;
                bool is_calling_svc;
                bool is_in_exception_handler;
                bool has_exception_svc_perms;
                s32 disable_count;
                void *context; /* TODO: KThreadContext * */
            };
            static_assert(alignof(StackParameters) == 0x10);
        /* TODO: This should be a KAutoObject, and this is a placeholder definition. */
    };

}
