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

    struct KConditionVariableComparator {
        static constexpr ALWAYS_INLINE int Compare(const KThread &lhs, const KThread &rhs) {
            const uintptr_t l_key = lhs.GetConditionVariableKey();
            const uintptr_t r_key = rhs.GetConditionVariableKey();

            if (l_key < r_key) {
                /* Sort first by key */
                return -1;
            } else if (l_key == r_key && lhs.GetPriority() < rhs.GetPriority()) {
                /* And then by priority. */
                return -1;
            } else {
                return 1;
            }
        }
    };

    class KConditionVariable {
        public:
            using ThreadTree = util::IntrusiveRedBlackTreeMemberTraits<&KThread::condvar_arbiter_tree_node>::TreeType<KConditionVariableComparator>;
        private:
            ThreadTree tree;
        public:
            constexpr KConditionVariable() : tree() { /* ... */ }

            /* Arbitration. */
            Result SignalToAddress(KProcessAddress addr);
            Result WaitForAddress(ams::svc::Handle handle, KProcessAddress addr, u32 value);

            /* Condition variable. */
            void Signal(uintptr_t cv_key, s32 count);
            Result Wait(KProcessAddress addr, uintptr_t key, u32 value, s64 timeout);

            ALWAYS_INLINE void BeforeUpdatePriority(KThread *thread) {
                MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

                this->tree.erase(this->tree.iterator_to(*thread));
            }

            ALWAYS_INLINE void AfterUpdatePriority(KThread *thread) {
                MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());

                this->tree.insert(*thread);
            }
    };

}
