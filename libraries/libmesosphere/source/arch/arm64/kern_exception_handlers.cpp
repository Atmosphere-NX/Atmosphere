/*
 * Copyright (c) Atmosphère-NX
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

    void RestoreContext(uintptr_t sp);

}

namespace ams::kern::arch::arm64 {

    namespace {

        enum EsrEc : u32 {
            EsrEc_Unknown                   = 0b000000,
            EsrEc_WaitForInterruptOrEvent   = 0b000001,
            EsrEc_Cp15McrMrc                = 0b000011,
            EsrEc_Cp15McrrMrrc              = 0b000100,
            EsrEc_Cp14McrMrc                = 0b000101,
            EsrEc_FpAccess                  = 0b000111,
            EsrEc_Cp14Mrrc                  = 0b001100,
            EsrEc_BranchTarget              = 0b001101,
            EsrEc_IllegalExecution          = 0b001110,
            EsrEc_Svc32                     = 0b010001,
            EsrEc_Svc64                     = 0b010101,
            EsrEc_SystemInstruction64       = 0b011000,
            EsrEc_SveZen                    = 0b011001,
            EsrEc_PointerAuthInstruction    = 0b011100,
            EsrEc_InstructionAbortEl0       = 0b100000,
            EsrEc_InstructionAbortEl1       = 0b100001,
            EsrEc_PcAlignmentFault          = 0b100010,
            EsrEc_DataAbortEl0              = 0b100100,
            EsrEc_DataAbortEl1              = 0b100101,
            EsrEc_SpAlignmentFault          = 0b100110,
            EsrEc_FpException32             = 0b101000,
            EsrEc_FpException64             = 0b101100,
            EsrEc_SErrorInterrupt           = 0b101111,
            EsrEc_BreakPointEl0             = 0b110000,
            EsrEc_BreakPointEl1             = 0b110001,
            EsrEc_SoftwareStepEl0           = 0b110010,
            EsrEc_SoftwareStepEl1           = 0b110011,
            EsrEc_WatchPointEl0             = 0b110100,
            EsrEc_WatchPointEl1             = 0b110101,
            EsrEc_BkptInstruction           = 0b111000,
            EsrEc_BrkInstruction            = 0b111100,
        };



        u32 GetInstructionDataSupervisorMode(const KExceptionContext *context, u64 esr) {
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

        u32 GetInstructionDataUserMode(const KExceptionContext *context) {
            /* Check for THUMB usermode */
            u32 insn = 0;
            if ((context->psr & 0x3F) == 0x30) {
                u16 insn_high = 0;
                if (UserspaceAccess::CopyMemoryFromUser(std::addressof(insn_high), reinterpret_cast<u16 *>(context->pc & ~0x1), sizeof(insn_high))) {
                    insn = insn_high;

                    /* Check if the instruction was a THUMB mode branch prefix. */
                    if (((insn >> 11) & 0b11110) == 0b11110) {
                        u16 insn_low = 0;
                        if (UserspaceAccess::CopyMemoryFromUser(std::addressof(insn_low), reinterpret_cast<u16 *>((context->pc & ~0x1) + sizeof(u16)), sizeof(insn_low))) {
                            insn = (static_cast<u32>(insn_high) << 16) | (static_cast<u32>(insn_low) << 0);
                        } else {
                            insn = 0;
                        }
                    }
                } else {
                    insn = 0;
                }
            } else {
                u32 insn_value = 0;
                if (UserspaceAccess::CopyMemoryFromUser(std::addressof(insn_value), reinterpret_cast<u32 *>(context->pc), sizeof(insn_value))) {
                    insn = insn_value;
                } else if (KTargetSystem::IsDebugMode() && (context->pc & 3) == 0 && UserspaceAccess::CopyMemoryFromUserSize32BitWithSupervisorAccess(std::addressof(insn_value), reinterpret_cast<u32 *>(context->pc))) {
                    insn = insn_value;
                } else {
                    insn = 0;
                }
            }
            return insn;
        }

        void HandleUserException(KExceptionContext *context, u64 raw_esr, u64 raw_far, u64 afsr0, u64 afsr1, u32 data) {
            /* Pre-process exception registers as needed. */
            u64 esr = raw_esr;
            u64 far = raw_far;
            const u64 ec = (esr >> 26) & 0x3F;
            if (ec == EsrEc_InstructionAbortEl0 || ec == EsrEc_DataAbortEl0) {
                /* Adjust registers if a synchronous external abort has occurred with far not valid. */
                /*     Mask 0x03F = Low 6 bits IFSC == 0x10: "Synchronous External abort,            */
                /*     not on translation table walk or hardware update of translation table.        */
                /*     Mask 0x400 = FnV = "FAR Not Valid"                                            */
                /* TODO: How would we perform this check using named register accesses? */
                if ((esr & 0x43F) == 0x410) {
                    /* Clear the faulting register on memory tagging exception. */
                    far = 0;
                } else {
                    /* If the faulting address is a kernel address, set ISFC = 4. */
                    if (far >= ams::svc::AddressMemoryRegion39Size) {
                        esr = (esr & 0xFFFFFFC0) | 4;
                    }
                }
            }

            KProcess &cur_process = GetCurrentProcess();
            bool should_process_user_exception = KTargetSystem::IsUserExceptionHandlersEnabled();

            /* In the event that we return from this exception, we want SPSR.SS set so that we advance an instruction if single-stepping. */
            #if defined(MESOSPHERE_ENABLE_HARDWARE_SINGLE_STEP)
            context->psr |= (1ul << 21);
            #endif

            /* If we should process the user exception (and it's not a breakpoint), try to enter. */
            const bool is_software_break = (ec == EsrEc_Unknown || ec == EsrEc_IllegalExecution || ec == EsrEc_BkptInstruction || ec == EsrEc_BrkInstruction);
            const bool is_breakpoint     = (ec == EsrEc_BreakPointEl0 || ec == EsrEc_SoftwareStepEl0 || ec == EsrEc_WatchPointEl0);
            if ((should_process_user_exception)                                                                              &&
                !(is_software_break && cur_process.IsAttachedToDebugger() && KDebug::IsBreakInstruction(data, context->psr)) &&
                !(is_breakpoint))
            {
                if (cur_process.EnterUserException()) {
                    /* Fill out the exception info. */
                    const bool is_aarch64 = (context->psr & 0x10) == 0;
                    if (is_aarch64) {
                        /* 64-bit. */
                        ams::svc::aarch64::ExceptionInfo *info = std::addressof(static_cast<ams::svc::aarch64::ProcessLocalRegion *>(cur_process.GetProcessLocalRegionHeapAddress())->exception_info);

                        for (size_t i = 0; i < util::size(info->r); ++i) {
                            info->r[i] = context->x[i];
                        }
                        info->sp     = context->sp;
                        info->lr     = context->x[30];
                        info->pc     = context->pc;
                        info->pstate = (context->psr & cpu::El0Aarch64PsrMask);
                        info->afsr0  = afsr0;
                        info->afsr1  = afsr1;
                        info->esr    = esr;
                        info->far    = far;
                    } else {
                        /* 32-bit. */
                        ams::svc::aarch32::ExceptionInfo *info = std::addressof(static_cast<ams::svc::aarch32::ProcessLocalRegion *>(cur_process.GetProcessLocalRegionHeapAddress())->exception_info);

                        for (size_t i = 0; i < util::size(info->r); ++i) {
                            info->r[i] = context->x[i];
                        }
                        info->sp     = context->x[13];
                        info->lr     = context->x[14];
                        info->pc     = context->pc;
                        info->flags  = 1;

                        info->status_64.pstate = (context->psr & cpu::El0Aarch32PsrMask);
                        info->status_64.afsr0  = afsr0;
                        info->status_64.afsr1  = afsr1;
                        info->status_64.esr    = esr;
                        info->status_64.far    = far;
                    }

                    /* Save the debug parameters to the current thread. */
                    GetCurrentThread().SaveDebugParams(raw_far, raw_esr, data);

                    /* Get the exception type. */
                    u32 type;
                    switch (ec) {
                        case EsrEc_Unknown:
                        case EsrEc_IllegalExecution:
                        case EsrEc_Cp15McrMrc:
                        case EsrEc_Cp15McrrMrrc:
                        case EsrEc_Cp14McrMrc:
                        case EsrEc_Cp14Mrrc:
                        case EsrEc_SystemInstruction64:
                        case EsrEc_BkptInstruction:
                        case EsrEc_BrkInstruction:
                            type = ams::svc::ExceptionType_InstructionAbort;
                            break;
                        case EsrEc_PcAlignmentFault:
                            type = ams::svc::ExceptionType_UnalignedInstruction;
                            break;
                        case EsrEc_SpAlignmentFault:
                            type = ams::svc::ExceptionType_UnalignedData;
                            break;
                        case EsrEc_Svc32:
                        case EsrEc_Svc64:
                            type = ams::svc::ExceptionType_InvalidSystemCall;
                            break;
                        case EsrEc_SErrorInterrupt:
                            type = ams::svc::ExceptionType_MemorySystemError;
                            break;
                        case EsrEc_InstructionAbortEl0:
                            type = ams::svc::ExceptionType_InstructionAbort;
                            break;
                        case EsrEc_DataAbortEl0:
                            /* If esr.IFSC is "Alignment Fault", return UnalignedData instead of DataAbort. */
                            if ((esr & 0x3F) == 0b100001) {
                                type = ams::svc::ExceptionType_UnalignedData;
                            } else {
                                type = ams::svc::ExceptionType_DataAbort;
                            }
                            break;
                        default:
                            type = ams::svc::ExceptionType_DataAbort;
                            break;
                    }

                    /* We want to enter at the process entrypoint, with x0 = type. */
                    context->pc   = GetInteger(cur_process.GetEntryPoint());
                    context->x[0] = type;
                    if (is_aarch64) {
                        context->x[1]   = GetInteger(cur_process.GetProcessLocalRegionAddress() + AMS_OFFSETOF(ams::svc::aarch64::ProcessLocalRegion, exception_info));

                        const auto *plr = GetPointer<ams::svc::aarch64::ProcessLocalRegion>(cur_process.GetProcessLocalRegionAddress());
                        context->sp     = util::AlignDown(reinterpret_cast<uintptr_t>(plr->data) + sizeof(plr->data), 0x10);
                        context->psr    = 0;
                    } else {
                        context->x[1]   = GetInteger(cur_process.GetProcessLocalRegionAddress() + AMS_OFFSETOF(ams::svc::aarch32::ProcessLocalRegion, exception_info));

                        const auto *plr = GetPointer<ams::svc::aarch32::ProcessLocalRegion>(cur_process.GetProcessLocalRegionAddress());
                        context->x[13]  = util::AlignDown(reinterpret_cast<uintptr_t>(plr->data) + sizeof(plr->data), 0x08);
                        context->psr    = 0x10;
                    }

                    /* Process that we're entering a usermode exception on the current thread. */
                    GetCurrentThread().OnEnterUsermodeException();
                    return;
                }
            }

            /* If we should, clear the thread's state as single-step. */
            #if defined(MESOSPHERE_ENABLE_HARDWARE_SINGLE_STEP)
            if (AMS_UNLIKELY(GetCurrentThread().IsHardwareSingleStep())) {
                GetCurrentThread().ClearHardwareSingleStep();
                cpu::MonitorDebugSystemControlRegisterAccessor().SetSoftwareStep(false).Store();
                cpu::InstructionMemoryBarrier();
            }
            #endif

            {
                /* Collect additional information based on the ec. */
                uintptr_t params[3] = {};
                switch (ec) {
                     case EsrEc_Unknown:
                     case EsrEc_IllegalExecution:
                     case EsrEc_BkptInstruction:
                     case EsrEc_BrkInstruction:
                         {
                             params[0] = ams::svc::DebugException_UndefinedInstruction;
                             params[1] = far;
                             params[2] = data;
                         }
                         break;
                     case EsrEc_PcAlignmentFault:
                     case EsrEc_SpAlignmentFault:
                         {
                             params[0] = ams::svc::DebugException_AlignmentFault;
                             params[1]    = far;
                         }
                         break;
                     case EsrEc_Svc32:
                     case EsrEc_Svc64:
                         {
                             params[0] = ams::svc::DebugException_UndefinedSystemCall;
                             params[1] = far;
                             params[2] = (esr & 0xFF);
                         }
                         break;
                     case EsrEc_BreakPointEl0:
                     case EsrEc_SoftwareStepEl0:
                         {
                             params[0] = ams::svc::DebugException_BreakPoint;
                             params[1] = far;
                             params[2] = ams::svc::BreakPointType_HardwareInstruction;
                         }
                         break;
                     case EsrEc_WatchPointEl0:
                         {
                             params[0] = ams::svc::DebugException_BreakPoint;
                             params[1] = far;
                             params[2] = ams::svc::BreakPointType_HardwareData;
                         }
                         break;
                     case EsrEc_SErrorInterrupt:
                         {
                             params[0] = ams::svc::DebugException_MemorySystemError;
                             params[1] = far;
                         }
                         break;
                     case EsrEc_InstructionAbortEl0:
                         {
                             params[0] = ams::svc::DebugException_InstructionAbort;
                             params[1] = far;
                         }
                         break;
                     case EsrEc_DataAbortEl0:
                     default:
                         {
                             params[0] = ams::svc::DebugException_DataAbort;
                             params[1] = far;
                         }
                         break;
                }

                /* Process the debug event. */
                Result result = KDebug::OnDebugEvent(ams::svc::DebugEvent_Exception, params, util::size(params));

                /* If we should stop processing the exception, do so. */
                if (svc::ResultStopProcessingException::Includes(result)) {
                    return;
                }

                #if defined(MESOSPHERE_ENABLE_HARDWARE_SINGLE_STEP)
                {
                    if (ec != EsrEc_SoftwareStepEl0) {
                        /* If the exception wasn't single-step, print details. */
                        MESOSPHERE_EXCEPTION_LOG("Exception occurred. ");

                        {
                            /* Print the current thread's registers. */
                            KDebug::PrintRegister();

                            /* Print a backtrace. */
                            KDebug::PrintBacktrace();
                        }
                    } else {
                        /* If the exception was single-step and we have no debug object, we should just return. */
                        if (AMS_UNLIKELY(!cur_process.IsAttachedToDebugger())) {
                            return;
                        }
                    }
                }
                #else
                {
                    /* Print that an exception occurred. */
                    MESOSPHERE_EXCEPTION_LOG("Exception occurred. ");

                    {
                        /* Print the current thread's registers. */
                        KDebug::PrintRegister();

                        /* Print a backtrace. */
                        KDebug::PrintBacktrace();
                    }
                }
                #endif

                /* If the SVC is handled, handle it. */
                if (!svc::ResultNotHandled::Includes(result)) {
                    /* If we successfully enter jit debug, stop processing the exception. */
                    if (cur_process.EnterJitDebug(ams::svc::DebugEvent_Exception, static_cast<ams::svc::DebugException>(params[0]), params[1], params[2])) {
                        return;
                    }
                }
            }

            /* Exit the current process. */
            cur_process.Exit();
        }

    }

    /* NOTE: This function is called from ASM. */
    void FpuContextSwitchHandler() {
        KThreadContext::FpuContextSwitchHandler(GetCurrentThreadPointer());
    }

    /* NOTE: This function is called from ASM. */
    void ReturnFromException(Result user_result) {
        /* Get the current thread. */
        KThread *cur_thread = GetCurrentThreadPointer();

        /* Get the current exception context. */
        KExceptionContext *e_ctx = GetExceptionContext(cur_thread);

        /* Get the current process. */
        KProcess &cur_process = GetCurrentProcess();

        /* Read the exception info that userland put in tls. */
        union {
            ams::svc::aarch64::ExceptionInfo info64;
            ams::svc::aarch32::ExceptionInfo info32;
        } info = {};

        const bool is_aarch64 = (e_ctx->psr & 0x10) == 0;
        if (is_aarch64) {
            /* We're 64-bit. */
            info.info64 = static_cast<const ams::svc::aarch64::ProcessLocalRegion *>(cur_process.GetProcessLocalRegionHeapAddress())->exception_info;
        } else {
            /* We're 32-bit. */
            info.info32 = static_cast<const ams::svc::aarch32::ProcessLocalRegion *>(cur_process.GetProcessLocalRegionHeapAddress())->exception_info;
        }

        /* Try to leave the user exception. */
        if (cur_process.LeaveUserException()) {
            /* Process that we're leaving a usermode exception on the current thread. */
            GetCurrentThread().OnLeaveUsermodeException();

            /* Copy the user context to the thread context. */
            if (is_aarch64) {
                for (size_t i = 0; i < util::size(info.info64.r); ++i) {
                    e_ctx->x[i] = info.info64.r[i];
                }
                e_ctx->x[30] = info.info64.lr;
                e_ctx->sp    = info.info64.sp;
                e_ctx->pc    = info.info64.pc;
                e_ctx->psr   = (info.info64.pstate & cpu::El0Aarch64PsrMask) | (e_ctx->psr & ~cpu::El0Aarch64PsrMask);
            } else {
                for (size_t i = 0; i < util::size(info.info32.r); ++i) {
                    e_ctx->x[i] = info.info32.r[i];
                }
                e_ctx->x[14] = info.info32.lr;
                e_ctx->x[13] = info.info32.sp;
                e_ctx->pc    = info.info32.pc;
                e_ctx->psr   = (info.info32.status_64.pstate & cpu::El0Aarch32PsrMask) | (e_ctx->psr & ~cpu::El0Aarch32PsrMask);
            }

            /* Note that PC was adjusted. */
            e_ctx->write = 1;

            if (R_SUCCEEDED(user_result)) {
                /* If result handling succeeded, just restore the context. */
                svc::RestoreContext(reinterpret_cast<uintptr_t>(e_ctx));
            } else {
                /* Restore the debug params for the exception. */
                uintptr_t far, esr, data;
                GetCurrentThread().RestoreDebugParams(std::addressof(far), std::addressof(esr), std::addressof(data));

                /* Pre-process exception registers as needed. */
                const u64 ec = (esr >> 26) & 0x3F;
                if (ec == EsrEc_InstructionAbortEl0 || ec == EsrEc_DataAbortEl0) {
                    /* Adjust registers if a synchronous external abort has occurred with far not valid. */
                    /*     Mask 0x03F = Low 6 bits IFSC == 0x10: "Synchronous External abort,            */
                    /*     not on translation table walk or hardware update of translation table.        */
                    /*     Mask 0x400 = FnV = "FAR Not Valid"                                            */
                    /* TODO: How would we perform this check using named register accesses? */
                    if ((esr & 0x43F) == 0x410) {
                        /* Clear the faulting register on memory tagging exception. */
                        far = 0;
                    } else {
                        /* If the faulting address is a kernel address, set ISFC = 4. */
                        if (far >= ams::svc::AddressMemoryRegion39Size) {
                            esr = (esr & 0xFFFFFFC0) | 4;
                        }
                    }
                }

                /* Collect additional information based on the ec. */
                uintptr_t params[3] = {};
                switch (ec) {
                     case EsrEc_Unknown:
                     case EsrEc_IllegalExecution:
                     case EsrEc_BkptInstruction:
                     case EsrEc_BrkInstruction:
                         {
                             params[0] = ams::svc::DebugException_UndefinedInstruction;
                             params[1] = far;
                             params[2] = data;
                         }
                         break;
                     case EsrEc_PcAlignmentFault:
                     case EsrEc_SpAlignmentFault:
                         {
                             params[0] = ams::svc::DebugException_AlignmentFault;
                             params[1] = far;
                         }
                         break;
                     case EsrEc_Svc32:
                     case EsrEc_Svc64:
                         {
                             params[0] = ams::svc::DebugException_UndefinedSystemCall;
                             params[1] = far;
                             params[2] = (esr & 0xFF);
                         }
                         break;
                     case EsrEc_SErrorInterrupt:
                         {
                             params[0] = ams::svc::DebugException_MemorySystemError;
                             params[1] = far;
                         }
                         break;
                     case EsrEc_InstructionAbortEl0:
                         {
                             params[0] = ams::svc::DebugException_InstructionAbort;
                             params[1] = far;
                         }
                         break;
                     case EsrEc_DataAbortEl0:
                     default:
                         {
                             params[0] = ams::svc::DebugException_DataAbort;
                             params[1] = far;
                         }
                         break;
                }

                /* Process the debug event. */
                Result result = KDebug::OnDebugEvent(ams::svc::DebugEvent_Exception, params, util::size(params));

                /* If the SVC is handled, handle it. */
                if (!svc::ResultNotHandled::Includes(result)) {
                    /* If we should stop processing the exception, restore. */
                    if (svc::ResultStopProcessingException::Includes(result)) {
                        svc::RestoreContext(reinterpret_cast<uintptr_t>(e_ctx));
                    }

                    /* If we successfully enter jit debug, restore. */
                    if (cur_process.EnterJitDebug(ams::svc::DebugEvent_Exception, static_cast<ams::svc::DebugException>(params[0]), params[1], params[2])) {
                        svc::RestoreContext(reinterpret_cast<uintptr_t>(e_ctx));
                    }
                }

                /* Otherwise, if result debug was returned, restore. */
                if (svc::ResultDebug::Includes(result)) {
                    svc::RestoreContext(reinterpret_cast<uintptr_t>(e_ctx));
                }
            }
        }

        /* Print that an exception occurred. */
        MESOSPHERE_EXCEPTION_LOG("Exception occurred. ");

        /* Exit the current process. */
        GetCurrentProcess().Exit();
    }

    /* NOTE: This function is called from ASM. */
    void HandleException(KExceptionContext *context) {
        MESOSPHERE_ASSERT(!KInterruptManager::AreInterruptsEnabled());

        /* Retrieve information about the exception. */
        const bool is_user_mode = (context->psr & 0xF) == 0;
        const u64 esr           = cpu::GetEsrEl1();
        const u64 afsr0         = cpu::GetAfsr0El1();
        const u64 afsr1         = cpu::GetAfsr1El1();
        u64 far  = 0;
        u32 data = 0;

        /* Collect far and data based on the ec. */
        switch ((esr >> 26) & 0x3F) {
            case EsrEc_Unknown:
            case EsrEc_IllegalExecution:
            case EsrEc_BkptInstruction:
            case EsrEc_BrkInstruction:
                far   = context->pc;
                /* NOTE: Nintendo always calls GetInstructionDataUserMode. */
                if (is_user_mode) {
                    data = GetInstructionDataUserMode(context);
                } else {
                    data = GetInstructionDataSupervisorMode(context, esr);
                }
                break;
            case EsrEc_Svc32:
                if (context->psr & 0x20) {
                    /* Thumb mode. */
                    context->pc -= 2;
                } else {
                    /* ARM mode. */
                    context->pc -= 4;
                }
                far = context->pc;
                break;
            case EsrEc_Svc64:
                context->pc -= 4;
                far = context->pc;
                break;
            case EsrEc_BreakPointEl0:
                far = context->pc;
                break;
            default:
                far = cpu::GetFarEl1();
                break;
        }

        /* Note that we're in an exception handler. */
        GetCurrentThread().SetInExceptionHandler();

        /* Verify that spsr's M is allowable (EL0t). */
        {
            if (is_user_mode) {
                /* If the user disable count is set, we may need to pin the current thread. */
                if (GetCurrentThread().GetUserDisableCount() != 0 && GetCurrentProcess().GetPinnedThread(GetCurrentCoreId()) == nullptr) {
                    KScopedSchedulerLock lk;

                    /* Pin the current thread. */
                    GetCurrentProcess().PinCurrentThread();

                    /* Set the interrupt flag for the thread. */
                    GetCurrentThread().SetInterruptFlag();
                }

                /* Enable interrupts while we process the usermode exception. */
                {
                    KScopedInterruptEnable ei;

                    /* Terminate the thread, if we should. */
                    if (GetCurrentThread().IsTerminationRequested()) {
                        GetCurrentThread().Exit();
                    }

                    HandleUserException(context, esr, far, afsr0, afsr1, data);
                }
            } else {
                const s32 core_id = GetCurrentCoreId();

                MESOSPHERE_LOG("%d: Unhandled Exception in Supervisor Mode\n", core_id);
                if (GetCurrentProcessPointer() != nullptr) {
                    MESOSPHERE_LOG("%d: Current Process = %s\n", core_id, GetCurrentProcess().GetName());
                }

                for (size_t i = 0; i < 31; i++) {
                    MESOSPHERE_LOG("%d: X[%02zu] = %016lx\n", core_id, i, context->x[i]);
                }
                MESOSPHERE_LOG("%d: PC    = %016lx\n", core_id, context->pc);
                MESOSPHERE_LOG("%d: SP    = %016lx\n", core_id, context->sp);

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
