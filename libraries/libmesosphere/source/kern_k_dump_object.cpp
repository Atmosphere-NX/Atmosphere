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

namespace ams::kern::KDumpObject {

    namespace {

        constexpr const char * const ThreadStates[] = {
            [KThread::ThreadState_Initialized] = "Initialized",
            [KThread::ThreadState_Waiting]     = "Waiting",
            [KThread::ThreadState_Runnable]    = "Runnable",
            [KThread::ThreadState_Terminated]  = "Terminated",
        };

        void DumpThread(KThread *thread) {
            if (KProcess *process = thread->GetOwnerProcess(); process != nullptr) {
                MESOSPHERE_LOG("Thread ID=%5lu pid=%3lu %-11s Pri=%2d %-11s KernelStack=%4zu/%4zu Run=%d Ideal=%d (%d) Affinity=%016lx (%016lx)\n",
                               thread->GetId(), process->GetId(), process->GetName(), thread->GetPriority(), ThreadStates[thread->GetState()],
                               thread->GetKernelStackUsage(), PageSize, thread->GetActiveCore(), thread->GetIdealVirtualCore(), thread->GetIdealPhysicalCore(),
                               thread->GetVirtualAffinityMask(), thread->GetAffinityMask().GetAffinityMask());

                MESOSPHERE_LOG("    State: 0x%04x Suspend: 0x%04x Dpc: 0x%x\n", thread->GetRawState(), thread->GetSuspendFlags(), thread->GetDpc());

                MESOSPHERE_LOG("    TLS: %p (%p)\n", GetVoidPointer(thread->GetThreadLocalRegionAddress()), thread->GetThreadLocalRegionHeapAddress());
            } else {
                MESOSPHERE_LOG("Thread ID=%5lu pid=%3d %-11s Pri=%2d %-11s KernelStack=%4zu/%4zu Run=%d Ideal=%d (%d) Affinity=%016lx (%016lx)\n",
                               thread->GetId(), -1, "(kernel)", thread->GetPriority(), ThreadStates[thread->GetState()],
                               thread->GetKernelStackUsage(), PageSize, thread->GetActiveCore(), thread->GetIdealVirtualCore(), thread->GetIdealPhysicalCore(),
                               thread->GetVirtualAffinityMask(), thread->GetAffinityMask().GetAffinityMask());
            }
        }

    }

    void DumpThread() {
        MESOSPHERE_LOG("Dump Thread\n");

        {
            /* Lock the list. */
            KThread::ListAccessor accessor;
            const auto end = accessor.end();

            /* Dump each thread. */
            for (auto it = accessor.begin(); it != end; ++it) {
                DumpThread(static_cast<KThread *>(std::addressof(*it)));
            }
        }

        MESOSPHERE_LOG("\n");
    }

    void DumpThread(u64 thread_id) {
        MESOSPHERE_LOG("Dump Thread\n");

        {
            /* Find and dump the target thread. */
            if (KThread *thread = KThread::GetThreadFromId(thread_id); thread != nullptr) {
                ON_SCOPE_EXIT { thread->Close(); };
                DumpThread(thread);
            }
        }

        MESOSPHERE_LOG("\n");
    }

}
