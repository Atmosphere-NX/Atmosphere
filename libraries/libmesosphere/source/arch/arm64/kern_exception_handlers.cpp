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

namespace ams::kern::arch::arm64 {

    namespace {

        constexpr u32 GetInstructionData(const KExceptionContext *context, u64 esr) {
            /* Check for THUMB usermode */
            if ((context->psr & 0x3F) == 0x30) {
                u32 insn = *reinterpret_cast<u16 *>(context->pc & ~0x1);
                /* Check if the instruction was 32-bit. */
                if ((esr >> 25) & 1) {
                    insn = (insn << 16) | *reinterpret_cast<u16 *>((context->pc & ~0x1) + sizeof(u16));
                }
                return insn;
            } else {
                /* Not thumb, so just get the instruction. */
                return *reinterpret_cast<u32 *>(context->pc);
            }
        }

        void HandleUserException(KExceptionContext *context, u64 esr, u64 far, u64 afsr0, u64 afsr1, u32 data) {
            KProcess *cur_process = GetCurrentProcessPointer();
            bool should_process_user_exception = KTargetSystem::IsUserExceptionHandlersEnabled();

            const u64 ec = (esr >> 26) & 0x3F;
            switch (ec) {
                case 0x0:  /* Unknown */
                case 0xE:  /* Illegal Execution State */
                case 0x11: /* SVC instruction from Aarch32 */
                case 0x15: /* SVC instruction from Aarch64 */
                case 0x22: /* PC Misalignment */
                case 0x26: /* SP Misalignment */
                case 0x2F: /* SError */
                case 0x30: /* Breakpoint from lower EL */
                case 0x32: /* SoftwareStep from lower EL */
                case 0x34: /* Watchpoint from lower EL */
                case 0x38: /* BKPT instruction */
                case 0x3C: /* BRK instruction */
                    break;
                default:
                    {
                        MESOSPHERE_TODO("Get memory state.");
                        /* If state is KMemoryState_Code and the user can't read it, set should_process_user_exception = true; */
                    }
                    break;
            }

            if (should_process_user_exception) {
                MESOSPHERE_TODO("Process the user exception.");
            }

            {
                MESOSPHERE_TODO("Process for KDebug.");

                MESOSPHERE_TODO("cur_process->GetProgramId()");
                MESOSPHERE_RELEASE_LOG("Exception occurred. %016lx\n", 0ul);

                MESOSPHERE_TODO("if (!svc::ResultNotHandled::Includes(res)) { debug process }.");
            }

            MESOSPHERE_TODO("cur_process->Exit();");
            (void)cur_process;
        }

    }

    /* NOTE: This function is called from ASM. */
    void FpuContextSwitchHandler() {
        KThreadContext::FpuContextSwitchHandler(GetCurrentThreadPointer());
    }

    /* NOTE: This function is called from ASM. */
    void HandleException(KExceptionContext *context) {
        MESOSPHERE_ASSERT(!KInterruptManager::AreInterruptsEnabled());

        /* Retrieve information about the exception. */
        const u64 esr   = cpu::GetEsrEl1();
        const u64 afsr0 = cpu::GetAfsr0El1();
        const u64 afsr1 = cpu::GetAfsr1El1();
        u64 far  = 0;
        u32 data = 0;

        /* Collect far and data based on the ec. */
        switch ((esr >> 26) & 0x3F) {
            case 0x0:  /* Unknown */
            case 0xE:  /* Illegal Execution State */
            case 0x38: /* BKPT instruction */
            case 0x3C: /* BRK instruction */
                far   = context->pc;
                data = GetInstructionData(context, esr);
                break;
            case 0x11: /* SVC instruction from Aarch32 */
                if (context->psr & 0x20) {
                    /* Thumb mode. */
                    context->pc -= 2;
                } else {
                    /* ARM mode. */
                    context->pc -= 4;
                }
                far = context->pc;
                break;
            case 0x15: /* SVC instruction from Aarch64 */
                context->pc -= 4;
                far = context->pc;
                break;
            case 0x30: /* Breakpoint from lower EL */
                far = context->pc;
                break;
            default:
                far = cpu::GetFarEl1();
                break;
        }

        /* Note that we're in an exception handler. */
        GetCurrentThread().SetInExceptionHandler();
        {
            const bool is_user_mode = (context->psr & 0xF) == 0;
            if (is_user_mode) {
                /* Handle any changes needed to the user preemption state. */
                if (GetCurrentThread().GetUserPreemptionState() != 0 && GetCurrentProcess().GetPreemptionStatePinnedThread(GetCurrentCoreId()) == nullptr) {
                    KScopedSchedulerLock lk;

                    /* Note the preemption state in process. */
                    GetCurrentProcess().SetPreemptionState();

                    /* Set the kernel preemption state flag. */
                    GetCurrentThread().SetKernelPreemptionState(1);
                }

                /* Enable interrupts while we process the usermode exception. */
                {
                    KScopedInterruptEnable ei;

                    HandleUserException(context, esr, far, afsr0, afsr1, data);
                }
            } else {
                MESOSPHERE_LOG("Unhandled Exception in Supervisor Mode\n");
                MESOSPHERE_LOG("Current Process = %s\n", GetCurrentProcess().GetName());

                for (size_t i = 0; i < 31; i++) {
                    MESOSPHERE_LOG("X[%02zu] = %016lx\n", i, context->x[i]);
                }
                MESOSPHERE_LOG("PC    = %016lx\n", context->pc);
                MESOSPHERE_LOG("SP    = %016lx\n", context->sp);

                MESOSPHERE_PANIC("Unhandled Exception in Supervisor Mode\n");
            }

            MESOSPHERE_ASSERT(!KInterruptManager::AreInterruptsEnabled());

            /* Handle any DPC requests. */
            while (GetCurrentThread().HasDpc()) {
                KDpcManager::HandleDpc();
            }
        }
        /* Note that we're no longer in an exception handler. */
        GetCurrentThread().ClearInExceptionHandler();
    }

}
