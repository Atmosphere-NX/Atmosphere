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

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        void KernelDebug(ams::svc::KernelDebugType kern_debug_type, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
            MESOSPHERE_UNUSED(kern_debug_type, arg0, arg1, arg2);

            #ifdef MESOSPHERE_BUILD_FOR_DEBUGGING
            {
                switch (kern_debug_type) {
                    case ams::svc::KernelDebugType_Thread:
                        if (arg0 == static_cast<u64>(-1)) {
                            KDumpObject::DumpThread();
                        } else {
                            KDumpObject::DumpThread(arg0);
                        }
                        break;
                    case ams::svc::KernelDebugType_ThreadCallStack:
                        if (arg0 == static_cast<u64>(-1)) {
                            KDumpObject::DumpThreadCallStack();
                        } else {
                            KDumpObject::DumpThreadCallStack(arg0);
                        }
                        break;
                    case ams::svc::KernelDebugType_KernelObject:
                        KDumpObject::DumpKernelObject();
                        break;
                    case ams::svc::KernelDebugType_Handle:
                        if (arg0 == static_cast<u64>(-1)) {
                            KDumpObject::DumpHandle();
                        } else {
                            KDumpObject::DumpHandle(arg0);
                        }
                        break;
                    case ams::svc::KernelDebugType_Memory:
                        if (arg0 == static_cast<u64>(-2)) {
                            KDumpObject::DumpKernelMemory();
                        } else if (arg0 == static_cast<u64>(-1)) {
                            KDumpObject::DumpMemory();
                        } else {
                            KDumpObject::DumpMemory(arg0);
                        }
                        break;
                    case ams::svc::KernelDebugType_PageTable:
                        if (arg0 == static_cast<u64>(-2)) {
                            KDumpObject::DumpKernelPageTable();
                        } else if (arg0 == static_cast<u64>(-1)) {
                            KDumpObject::DumpPageTable();
                        } else {
                            KDumpObject::DumpPageTable(arg0);
                        }
                        break;
                    case ams::svc::KernelDebugType_CpuUtilization:
                        {
                            const auto old_prio = GetCurrentThread().GetBasePriority();
                            GetCurrentThread().SetBasePriority(3);

                            if (arg0 == static_cast<u64>(-2)) {
                                KDumpObject::DumpKernelCpuUtilization();
                            } else if (arg0 == static_cast<u64>(-1)) {
                                KDumpObject::DumpCpuUtilization();
                            } else {
                                KDumpObject::DumpCpuUtilization(arg0);
                            }

                            GetCurrentThread().SetBasePriority(old_prio);
                        }
                        break;
                    case ams::svc::KernelDebugType_Process:
                        if (arg0 == static_cast<u64>(-1)) {
                            KDumpObject::DumpProcess();
                        } else {
                            KDumpObject::DumpProcess(arg0);
                        }
                        break;
                    case ams::svc::KernelDebugType_SuspendProcess:
                        if (KProcess *process = KProcess::GetProcessFromId(arg0); process != nullptr) {
                            ON_SCOPE_EXIT { process->Close(); };

                            if (R_SUCCEEDED(process->SetActivity(ams::svc::ProcessActivity_Paused))) {
                                MESOSPHERE_RELEASE_LOG("Suspend Process ID=%3lu\n", process->GetId());
                            }
                        }
                        break;
                    case ams::svc::KernelDebugType_ResumeProcess:
                        if (KProcess *process = KProcess::GetProcessFromId(arg0); process != nullptr) {
                            ON_SCOPE_EXIT { process->Close(); };

                            if (R_SUCCEEDED(process->SetActivity(ams::svc::ProcessActivity_Runnable))) {
                                MESOSPHERE_RELEASE_LOG("Resume Process ID=%3lu\n", process->GetId());
                            }
                        }
                        break;
                    case ams::svc::KernelDebugType_Port:
                        if (arg0 == static_cast<u64>(-1)) {
                            KDumpObject::DumpPort();
                        } else {
                            KDumpObject::DumpPort(arg0);
                        }
                        break;
                    default:
                        break;
                }
            }
            #endif
        }

        void ChangeKernelTraceState(ams::svc::KernelTraceState kern_trace_state) {
            #ifdef MESOSPHERE_BUILD_FOR_DEBUGGING
            {
                switch (kern_trace_state) {
                    case ams::svc::KernelTraceState_Enabled:
                        {
                            MESOSPHERE_KTRACE_RESUME();
                        }
                        break;
                    case ams::svc::KernelTraceState_Disabled:
                        {
                            MESOSPHERE_KTRACE_PAUSE();
                        }
                        break;
                    default:
                        break;
                }
            }
            #else
            {
                MESOSPHERE_UNUSED(kern_trace_state);
            }
            #endif
        }

    }

    /* =============================    64 ABI    ============================= */

    void KernelDebug64(ams::svc::KernelDebugType kern_debug_type, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
        return KernelDebug(kern_debug_type, arg0, arg1, arg2);
    }

    void ChangeKernelTraceState64(ams::svc::KernelTraceState kern_trace_state) {
        return ChangeKernelTraceState(kern_trace_state);
    }

    /* ============================= 64From32 ABI ============================= */

    void KernelDebug64From32(ams::svc::KernelDebugType kern_debug_type, uint64_t arg0, uint64_t arg1, uint64_t arg2) {
        return KernelDebug(kern_debug_type, arg0, arg1, arg2);
    }

    void ChangeKernelTraceState64From32(ams::svc::KernelTraceState kern_trace_state) {
        return ChangeKernelTraceState(kern_trace_state);
    }

}
