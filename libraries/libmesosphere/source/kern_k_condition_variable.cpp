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
#include <mesosphere.hpp>

namespace ams::kern {

    namespace {

        constinit KThread g_cv_compare_thread;

    }

    Result KConditionVariable::SignalToAddress(KProcessAddress addr) {
        MESOSPHERE_UNIMPLEMENTED();
    }

    Result KConditionVariable::WaitForAddress(ams::svc::Handle handle, KProcessAddress addr, u32 value) {
        MESOSPHERE_UNIMPLEMENTED();
    }

    KThread *KConditionVariable::SignalImpl(KThread *thread) {
        MESOSPHERE_UNIMPLEMENTED();
    }

    void KConditionVariable::Signal(uintptr_t cv_key, s32 count) {
        /* Prepare for signaling. */
        constexpr int MaxThreads = 16;
        KLinkedList<KThread> thread_list;
        KThread *thread_array[MaxThreads];
        int num_to_close = 0;

        /* Perform signaling. */
        int num_waiters = 0;
        {
            KScopedSchedulerLock sl;
            g_cv_compare_thread.SetupForConditionVariableCompare(cv_key, -1);

            auto it = this->tree.nfind(g_cv_compare_thread);
            while ((it != this->tree.end()) && (count <= 0 || num_waiters < count) && (it->GetConditionVariableKey() == cv_key)) {
                KThread *target_thread = std::addressof(*it);

                if (KThread *thread = this->SignalImpl(target_thread); thread != nullptr) {
                    if (num_to_close < MaxThreads) {
                        thread_array[num_to_close++] = thread;
                    } else {
                        thread_list.push_back(*thread);
                    }
                }

                it = this->tree.erase(it);
                target_thread->ClearConditionVariable();
                ++num_waiters;
            }
        }

        /* Close threads in the array. */
        for (auto i = 0; i < num_to_close; ++i) {
            thread_array[i]->Close();
        }

        /* Close threads in the list. */
        if (num_waiters > MaxThreads) {
            auto it = thread_list.begin();
            while (it != thread_list.end()) {
                KThread *thread = std::addressof(*it);
                thread->Close();
                it = thread_list.erase(it);
            }
        }
    }

    Result KConditionVariable::Wait(KProcessAddress addr, uintptr_t key, u32 value, s64 timeout) {
        MESOSPHERE_UNIMPLEMENTED();
    }

}
