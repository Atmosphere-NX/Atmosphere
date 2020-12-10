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
                        MESOSPHERE_RELEASE_LOG("%-9s: Handle %08x Obj=%p Cur=%3d Max=%3d Peak=%3d\n", name, handle, obj.GetPointerUnsafe(), client->GetNumSessions(), client->GetMaxSessions(), client->GetPeakSessions());

                        /* Identify any sessions. */
                        {
                            for (auto it = accessor.begin(); it != end && client_port_process.IsNull(); ++it) {
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
