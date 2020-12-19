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
#include <mesosphere/kern_k_thread.hpp>
#include <mesosphere/kern_k_scheduler.hpp>

namespace ams::kern {

    class KConditionVariable {
        public:
            using ThreadTree = typename KThread::ConditionVariableThreadTreeType;
        private:
            ThreadTree m_tree;
        public:
            constexpr KConditionVariable() : m_tree() { /* ... */ }

            /* Arbitration. */
            Result SignalToAddress(KProcessAddress addr);
            Result WaitForAddress(ams::svc::Handle handle, KProcessAddress addr, u32 value);

            /* Condition variable. */
            void Signal(uintptr_t cv_key, s32 count);
            Result Wait(KProcessAddress addr, uintptr_t key, u32 value, s64 timeout);
        private:
            KThread *SignalImpl(KThread *thread);
    };

    ALWAYS_INLINE void BeforeUpdatePriority(KConditionVariable::ThreadTree *tree, KThread *thread) {
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        tree->erase(tree->iterator_to(*thread));
    }

    ALWAYS_INLINE void AfterUpdatePriority(KConditionVariable::ThreadTree *tree, KThread *thread) {
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

        tree->insert(*thread);
    }

}
