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
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_k_synchronization_object.hpp>
#include <mesosphere/kern_k_handle_table.hpp>
#include <mesosphere/kern_k_thread.hpp>
#include <mesosphere/kern_k_thread_local_page.hpp>

namespace ams::kern {

    class KProcess final : public KAutoObjectWithSlabHeapAndContainer<KProcess, KSynchronizationObject> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KProcess, KSynchronizationObject);
        /* TODO: This is a placeholder definition. */
        public:
            u64 GetCoreMask() const { MESOSPHERE_TODO_IMPLEMENT(); }
            u64 GetPriorityMask() const { MESOSPHERE_TODO_IMPLEMENT();}

            bool Is64Bit() const { MESOSPHERE_TODO_IMPLEMENT(); }

            KThread *GetPreemptionStatePinnedThread(s32 core_id) { MESOSPHERE_TODO_IMPLEMENT(); }

            void SetPreemptionState();
    };

}
