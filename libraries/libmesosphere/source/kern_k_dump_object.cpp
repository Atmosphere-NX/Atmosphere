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
                MESOSPHERE_RELEASE_LOG("Thread ID=%5lu pid=%3lu %-11s Pri=%2d %-11s KernelStack=%4zu/%4zu Run=%d Ideal=%d (%d) Affinity=%016lx (%016lx)\n",
                               thread->GetId(), process->GetId(), process->GetName(), thread->GetPriority(), ThreadStates[thread->GetState()],
                               thread->GetKernelStackUsage(), PageSize, thread->GetActiveCore(), thread->GetIdealVirtualCore(), thread->GetIdealPhysicalCore(),
                               thread->GetVirtualAffinityMask(), thread->GetAffinityMask().GetAffinityMask());

                MESOSPHERE_RELEASE_LOG("    State: 0x%04x Suspend: 0x%04x Dpc: 0x%x\n", thread->GetRawState(), thread->GetSuspendFlags(), thread->GetDpc());

                MESOSPHERE_RELEASE_LOG("    TLS: %p (%p)\n", GetVoidPointer(thread->GetThreadLocalRegionAddress()), thread->GetThreadLocalRegionHeapAddress());
            } else {
                MESOSPHERE_RELEASE_LOG("Thread ID=%5lu pid=%3d %-11s Pri=%2d %-11s KernelStack=%4zu/%4zu Run=%d Ideal=%d (%d) Affinity=%016lx (%016lx)\n",
                               thread->GetId(), -1, "(kernel)", thread->GetPriority(), ThreadStates[thread->GetState()],
                               thread->GetKernelStackUsage(), PageSize, thread->GetActiveCore(), thread->GetIdealVirtualCore(), thread->GetIdealPhysicalCore(),
                               thread->GetVirtualAffinityMask(), thread->GetAffinityMask().GetAffinityMask());
            }
        }

        void DumpThreadCallStack(KThread *thread) {
            if (KProcess *process = thread->GetOwnerProcess(); process != nullptr) {
                MESOSPHERE_RELEASE_LOG("Thread ID=%5lu pid=%3lu %-11s Pri=%2d %-11s KernelStack=%4zu/%4zu\n",
                               thread->GetId(), process->GetId(), process->GetName(), thread->GetPriority(), ThreadStates[thread->GetState()], thread->GetKernelStackUsage(), PageSize);

                KDebug::PrintRegister(thread);
                KDebug::PrintBacktrace(thread);
            } else {
                MESOSPHERE_RELEASE_LOG("Thread ID=%5lu pid=%3d %-11s Pri=%2d %-11s KernelStack=%4zu/%4zu\n",
                               thread->GetId(), -1, "(kernel)", thread->GetPriority(), ThreadStates[thread->GetState()], thread->GetKernelStackUsage(), PageSize);
            }
        }

        void DumpHandle(const KProcess::ListAccessor &accessor, KProcess *process) {
            MESOSPHERE_RELEASE_LOG("Process ID=%lu (%s)\n", process->GetId(), process->GetName());

            const auto end = accessor.end();
            const auto &handle_table = process->GetHandleTable();
            const size_t max_handles = handle_table.GetMaxCount();
            for (size_t i = 0; i < max_handles; ++i) {
                /* Get the object + handle. */
                ams::svc::Handle handle = ams::svc::InvalidHandle;
                KScopedAutoObject obj = handle_table.GetObjectByIndex(std::addressof(handle), i);
                if (obj.IsNotNull()) {
                    if (auto *target = obj->DynamicCast<KServerSession *>(); target != nullptr) {
                        MESOSPHERE_RELEASE_LOG("Handle %08x Obj=%p Ref=%d Type=%s Client=%p\n", handle, obj.GetPointerUnsafe(), obj->GetReferenceCount() - 1, obj->GetTypeName(), std::addressof(target->GetParent()->GetClientSession()));
                        target->Dump();
                    } else if (auto *target = obj->DynamicCast<KClientSession *>(); target != nullptr) {
                        MESOSPHERE_RELEASE_LOG("Handle %08x Obj=%p Ref=%d Type=%s Server=%p\n", handle, obj.GetPointerUnsafe(), obj->GetReferenceCount() - 1, obj->GetTypeName(), std::addressof(target->GetParent()->GetServerSession()));
                    } else if (auto *target = obj->DynamicCast<KThread *>(); target != nullptr) {
                        KProcess *target_owner = target->GetOwnerProcess();
                        const s32 owner_pid = target_owner != nullptr ? static_cast<s32>(target_owner->GetId()) : -1;
                        MESOSPHERE_RELEASE_LOG("Handle %08x Obj=%p Ref=%d Type=%s ID=%d PID=%d\n", handle, obj.GetPointerUnsafe(), obj->GetReferenceCount() - 1, obj->GetTypeName(), static_cast<s32>(target->GetId()), owner_pid);
                    } else if (auto *target = obj->DynamicCast<KProcess *>(); target != nullptr) {
                        MESOSPHERE_RELEASE_LOG("Handle %08x Obj=%p Ref=%d Type=%s ID=%d\n", handle, obj.GetPointerUnsafe(), obj->GetReferenceCount() - 1, obj->GetTypeName(), static_cast<s32>(target->GetId()));
                    } else if (auto *target = obj->DynamicCast<KSharedMemory *>(); target != nullptr) {
                        /* Find the owner. */
                        KProcess *target_owner = nullptr;
                        for (auto it = accessor.begin(); it != end; ++it) {
                            if (static_cast<KProcess *>(std::addressof(*it))->GetId() == target->GetOwnerProcessId()) {
                                target_owner = static_cast<KProcess *>(std::addressof(*it));
                                break;
                            }
                        }

                        MESOSPHERE_ASSERT(target_owner != nullptr);
                        MESOSPHERE_RELEASE_LOG("Handle %08x Obj=%p Ref=%d Type=%s Size=%zu KB OwnerPID=%d (%s)\n", handle, obj.GetPointerUnsafe(), obj->GetReferenceCount() - 1, obj->GetTypeName(), target->GetSize() / 1_KB, static_cast<s32>(target_owner->GetId()), target_owner->GetName());
                    } else if (auto *target = obj->DynamicCast<KTransferMemory *>(); target != nullptr) {
                        KProcess *target_owner = target->GetOwner();
                        MESOSPHERE_RELEASE_LOG("Handle %08x Obj=%p Ref=%d Type=%s OwnerPID=%d (%s) OwnerAddress=%lx Size=%zu KB\n", handle, obj.GetPointerUnsafe(), obj->GetReferenceCount() - 1, obj->GetTypeName(), static_cast<s32>(target_owner->GetId()), target_owner->GetName(), GetInteger(target->GetSourceAddress()), target->GetSize() / 1_KB);
                    } else if (auto *target = obj->DynamicCast<KCodeMemory *>(); target != nullptr) {
                        KProcess *target_owner = target->GetOwner();
                        MESOSPHERE_RELEASE_LOG("Handle %08x Obj=%p Ref=%d Type=%s OwnerPID=%d (%s) OwnerAddress=%lx Size=%zu KB\n", handle, obj.GetPointerUnsafe(), obj->GetReferenceCount() - 1, obj->GetTypeName(), static_cast<s32>(target_owner->GetId()), target_owner->GetName(), GetInteger(target->GetSourceAddress()), target->GetSize() / 1_KB);
                    } else if (auto *target = obj->DynamicCast<KInterruptEvent *>(); target != nullptr) {
                        MESOSPHERE_RELEASE_LOG("Handle %08x Obj=%p Ref=%d Type=%s irq=%d\n", handle, obj.GetPointerUnsafe(), obj->GetReferenceCount() - 1, obj->GetTypeName(), target->GetInterruptId());
                    } else if (auto *target = obj->DynamicCast<KWritableEvent *>(); target != nullptr) {
                        if (KEvent *event = target->GetParent(); event != nullptr) {
                            MESOSPHERE_RELEASE_LOG("Handle %08x Obj=%p Ref=%d Type=%s Pair=%p\n", handle, obj.GetPointerUnsafe(), obj->GetReferenceCount() - 1, obj->GetTypeName(), std::addressof(event->GetReadableEvent()));
                        } else {
                            MESOSPHERE_RELEASE_LOG("Handle %08x Obj=%p Ref=%d Type=%s\n", handle, obj.GetPointerUnsafe(), obj->GetReferenceCount() - 1, obj->GetTypeName());
                        }
                    } else if (auto *target = obj->DynamicCast<KReadableEvent *>(); target != nullptr) {
                        if (KEvent *event = target->GetParent(); event != nullptr) {
                            MESOSPHERE_RELEASE_LOG("Handle %08x Obj=%p Ref=%d Type=%s Pair=%p\n", handle, obj.GetPointerUnsafe(), obj->GetReferenceCount() - 1, obj->GetTypeName(), std::addressof(event->GetWritableEvent()));
                        } else {
                            MESOSPHERE_RELEASE_LOG("Handle %08x Obj=%p Ref=%d Type=%s\n", handle, obj.GetPointerUnsafe(), obj->GetReferenceCount() - 1, obj->GetTypeName());
                        }
                    } else {
                        MESOSPHERE_RELEASE_LOG("Handle %08x Obj=%p Ref=%d Type=%s\n", handle, obj.GetPointerUnsafe(), obj->GetReferenceCount() - 1, obj->GetTypeName());
                    }

                    if (auto *sync = obj->DynamicCast<KSynchronizationObject *>(); sync != nullptr) {
                        sync->DumpWaiters();
                    }
                }
            }

            MESOSPHERE_RELEASE_LOG("%zu(max %zu)/%zu used.\n", handle_table.GetCount(), max_handles, handle_table.GetTableSize());
            MESOSPHERE_RELEASE_LOG("\n\n");
        }

        void DumpMemory(KProcess *process) {
            const auto process_id = process->GetId();
            MESOSPHERE_RELEASE_LOG("Process ID=%3lu (%s)\n", process_id, process->GetName());

            /* Dump the memory blocks. */
            process->GetPageTable().DumpMemoryBlocks();

            /* Collect information about memory totals. */
            const size_t code            = process->GetPageTable().GetCodeSize();
            const size_t code_data       = process->GetPageTable().GetCodeDataSize();
            const size_t alias_code      = process->GetPageTable().GetAliasCodeSize();
            const size_t alias_code_data = process->GetPageTable().GetAliasCodeDataSize();
            const size_t normal          = process->GetPageTable().GetNormalMemorySize();
            const size_t main_stack      = process->GetMainStackSize();

            size_t shared = 0;
            {
                KSharedMemory::ListAccessor accessor;
                const auto end = accessor.end();
                for (auto it = accessor.begin(); it != end; ++it) {
                    KSharedMemory *shared_mem = static_cast<KSharedMemory *>(std::addressof(*it));
                    if (shared_mem->GetOwnerProcessId() == process_id) {
                        shared += shared_mem->GetSize();
                    }
                }
            }

            /* Dump the totals. */
            MESOSPHERE_RELEASE_LOG("---\n");
            MESOSPHERE_RELEASE_LOG("Code          %8zu KB\n", code / 1_KB);
            MESOSPHERE_RELEASE_LOG("CodeData      %8zu KB\n", code_data / 1_KB);
            MESOSPHERE_RELEASE_LOG("AliasCode     %8zu KB\n", alias_code / 1_KB);
            MESOSPHERE_RELEASE_LOG("AliasCodeData %8zu KB\n", alias_code_data / 1_KB);
            MESOSPHERE_RELEASE_LOG("Heap          %8zu KB\n", normal / 1_KB);
            MESOSPHERE_RELEASE_LOG("SharedMemory  %8zu KB\n", shared / 1_KB);
            MESOSPHERE_RELEASE_LOG("InitialStack  %8zu KB\n", main_stack / 1_KB);
            MESOSPHERE_RELEASE_LOG("---\n");
            MESOSPHERE_RELEASE_LOG("TOTAL         %8zu KB\n", (code + code_data + alias_code + alias_code_data + normal + main_stack + shared) / 1_KB);
            MESOSPHERE_RELEASE_LOG("\n\n");
        }

        void DumpPageTable(KProcess *process) {
            MESOSPHERE_RELEASE_LOG("Process ID=%3lu (%s)\n", process->GetId(), process->GetName());
            process->GetPageTable().DumpPageTable();
            MESOSPHERE_RELEASE_LOG("\n\n");
        }

        void DumpProcess(KProcess *process) {
            MESOSPHERE_RELEASE_LOG("Process ID=%3lu index=%3zu State=%d (%s)\n", process->GetId(), process->GetSlabIndex(), process->GetState(), process->GetName());
        }

        void DumpPort(const KProcess::ListAccessor &accessor, KProcess *process) {
            MESOSPHERE_RELEASE_LOG("Dump Port Process ID=%lu (%s)\n", process->GetId(), process->GetName());

            const auto end = accessor.end();
            const auto &handle_table = process->GetHandleTable();
            const size_t max_handles = handle_table.GetMaxCount();
            for (size_t i = 0; i < max_handles; ++i) {
                /* Get the object + handle. */
                ams::svc::Handle handle = ams::svc::InvalidHandle;
                KScopedAutoObject obj = handle_table.GetObjectByIndex(std::addressof(handle), i);
                if (obj.IsNull()) {
                    continue;
                }

                /* Process the object as a port. */
                if (auto *server = obj->DynamicCast<KServerPort *>(); server != nullptr) {
                    const KClientPort *client = std::addressof(server->GetParent()->GetClientPort());
                    const uintptr_t port_name = server->GetParent()->GetName();

                    /* Get the port name. */
                    char name[9] = {};
                    {
                        /* Find the client port process. */
                        KScopedAutoObject<KProcess> client_port_process;
                        {
                            for (auto it = accessor.begin(); it != end && client_port_process.IsNull(); ++it) {
                                KProcess *cur = static_cast<KProcess *>(std::addressof(*it));
                                for (size_t j = 0; j < cur->GetHandleTable().GetMaxCount(); ++j) {
                                    ams::svc::Handle cur_h  = ams::svc::InvalidHandle;
                                    KScopedAutoObject cur_o = cur->GetHandleTable().GetObjectByIndex(std::addressof(cur_h), j);
                                    if (cur_o.IsNotNull()) {
                                        if (cur_o.GetPointerUnsafe() == client) {
                                            client_port_process = cur;
                                            break;
                                        }
                                    }
                                }
                            }
                        }

                        /* Read the port name. */
                        if (client_port_process.IsNotNull()) {
                            if (R_FAILED(client_port_process->GetPageTable().CopyMemoryFromLinearToKernel(KProcessAddress(name), 8, port_name, KMemoryState_None, KMemoryState_None, KMemoryPermission_UserRead, KMemoryAttribute_None, KMemoryAttribute_None))) {
                                std::memset(name, 0, sizeof(name));
                            }
                            for (size_t i = 0; i < 8 && name[i] != 0; i++) {
                                if (name[i] > 0x7F) {
                                    std::memset(name, 0, sizeof(name));
                                    break;
                                }
                            }
                        }
                    }

                    MESOSPHERE_RELEASE_LOG("%-9s: Handle %08x Obj=%p Cur=%3d Peak=%3d Max=%3d\n", name, handle, obj.GetPointerUnsafe(), client->GetNumSessions(), client->GetPeakSessions(), client->GetMaxSessions());

                    /* Identify any sessions. */
                    {
                        for (auto it = accessor.begin(); it != end; ++it) {
                            KProcess *cur = static_cast<KProcess *>(std::addressof(*it));
                            for (size_t j = 0; j < cur->GetHandleTable().GetMaxCount(); ++j) {
                                ams::svc::Handle cur_h  = ams::svc::InvalidHandle;
                                KScopedAutoObject cur_o = cur->GetHandleTable().GetObjectByIndex(std::addressof(cur_h), j);
                                if (cur_o.IsNull()) {
                                    continue;
                                }
                                if (auto *session = cur_o->DynamicCast<KClientSession *>(); session != nullptr && session->GetParent()->GetParent() == client) {
                                    MESOSPHERE_RELEASE_LOG("    Client %p Server %p %-12s: PID=%3lu\n", session, std::addressof(session->GetParent()->GetServerSession()), cur->GetName(), cur->GetId());
                                }
                            }
                        }
                    }
                }
            }
        }

        ALWAYS_INLINE s64 GetTickOrdered() {
            __asm__ __volatile__("" ::: "memory");
            const s64 tick = KHardwareTimer::GetTick();
            __asm__ __volatile__("" ::: "memory");
            return tick;
        }

    }

    void DumpThread() {
        MESOSPHERE_RELEASE_LOG("Dump Thread\n");

        {
            /* Lock the list. */
            KThread::ListAccessor accessor;
            const auto end = accessor.end();

            /* Dump each thread. */
            for (auto it = accessor.begin(); it != end; ++it) {
                DumpThread(static_cast<KThread *>(std::addressof(*it)));
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpThread(u64 thread_id) {
        MESOSPHERE_RELEASE_LOG("Dump Thread\n");

        {
            /* Find and dump the target thread. */
            if (KThread *thread = KThread::GetThreadFromId(thread_id); thread != nullptr) {
                ON_SCOPE_EXIT { thread->Close(); };
                DumpThread(thread);
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpThreadCallStack() {
        MESOSPHERE_RELEASE_LOG("Dump Thread\n");

        {
            /* Lock the list. */
            KThread::ListAccessor accessor;
            const auto end = accessor.end();

            /* Dump each thread. */
            for (auto it = accessor.begin(); it != end; ++it) {
                DumpThreadCallStack(static_cast<KThread *>(std::addressof(*it)));
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpThreadCallStack(u64 thread_id) {
        MESOSPHERE_RELEASE_LOG("Dump Thread\n");

        {
            /* Find and dump the target thread. */
            if (KThread *thread = KThread::GetThreadFromId(thread_id); thread != nullptr) {
                ON_SCOPE_EXIT { thread->Close(); };
                DumpThreadCallStack(thread);
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpKernelObject() {
        MESOSPHERE_LOG("Dump Kernel Object\n");

        {
            /* Static slab heaps. */
            {
                #define DUMP_KSLABOBJ(__OBJECT__)                                                                                                                                                         \
                    MESOSPHERE_RELEASE_LOG(#__OBJECT__ "\n");                                                                                                                                             \
                    MESOSPHERE_RELEASE_LOG("    Cur=%3zu Peak=%3zu Max=%3zu\n", __OBJECT__::GetSlabHeapSize() - __OBJECT__::GetNumRemaining(), __OBJECT__::GetPeakIndex(), __OBJECT__::GetSlabHeapSize())

                DUMP_KSLABOBJ(KPageBuffer);
                DUMP_KSLABOBJ(KEvent);
                DUMP_KSLABOBJ(KInterruptEvent);
                DUMP_KSLABOBJ(KProcess);
                DUMP_KSLABOBJ(KThread);
                DUMP_KSLABOBJ(KPort);
                DUMP_KSLABOBJ(KSharedMemory);
                DUMP_KSLABOBJ(KTransferMemory);
                DUMP_KSLABOBJ(KDeviceAddressSpace);
                DUMP_KSLABOBJ(KDebug);
                DUMP_KSLABOBJ(KSession);
                DUMP_KSLABOBJ(KLightSession);
                DUMP_KSLABOBJ(KLinkedListNode);
                DUMP_KSLABOBJ(KThreadLocalPage);
                DUMP_KSLABOBJ(KObjectName);
                DUMP_KSLABOBJ(KEventInfo);
                DUMP_KSLABOBJ(KSessionRequest);
                DUMP_KSLABOBJ(KResourceLimit);
                DUMP_KSLABOBJ(KAlpha);
                DUMP_KSLABOBJ(KBeta);

                #undef DUMP_KSLABOBJ

            }
            MESOSPHERE_RELEASE_LOG("\n");

            /* Dynamic slab heaps. */
            {
                /* Memory block slabs. */
                {
                    MESOSPHERE_RELEASE_LOG("App Memory Block\n");
                    auto &app = Kernel::GetApplicationMemoryBlockManager();
                    MESOSPHERE_RELEASE_LOG("    Cur=%6zu Peak=%6zu Max=%6zu\n", app.GetUsed(), app.GetPeak(), app.GetCount());
                    MESOSPHERE_RELEASE_LOG("Sys Memory Block\n");
                    auto &sys = Kernel::GetSystemMemoryBlockManager();
                    MESOSPHERE_RELEASE_LOG("    Cur=%6zu Peak=%6zu Max=%6zu\n", sys.GetUsed(), sys.GetPeak(), sys.GetCount());
                }

                /* KBlockInfo slab. */
                {
                    MESOSPHERE_RELEASE_LOG("KBlockInfo\n");
                    auto &manager = Kernel::GetBlockInfoManager();
                    MESOSPHERE_RELEASE_LOG("    Cur=%6zu Peak=%6zu Max=%6zu\n", manager.GetUsed(), manager.GetPeak(), manager.GetCount());
                }

                /* Page Table slab. */
                {
                    MESOSPHERE_RELEASE_LOG("Page Table\n");
                    auto &manager = Kernel::GetPageTableManager();
                    MESOSPHERE_RELEASE_LOG("    Cur=%6zu Peak=%6zu Max=%6zu\n", manager.GetUsed(), manager.GetPeak(), manager.GetCount());
                }
            }
            MESOSPHERE_RELEASE_LOG("\n");

            /* Process resources. */
            {
                KProcess::ListAccessor accessor;

                size_t process_pts = 0;

                const auto end = accessor.end();
                for (auto it = accessor.begin(); it != end; ++it) {
                    KProcess *process = static_cast<KProcess *>(std::addressof(*it));

                    /* Count the number of threads. */
                    int threads = 0;
                    {
                        KThread::ListAccessor thr_accessor;
                        const auto thr_end = thr_accessor.end();
                        for (auto thr_it = thr_accessor.begin(); thr_it != thr_end; ++thr_it) {
                            KThread *thread = static_cast<KThread *>(std::addressof(*thr_it));
                            if (thread->GetOwnerProcess() == process) {
                                ++threads;
                            }
                        }
                    }

                    /* Count the number of events. */
                    int events = 0;
                    {
                        KEvent::ListAccessor ev_accessor;
                        const auto ev_end = ev_accessor.end();
                        for (auto ev_it = ev_accessor.begin(); ev_it != ev_end; ++ev_it) {
                            KEvent *event = static_cast<KEvent *>(std::addressof(*ev_it));
                            if (event->GetOwner() == process) {
                                ++events;
                            }
                        }
                    }

                    size_t pts = process->GetPageTable().CountPageTables();
                    process_pts += pts;

                    MESOSPHERE_RELEASE_LOG("%-12s: PID=%3lu Thread %4d / Event %4d / PageTable %5zu\n", process->GetName(), process->GetId(), threads, events, pts);
                    if (process->GetTotalSystemResourceSize() != 0) {
                        MESOSPHERE_RELEASE_LOG("    System Resource\n");
                        MESOSPHERE_RELEASE_LOG("        Cur=%6zu Peak=%6zu Max=%6zu\n", process->GetDynamicPageManager().GetUsed(), process->GetDynamicPageManager().GetPeak(), process->GetDynamicPageManager().GetCount());
                        MESOSPHERE_RELEASE_LOG("    Memory Block\n");
                        MESOSPHERE_RELEASE_LOG("        Cur=%6zu Peak=%6zu Max=%6zu\n", process->GetMemoryBlockSlabManager().GetUsed(), process->GetMemoryBlockSlabManager().GetPeak(), process->GetMemoryBlockSlabManager().GetCount());
                        MESOSPHERE_RELEASE_LOG("    Page Table\n");
                        MESOSPHERE_RELEASE_LOG("        Cur=%6zu Peak=%6zu Max=%6zu\n", process->GetPageTableManager().GetUsed(), process->GetPageTableManager().GetPeak(), process->GetPageTableManager().GetCount());
                        MESOSPHERE_RELEASE_LOG("    Block Info\n");
                        MESOSPHERE_RELEASE_LOG("        Cur=%6zu Peak=%6zu Max=%6zu\n", process->GetBlockInfoManager().GetUsed(), process->GetBlockInfoManager().GetPeak(), process->GetBlockInfoManager().GetCount());
                    }
                }

                MESOSPHERE_RELEASE_LOG("Process Page Table %zu\n", process_pts);
                MESOSPHERE_RELEASE_LOG("Kernel Page Table  %zu\n", Kernel::GetKernelPageTable().CountPageTables());
            }
            MESOSPHERE_RELEASE_LOG("\n");

            /* Resource limits. */
            {
                auto &sys_rl = Kernel::GetSystemResourceLimit();

                u64 cur = sys_rl.GetCurrentValue(ams::svc::LimitableResource_PhysicalMemoryMax);
                u64 lim = sys_rl.GetLimitValue(ams::svc::LimitableResource_PhysicalMemoryMax);
                MESOSPHERE_RELEASE_LOG("System ResourceLimit PhysicalMemory 0x%01x_%08x / 0x%01x_%08x\n", static_cast<u32>(cur >> 32), static_cast<u32>(cur), static_cast<u32>(lim >> 32), static_cast<u32>(lim));

                cur = sys_rl.GetCurrentValue(ams::svc::LimitableResource_ThreadCountMax);
                lim = sys_rl.GetLimitValue(ams::svc::LimitableResource_ThreadCountMax);
                MESOSPHERE_RELEASE_LOG("System ResourceLimit Thread         %4lu / %4lu\n", cur, lim);

                cur = sys_rl.GetCurrentValue(ams::svc::LimitableResource_EventCountMax);
                lim = sys_rl.GetLimitValue(ams::svc::LimitableResource_EventCountMax);
                MESOSPHERE_RELEASE_LOG("System ResourceLimit Event          %4lu / %4lu\n", cur, lim);

                cur = sys_rl.GetCurrentValue(ams::svc::LimitableResource_TransferMemoryCountMax);
                lim = sys_rl.GetLimitValue(ams::svc::LimitableResource_TransferMemoryCountMax);
                MESOSPHERE_RELEASE_LOG("System ResourceLimit TransferMemory %4lu / %4lu\n", cur, lim);

                cur = sys_rl.GetCurrentValue(ams::svc::LimitableResource_SessionCountMax);
                lim = sys_rl.GetLimitValue(ams::svc::LimitableResource_SessionCountMax);
                MESOSPHERE_RELEASE_LOG("System ResourceLimit Session        %4lu / %4lu\n", cur, lim);

                {
                    KResourceLimit::ListAccessor accessor;

                    const auto end = accessor.end();
                    for (auto it = accessor.begin(); it != end; ++it) {
                        KResourceLimit *rl = static_cast<KResourceLimit *>(std::addressof(*it));
                        cur = rl->GetCurrentValue(ams::svc::LimitableResource_PhysicalMemoryMax);
                        lim = rl->GetLimitValue(ams::svc::LimitableResource_PhysicalMemoryMax);
                        MESOSPHERE_RELEASE_LOG("ResourceLimit %zu PhysicalMemory 0x%01x_%08x / 0x%01x_%08x\n", rl->GetSlabIndex(), static_cast<u32>(cur >> 32), static_cast<u32>(cur), static_cast<u32>(lim >> 32), static_cast<u32>(lim));
                    }
                }
            }
            MESOSPHERE_RELEASE_LOG("\n");

            /* Memory Manager. */
            {
                auto &mm = Kernel::GetMemoryManager();
                u64 max = mm.GetSize();
                u64 cur = max - mm.GetFreeSize();
                MESOSPHERE_RELEASE_LOG("Kernel Heap Size 0x%01x_%08x / 0x%01x_%08x\n", static_cast<u32>(cur >> 32), static_cast<u32>(cur), static_cast<u32>(max >> 32), static_cast<u32>(max));
                MESOSPHERE_RELEASE_LOG("\n");

                max = mm.GetSize(KMemoryManager::Pool_Application);
                cur = max - mm.GetFreeSize(KMemoryManager::Pool_Application);
                MESOSPHERE_RELEASE_LOG("Application      0x%01x_%08x / 0x%01x_%08x\n", static_cast<u32>(cur >> 32), static_cast<u32>(cur), static_cast<u32>(max >> 32), static_cast<u32>(max));
                mm.DumpFreeList(KMemoryManager::Pool_Application);
                MESOSPHERE_RELEASE_LOG("\n");

                max = mm.GetSize(KMemoryManager::Pool_Applet);
                cur = max - mm.GetFreeSize(KMemoryManager::Pool_Applet);
                MESOSPHERE_RELEASE_LOG("Applet           0x%01x_%08x / 0x%01x_%08x\n", static_cast<u32>(cur >> 32), static_cast<u32>(cur), static_cast<u32>(max >> 32), static_cast<u32>(max));
                mm.DumpFreeList(KMemoryManager::Pool_Applet);
                MESOSPHERE_RELEASE_LOG("\n");

                max = mm.GetSize(KMemoryManager::Pool_System);
                cur = max - mm.GetFreeSize(KMemoryManager::Pool_System);
                MESOSPHERE_RELEASE_LOG("System           0x%01x_%08x / 0x%01x_%08x\n", static_cast<u32>(cur >> 32), static_cast<u32>(cur), static_cast<u32>(max >> 32), static_cast<u32>(max));
                mm.DumpFreeList(KMemoryManager::Pool_System);
                MESOSPHERE_RELEASE_LOG("\n");

                max = mm.GetSize(KMemoryManager::Pool_SystemNonSecure);
                cur = max - mm.GetFreeSize(KMemoryManager::Pool_SystemNonSecure);
                MESOSPHERE_RELEASE_LOG("SystemNonSecure  0x%01x_%08x / 0x%01x_%08x\n", static_cast<u32>(cur >> 32), static_cast<u32>(cur), static_cast<u32>(max >> 32), static_cast<u32>(max));
                mm.DumpFreeList(KMemoryManager::Pool_SystemNonSecure);
                MESOSPHERE_RELEASE_LOG("\n");
            }
            MESOSPHERE_RELEASE_LOG("\n");

        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpHandle() {
        MESOSPHERE_RELEASE_LOG("Dump Handle\n");

        {
            /* Lock the list. */
            KProcess::ListAccessor accessor;
            const auto end = accessor.end();

            /* Dump each process. */
            for (auto it = accessor.begin(); it != end; ++it) {
                DumpHandle(accessor, static_cast<KProcess *>(std::addressof(*it)));
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpHandle(u64 process_id) {
        MESOSPHERE_RELEASE_LOG("Dump Handle\n");

        {
            /* Find and dump the target process. */
            if (KProcess *process = KProcess::GetProcessFromId(process_id); process != nullptr) {
                ON_SCOPE_EXIT { process->Close(); };

                /* Lock the list. */
                KProcess::ListAccessor accessor;
                DumpHandle(accessor, process);
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpKernelMemory() {
        MESOSPHERE_RELEASE_LOG("Dump Kernel Memory Info\n");

        {
            Kernel::GetKernelPageTable().DumpMemoryBlocks();
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpMemory() {
        MESOSPHERE_RELEASE_LOG("Dump Memory Info\n");

        {
            /* Lock the list. */
            KProcess::ListAccessor accessor;
            const auto end = accessor.end();

            /* Dump each process. */
            for (auto it = accessor.begin(); it != end; ++it) {
                DumpMemory(static_cast<KProcess *>(std::addressof(*it)));
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpMemory(u64 process_id) {
        MESOSPHERE_RELEASE_LOG("Dump Memory Info\n");

        {
            /* Find and dump the target process. */
            if (KProcess *process = KProcess::GetProcessFromId(process_id); process != nullptr) {
                ON_SCOPE_EXIT { process->Close(); };
                DumpMemory(process);
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpProcess() {
        MESOSPHERE_RELEASE_LOG("Dump Process\n");

        {
            /* Lock the list. */
            KProcess::ListAccessor accessor;
            const auto end = accessor.end();

            /* Dump each process. */
            for (auto it = accessor.begin(); it != end; ++it) {
                DumpProcess(static_cast<KProcess *>(std::addressof(*it)));
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpKernelPageTable() {
        MESOSPHERE_RELEASE_LOG("Dump Kernel PageTable\n");

        {
            Kernel::GetKernelPageTable().DumpPageTable();
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpPageTable() {
        MESOSPHERE_RELEASE_LOG("Dump Process\n");

        {
            /* Lock the list. */
            KProcess::ListAccessor accessor;
            const auto end = accessor.end();

            /* Dump each process. */
            for (auto it = accessor.begin(); it != end; ++it) {
                DumpPageTable(static_cast<KProcess *>(std::addressof(*it)));
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpPageTable(u64 process_id) {
        MESOSPHERE_RELEASE_LOG("Dump PageTable\n");

        {
            /* Find and dump the target process. */
            if (KProcess *process = KProcess::GetProcessFromId(process_id); process != nullptr) {
                ON_SCOPE_EXIT { process->Close(); };
                DumpPageTable(process);
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpKernelCpuUtilization() {
        MESOSPHERE_RELEASE_LOG("Dump Kernel Cpu Utilization\n");

        constexpr size_t MaxObjects = 64;
        {
            /* Create tracking arrays. */
            KAutoObject *objects[MaxObjects];
            u32 cpu_time[MaxObjects];

            s64 start_tick;
            size_t i, n;
            KDpcManager::Sync();
            {
                /* Lock the thread list. */
                KThread::ListAccessor accessor;

                /* Begin tracking. */
                start_tick = GetTickOrdered();

                /* Iterate, finding kernel threads. */
                const auto end = accessor.end();
                i = 0;
                for (auto it = accessor.begin(); it != end; ++it) {
                    KThread *thread = static_cast<KThread *>(std::addressof(*it));
                    if (KProcess *process = thread->GetOwnerProcess(); process == nullptr) {
                        if (AMS_LIKELY(i < MaxObjects)) {
                            if (AMS_LIKELY(thread->Open())) {
                                cpu_time[i] = thread->GetCpuTime();
                                objects[i]  = thread;
                                ++i;
                            }
                        }
                    }
                }

                /* Keep track of how many kernel threads we found. */
                n = i;
            }

            /* Wait one second. */
            const s64 timeout = KHardwareTimer::GetTick() + ams::svc::Tick(TimeSpan::FromSeconds(1));
            GetCurrentThread().Sleep(timeout);
            KDpcManager::Sync();

            /* Update our metrics. */
            for (i = 0; i < n; ++i) {
                KThread *thread = static_cast<KThread *>(objects[i]);
                cpu_time[i] = thread->GetCpuTime() - cpu_time[i];
            }

            /* End tracking. */
            const s64 end_tick = GetTickOrdered();

            /* Log thread utilization. */
            for (i = 0; i < n; ++i) {
                KThread *thread = static_cast<KThread *>(objects[i]);
                const s64 t = static_cast<u64>(cpu_time[i]) * 1000 / (end_tick - start_tick);

                MESOSPHERE_RELEASE_LOG("tid=%3lu (kernel) %3lu.%lu%% pri=%2d af=%lx\n", thread->GetId(), t / 10, t % 10, thread->GetPriority(), thread->GetAffinityMask().GetAffinityMask());
            }

            /* Close all objects. */
            for (i = 0; i < n; ++i) {
                objects[i]->Close();
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpCpuUtilization() {
        MESOSPHERE_RELEASE_LOG("Dump Cpu Utilization\n");

        /* NOTE: Nintendo uses 0x40 as maximum here, but the KProcess slabheap has 0x50 entries. */
        /*       We have the stack space, so there's no reason not to allow logging all processes. */
        constexpr size_t MaxObjects = 0x50;
        {
            /* Create tracking arrays. */
            KAutoObject *objects[MaxObjects];
            u32 cpu_time[MaxObjects];

            s64 start_tick;
            size_t i, n;
            KDpcManager::Sync();
            {
                /* Lock the process list. */
                KProcess::ListAccessor accessor;

                /* Begin tracking. */
                start_tick = GetTickOrdered();

                /* Iterate, finding processes. */
                const auto end = accessor.end();
                i = 0;
                for (auto it = accessor.begin(); it != end; ++it) {
                    KProcess *process = static_cast<KProcess *>(std::addressof(*it));
                    if (AMS_LIKELY(i < MaxObjects)) {
                        if (AMS_LIKELY(process->Open())) {
                            cpu_time[i] = process->GetCpuTime();
                            objects[i]  = process;
                            ++i;
                        }
                    }
                }

                /* Keep track of how many processes we found. */
                n = i;
            }

            /* Wait one second. */
            const s64 timeout = KHardwareTimer::GetTick() + ams::svc::Tick(TimeSpan::FromSeconds(1));
            GetCurrentThread().Sleep(timeout);
            KDpcManager::Sync();

            /* Update our metrics. */
            for (i = 0; i < n; ++i) {
                KProcess *process = static_cast<KProcess *>(objects[i]);
                cpu_time[i] = process->GetCpuTime() - cpu_time[i];
            }

            /* End tracking. */
            const s64 end_tick = GetTickOrdered();

            /* Log process utilization. */
            for (i = 0; i < n; ++i) {
                KProcess *process = static_cast<KProcess *>(objects[i]);
                const s64 t = static_cast<u64>(cpu_time[i]) * 1000 / (end_tick - start_tick);

                MESOSPHERE_RELEASE_LOG("pid=%3lu %-11s %3lu.%lu%%\n", process->GetId(), process->GetName(), t / 10, t % 10);
            }

            /* Close all objects. */
            for (i = 0; i < n; ++i) {
                objects[i]->Close();
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpCpuUtilization(u64 process_id) {
        MESOSPHERE_RELEASE_LOG("Dump Cpu Utilization\n");

        constexpr size_t MaxObjects = 64;
        {
            /* Create tracking arrays. */
            KAutoObject *objects[MaxObjects];
            u32 cpu_time[MaxObjects];

            s64 start_tick;
            size_t i, n;
            KDpcManager::Sync();
            {
                /* Lock the thread list. */
                KThread::ListAccessor accessor;

                /* Begin tracking. */
                start_tick = GetTickOrdered();

                /* Iterate, finding process threads. */
                const auto end = accessor.end();
                i = 0;
                for (auto it = accessor.begin(); it != end; ++it) {
                    KThread *thread = static_cast<KThread *>(std::addressof(*it));
                    if (KProcess *process = thread->GetOwnerProcess(); process != nullptr && process->GetId() == process_id) {
                        if (AMS_LIKELY(i < MaxObjects)) {
                            if (AMS_LIKELY(thread->Open())) {
                                cpu_time[i] = thread->GetCpuTime();
                                objects[i]  = thread;
                                ++i;
                            }
                        }
                    }
                }

                /* Keep track of how many process threads we found. */
                n = i;
            }

            /* Wait one second. */
            const s64 timeout = KHardwareTimer::GetTick() + ams::svc::Tick(TimeSpan::FromSeconds(1));
            GetCurrentThread().Sleep(timeout);
            KDpcManager::Sync();

            /* Update our metrics. */
            for (i = 0; i < n; ++i) {
                KThread *thread = static_cast<KThread *>(objects[i]);
                cpu_time[i] = thread->GetCpuTime() - cpu_time[i];
            }

            /* End tracking. */
            const s64 end_tick = GetTickOrdered();

            /* Log thread utilization. */
            for (i = 0; i < n; ++i) {
                KThread *thread   = static_cast<KThread *>(objects[i]);
                KProcess *process = thread->GetOwnerProcess();
                const s64 t = static_cast<u64>(cpu_time[i]) * 1000 / (end_tick - start_tick);

                MESOSPHERE_RELEASE_LOG("tid=%3lu pid=%3lu %-11s %3lu.%lu%% pri=%2d af=%lx\n", thread->GetId(), process->GetId(), process->GetName(), t / 10, t % 10, thread->GetPriority(), thread->GetAffinityMask().GetAffinityMask());
            }

            /* Close all objects. */
            for (i = 0; i < n; ++i) {
                objects[i]->Close();
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpProcess(u64 process_id) {
        MESOSPHERE_RELEASE_LOG("Dump Process\n");

        {
            /* Find and dump the target process. */
            if (KProcess *process = KProcess::GetProcessFromId(process_id); process != nullptr) {
                ON_SCOPE_EXIT { process->Close(); };
                DumpProcess(process);
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpPort() {
        MESOSPHERE_RELEASE_LOG("Dump Port\n");

        {
            /* Lock the list. */
            KProcess::ListAccessor accessor;
            const auto end = accessor.end();

            /* Dump each process. */
            for (auto it = accessor.begin(); it != end; ++it) {
                DumpPort(accessor, static_cast<KProcess *>(std::addressof(*it)));
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

    void DumpPort(u64 process_id) {
        MESOSPHERE_RELEASE_LOG("Dump Port\n");

        {
            /* Find and dump the target process. */
            if (KProcess *process = KProcess::GetProcessFromId(process_id); process != nullptr) {
                ON_SCOPE_EXIT { process->Close(); };

                /* Lock the list. */
                KProcess::ListAccessor accessor;
                DumpPort(accessor, process);
            }
        }

        MESOSPHERE_RELEASE_LOG("\n");
    }

}
