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

        constexpr inline u64 ForbiddenBreakPointFlagsMask = (((1ul << 40) - 1) << 24) | /* Reserved upper bits. */
                                                            (((1ul <<  1) - 1) << 23) | /* Match VMID BreakPoint Type. */
                                                            (((1ul <<  2) - 1) << 14) | /* Security State Control. */
                                                            (((1ul <<  1) - 1) << 13) | /* Hyp Mode Control. */
                                                            (((1ul <<  4) - 1) <<  9) | /* Reserved middle bits. */
                                                            (((1ul <<  2) - 1) <<  3) | /* Reserved lower bits. */
                                                            (((1ul <<  2) - 1) <<  1);  /* Privileged Mode Control. */

        static_assert(ForbiddenBreakPointFlagsMask == 0xFFFFFFFFFF80FE1Eul);

        constexpr inline u64 ForbiddenWatchPointFlagsMask = (((1ul << 32) - 1) << 32) | /* Reserved upper bits. */
                                                            (((1ul <<  4) - 1) << 20) | /* WatchPoint Type. */
                                                            (((1ul <<  2) - 1) << 14) | /* Security State Control. */
                                                            (((1ul <<  1) - 1) << 13) | /* Hyp Mode Control. */
                                                            (((1ul <<  2) - 1) <<  1);  /* Privileged Access Control. */

        static_assert(ForbiddenWatchPointFlagsMask == 0xFFFFFFFF00F0E006ul);

        constexpr inline u32 El0PsrMask = 0xFF0FFE20;

    }

    uintptr_t KDebug::GetProgramCounter(const KThread &thread) {
        return GetExceptionContext(std::addressof(thread))->pc;
    }

    void KDebug::SetPreviousProgramCounter() {
        /* Get the current thread. */
        KThread *thread = GetCurrentThreadPointer();
        MESOSPHERE_ASSERT(thread->IsCallingSvc());

        /* Get the exception context. */
        KExceptionContext *e_ctx = GetExceptionContext(thread);

        /* Set the previous pc. */
        if (e_ctx->write == 0) {
            /* Subtract from the program counter. */
            if (thread->GetOwnerProcess()->Is64Bit()) {
                e_ctx->pc -= sizeof(u32);
            } else {
                e_ctx->pc -= (e_ctx->psr & 0x20) ? sizeof(u16) : sizeof(u32);
            }

            /* Mark that we've set. */
            e_ctx->write = 1;
        }
    }

    Result KDebug::GetThreadContextImpl(ams::svc::ThreadContext *out, KThread *thread, u32 context_flags) {
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());
        MESOSPHERE_ASSERT(thread != GetCurrentThreadPointer());

        /* Get the exception context. */
        const KExceptionContext *e_ctx = GetExceptionContext(thread);

        /* If general registers are requested, get them. */
        if ((context_flags & ams::svc::ThreadContextFlag_General) != 0) {
            if (!thread->IsCallingSvc() || thread->GetSvcId() == svc::SvcId_ReturnFromException) {
                if (this->Is64Bit()) {
                    /* Get X0-X28. */
                    for (auto i = 0; i <= 28; ++i) {
                        out->r[i] = e_ctx->x[i];
                    }
                } else {
                    /* Get R0-R12. */
                    for (auto i = 0; i <= 12; ++i) {
                        out->r[i] = static_cast<u32>(e_ctx->x[i]);
                    }
                }
            }
        }

        /* If control flags are requested, get them. */
        if ((context_flags & ams::svc::ThreadContextFlag_Control) != 0) {
            if (this->Is64Bit()) {
                out->fp     = e_ctx->x[29];
                out->lr     = e_ctx->x[30];
                out->sp     = e_ctx->sp;
                out->pc     = e_ctx->pc;
                out->pstate = (e_ctx->psr & El0PsrMask);

                /* Adjust PC if we should. */
                if (e_ctx->write == 0 && thread->IsCallingSvc()) {
                    out->pc -= sizeof(u32);
                }

                out->tpidr = e_ctx->tpidr;
            } else {
                out->r[11]  = static_cast<u32>(e_ctx->x[11]);
                out->r[13]  = static_cast<u32>(e_ctx->x[13]);
                out->r[14]  = static_cast<u32>(e_ctx->x[14]);
                out->lr     = 0;
                out->sp     = 0;
                out->pc     = e_ctx->pc;
                out->pstate = (e_ctx->psr & El0PsrMask);

                /* Adjust PC if we should. */
                if (e_ctx->write == 0 && thread->IsCallingSvc()) {
                    out->pc -= (e_ctx->psr & 0x20) ? sizeof(u16) : sizeof(u32);
                }

                out->tpidr = static_cast<u32>(e_ctx->tpidr);
            }
        }

        /* Get the FPU context. */
        return this->GetFpuContext(out, thread, context_flags);
    }

    Result KDebug::SetThreadContextImpl(const ams::svc::ThreadContext &ctx, KThread *thread, u32 context_flags) {
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());
        MESOSPHERE_ASSERT(thread != GetCurrentThreadPointer());

        /* Get the exception context. */
        KExceptionContext *e_ctx = GetExceptionContext(thread);

        /* If general registers are requested, set them. */
        if ((context_flags & ams::svc::ThreadContextFlag_General) != 0) {
            if (this->Is64Bit()) {
                /* Set X0-X28. */
                for (auto i = 0; i <= 28; ++i) {
                    e_ctx->x[i] = ctx.r[i];
                }
            } else {
                /* Set R0-R12. */
                for (auto i = 0; i <= 12; ++i) {
                    e_ctx->x[i] = static_cast<u32>(ctx.r[i]);
                }
            }
        }

        /* If control flags are requested, set them. */
        if ((context_flags & ams::svc::ThreadContextFlag_Control) != 0) {
            /* Mark ourselve as having adjusted pc. */
            e_ctx->write = 1;

            if (this->Is64Bit()) {
                e_ctx->x[29] = ctx.fp;
                e_ctx->x[30] = ctx.lr;
                e_ctx->sp    = ctx.sp;
                e_ctx->pc    = ctx.pc;
                e_ctx->psr   = ((ctx.pstate & El0PsrMask) | (e_ctx->psr & ~El0PsrMask));
                e_ctx->tpidr = ctx.tpidr;
            } else {
                e_ctx->x[13] = static_cast<u32>(ctx.r[13]);
                e_ctx->x[14] = static_cast<u32>(ctx.r[14]);
                e_ctx->x[30] = 0;
                e_ctx->sp    = 0;
                e_ctx->pc    = static_cast<u32>(ctx.pc);
                e_ctx->psr   = ((ctx.pstate & El0PsrMask) | (e_ctx->psr & ~El0PsrMask));
                e_ctx->tpidr = ctx.tpidr;
            }
        }

        /* Set the FPU context. */
        return this->SetFpuContext(ctx, thread, context_flags);
    }

    Result KDebug::GetFpuContext(ams::svc::ThreadContext *out, KThread *thread, u32 context_flags) {
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());
        MESOSPHERE_ASSERT(thread != GetCurrentThreadPointer());

        /* Succeed if there's nothing to do. */
        R_SUCCEED_IF((context_flags & (ams::svc::ThreadContextFlag_Fpu | ams::svc::ThreadContextFlag_FpuControl)) == 0);

        /* Get the thread context. */
        KThreadContext *t_ctx = std::addressof(thread->GetContext());

        /* Get the FPU control registers, if required. */
        if ((context_flags & ams::svc::ThreadContextFlag_FpuControl) != 0) {
            out->fpsr = t_ctx->GetFpsr();
            out->fpcr = t_ctx->GetFpcr();
        }

        /* Get the FPU registers, if required. */
        if ((context_flags & ams::svc::ThreadContextFlag_Fpu) != 0) {
            static_assert(util::size(ams::svc::ThreadContext{}.v) == KThreadContext::NumFpuRegisters);
            const u128 *f = t_ctx->GetFpuRegisters();

            if (this->Is64Bit()) {
                for (size_t i = 0; i < KThreadContext::NumFpuRegisters; ++i) {
                    out->v[i] = f[i];
                }
            } else {
                for (size_t i = 0; i < KThreadContext::NumFpuRegisters / 2; ++i) {
                    out->v[i] = f[i];
                }
                for (size_t i = KThreadContext::NumFpuRegisters / 2; i < KThreadContext::NumFpuRegisters; ++i) {
                    out->v[i] = 0;
                }
            }
        }

        return ResultSuccess();
    }

    Result KDebug::SetFpuContext(const ams::svc::ThreadContext &ctx, KThread *thread, u32 context_flags) {
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());
        MESOSPHERE_ASSERT(thread != GetCurrentThreadPointer());

        /* Succeed if there's nothing to do. */
        R_SUCCEED_IF((context_flags & (ams::svc::ThreadContextFlag_Fpu | ams::svc::ThreadContextFlag_FpuControl)) == 0);

        /* Get the thread context. */
        KThreadContext *t_ctx = std::addressof(thread->GetContext());

        /* Set the FPU control registers, if required. */
        if ((context_flags & ams::svc::ThreadContextFlag_FpuControl) != 0) {
            t_ctx->SetFpsr(ctx.fpsr);
            t_ctx->SetFpcr(ctx.fpcr);
        }

        /* Set the FPU registers, if required. */
        if ((context_flags & ams::svc::ThreadContextFlag_Fpu) != 0) {
            static_assert(util::size(ams::svc::ThreadContext{}.v) == KThreadContext::NumFpuRegisters);
            t_ctx->SetFpuRegisters(ctx.v, this->Is64Bit());
        }

        return ResultSuccess();
    }

    Result KDebug::BreakIfAttached(ams::svc::BreakReason break_reason, uintptr_t address, size_t size) {
        return KDebugBase::OnDebugEvent(ams::svc::DebugEvent_Exception, ams::svc::DebugException_UserBreak, GetProgramCounter(GetCurrentThread()), break_reason, address, size);
    }

    #define MESOSPHERE_SET_HW_BREAK_POINT(ID, FLAGS, VALUE) \
        ({                                                  \
            cpu::SetDbgBcr##ID##El1(0);                     \
            cpu::EnsureInstructionConsistency();            \
            cpu::SetDbgBvr##ID##El1(VALUE);                 \
            cpu::EnsureInstructionConsistency();            \
            cpu::SetDbgBcr##ID##El1(FLAGS);                 \
            cpu::EnsureInstructionConsistency();            \
        })

    #define MESOSPHERE_SET_HW_WATCH_POINT(ID, FLAGS, VALUE) \
        ({                                                  \
            cpu::SetDbgWcr##ID##El1(0);                     \
            cpu::EnsureInstructionConsistency();            \
            cpu::SetDbgWvr##ID##El1(VALUE);                 \
            cpu::EnsureInstructionConsistency();            \
            cpu::SetDbgWcr##ID##El1(FLAGS);                 \
            cpu::EnsureInstructionConsistency();            \
        })

    Result KDebug::SetHardwareBreakPoint(ams::svc::HardwareBreakPointRegisterName name, u64 flags, u64 value) {
        /* Get the debug feature register. */
        cpu::DebugFeatureRegisterAccessor dfr0;

        /* Extract interesting info from the debug feature register. */
        const auto num_bp  = dfr0.GetNumBreakpoints();
        const auto num_wp  = dfr0.GetNumWatchpoints();
        const auto num_ctx = dfr0.GetNumContextAwareBreakpoints();

        if (ams::svc::HardwareBreakPointRegisterName_I0 <= name && name <= ams::svc::HardwareBreakPointRegisterName_I15) {
            /* Check that the name is a valid instruction breakpoint. */
            R_UNLESS((name - ams::svc::HardwareBreakPointRegisterName_I0) <= num_bp, svc::ResultNotSupported());

            /* We may be getting the process, so prepare a scoped reference holder. */
            KScopedAutoObject<KProcess> process;

            /* Configure flags/value. */
            if ((flags & 1) != 0) {
                /* We're enabling the breakpoint. Check that the flags are allowable. */
                R_UNLESS((flags & ForbiddenBreakPointFlagsMask) == 0, svc::ResultInvalidCombination());

                /* Require that the breakpoint be linked or match context id. */
                R_UNLESS((flags & ((1ul << 21) | (1ul << 20))) != 0, svc::ResultInvalidCombination());

                /* If the breakpoint matches context id, we need to get the context id. */
                if ((flags & (1ul << 21)) != 0) {
                    /* Ensure that the breakpoint is context-aware. */
                    R_UNLESS((name - ams::svc::HardwareBreakPointRegisterName_I0) <= (num_bp - num_ctx), svc::ResultNotSupported());

                    /* Check that the breakpoint does not have the mismatch bit. */
                    R_UNLESS((flags & (1ul << 22)) == 0, svc::ResultInvalidCombination());

                    /* Get the debug object from the current handle table. */
                    KScopedAutoObject debug = GetCurrentProcess().GetHandleTable().GetObject<KDebug>(static_cast<ams::svc::Handle>(value));
                    R_UNLESS(debug.IsNotNull(), svc::ResultInvalidHandle());

                    /* Get the process from the debug object. */
                    process = debug->GetProcess();
                    R_UNLESS(process.IsNotNull(), svc::ResultProcessTerminated());

                    /* Set the value to be the context id. */
                    value = process->GetId() & 0xFFFFFFFF;
                }

                /* Set the breakpoint as non-secure EL0-only. */
                flags |= (1ul << 14) | (2ul << 1);
            } else {
                /* We're disabling the breakpoint. */
                flags = 0;
                value = 0;
            }

            /* Set the breakpoint. */
            switch (name) {
                case ams::svc::HardwareBreakPointRegisterName_I0:  MESOSPHERE_SET_HW_BREAK_POINT( 0, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I1:  MESOSPHERE_SET_HW_BREAK_POINT( 1, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I2:  MESOSPHERE_SET_HW_BREAK_POINT( 2, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I3:  MESOSPHERE_SET_HW_BREAK_POINT( 3, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I4:  MESOSPHERE_SET_HW_BREAK_POINT( 4, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I5:  MESOSPHERE_SET_HW_BREAK_POINT( 5, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I6:  MESOSPHERE_SET_HW_BREAK_POINT( 6, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I7:  MESOSPHERE_SET_HW_BREAK_POINT( 7, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I8:  MESOSPHERE_SET_HW_BREAK_POINT( 8, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I9:  MESOSPHERE_SET_HW_BREAK_POINT( 9, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I10: MESOSPHERE_SET_HW_BREAK_POINT(10, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I11: MESOSPHERE_SET_HW_BREAK_POINT(11, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I12: MESOSPHERE_SET_HW_BREAK_POINT(12, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I13: MESOSPHERE_SET_HW_BREAK_POINT(13, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I14: MESOSPHERE_SET_HW_BREAK_POINT(14, flags, value); break;
                case ams::svc::HardwareBreakPointRegisterName_I15: MESOSPHERE_SET_HW_BREAK_POINT(15, flags, value); break;
                default: break;
            }
        } else if (ams::svc::HardwareBreakPointRegisterName_D0 <= name && name <= ams::svc::HardwareBreakPointRegisterName_D15) {
            /* Check that the name is a valid data breakpoint. */
            R_UNLESS((name - ams::svc::HardwareBreakPointRegisterName_D0) <= num_wp, svc::ResultNotSupported());

            /* Configure flags/value. */
            if ((flags & 1) != 0) {
                /* We're enabling the watchpoint. Check that the flags are allowable. */
                R_UNLESS((flags & ForbiddenWatchPointFlagsMask) == 0, svc::ResultInvalidCombination());

                /* Set the breakpoint as linked non-secure EL0-only. */
                flags |= (1ul << 20) | (1ul << 14) | (2ul << 1);
            } else {
                /* We're disabling the watchpoint. */
                flags = 0;
                value = 0;
            }

            /* Set the watchkpoint. */
            switch (name) {
                case ams::svc::HardwareBreakPointRegisterName_D0:  MESOSPHERE_SET_HW_WATCH_POINT( 0, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D1:  MESOSPHERE_SET_HW_WATCH_POINT( 1, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D2:  MESOSPHERE_SET_HW_WATCH_POINT( 2, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D3:  MESOSPHERE_SET_HW_WATCH_POINT( 3, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D4:  MESOSPHERE_SET_HW_WATCH_POINT( 4, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D5:  MESOSPHERE_SET_HW_WATCH_POINT( 5, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D6:  MESOSPHERE_SET_HW_WATCH_POINT( 6, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D7:  MESOSPHERE_SET_HW_WATCH_POINT( 7, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D8:  MESOSPHERE_SET_HW_WATCH_POINT( 8, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D9:  MESOSPHERE_SET_HW_WATCH_POINT( 9, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D10: MESOSPHERE_SET_HW_WATCH_POINT(10, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D11: MESOSPHERE_SET_HW_WATCH_POINT(11, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D12: MESOSPHERE_SET_HW_WATCH_POINT(12, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D13: MESOSPHERE_SET_HW_WATCH_POINT(13, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D14: MESOSPHERE_SET_HW_WATCH_POINT(14, flags, value);  break;
                case ams::svc::HardwareBreakPointRegisterName_D15: MESOSPHERE_SET_HW_WATCH_POINT(15, flags, value);  break;
                default: break;
            }
        } else {
            /* Invalid name. */
            return svc::ResultInvalidEnumValue();
        }

        return ResultSuccess();
    }

    #undef MESOSPHERE_SET_HW_WATCH_POINT
    #undef MESOSPHERE_SET_HW_BREAK_POINT

    void KDebug::PrintRegister(KThread *thread) {
        #if defined(MESOSPHERE_BUILD_FOR_DEBUGGING)
        {
            /* Treat no thread as current thread. */
            if (thread == nullptr) {
                thread = GetCurrentThreadPointer();
            }

            /* Get the exception context. */
            KExceptionContext *e_ctx = GetExceptionContext(thread);

            /* Get the owner process. */
            if (auto *process = thread->GetOwnerProcess(); process != nullptr) {
                /* Lock the owner process. */
                KScopedLightLock state_lk(process->GetStateLock());
                KScopedLightLock list_lk(process->GetListLock());

                /* Suspend all the process's threads. */
                {
                    KScopedSchedulerLock sl;

                    auto end = process->GetThreadList().end();
                    for (auto it = process->GetThreadList().begin(); it != end; ++it) {
                        if (std::addressof(*it) != GetCurrentThreadPointer()) {
                            it->RequestSuspend(KThread::SuspendType_Backtrace);
                        }
                    }
                }

                /* Print the registers. */
                MESOSPHERE_RELEASE_LOG("Registers\n");
                if ((e_ctx->psr & 0x10) == 0) {
                    /* 64-bit thread. */
                    for (auto i = 0; i < 31; ++i) {
                        MESOSPHERE_RELEASE_LOG("   X[%2d]:     0x%016lx\n", i, e_ctx->x[i]);
                    }
                    MESOSPHERE_RELEASE_LOG("   SP:        0x%016lx\n", e_ctx->sp);
                    MESOSPHERE_RELEASE_LOG("   PC:        0x%016lx\n", e_ctx->pc - sizeof(u32));
                    MESOSPHERE_RELEASE_LOG("   PSR:       0x%08x\n", e_ctx->psr);
                    MESOSPHERE_RELEASE_LOG("   TPIDR_EL0: 0x%016lx\n", e_ctx->tpidr);
                } else {
                    /* 32-bit thread. */
                    for (auto i = 0; i < 13; ++i) {
                        MESOSPHERE_RELEASE_LOG("   R[%2d]: 0x%08x\n", i, static_cast<u32>(e_ctx->x[i]));
                    }
                    MESOSPHERE_RELEASE_LOG("   SP:    0x%08x\n", static_cast<u32>(e_ctx->x[13]));
                    MESOSPHERE_RELEASE_LOG("   LR:    0x%08x\n", static_cast<u32>(e_ctx->x[14]));
                    MESOSPHERE_RELEASE_LOG("   PC:    0x%08x\n", static_cast<u32>(e_ctx->pc) - static_cast<u32>((e_ctx->psr & 0x20) ? sizeof(u16) : sizeof(u32)));
                    MESOSPHERE_RELEASE_LOG("   PSR:   0x%08x\n", e_ctx->psr);
                    MESOSPHERE_RELEASE_LOG("   TPIDR: 0x%08x\n", static_cast<u32>(e_ctx->tpidr));
                }

                /* Resume the threads that we suspended. */
                {
                    KScopedSchedulerLock sl;

                    auto end = process->GetThreadList().end();
                    for (auto it = process->GetThreadList().begin(); it != end; ++it) {
                        if (std::addressof(*it) != GetCurrentThreadPointer()) {
                            it->Resume(KThread::SuspendType_Backtrace);
                        }
                    }
                }
            }
        }
        #else
            MESOSPHERE_UNUSED(thread);
        #endif
    }

    #if defined(MESOSPHERE_BUILD_FOR_DEBUGGING)
    namespace {

        bool IsHeapPhysicalAddress(KPhysicalAddress phys_addr) {
            const KMemoryRegion *cached = nullptr;
            return KMemoryLayout::IsHeapPhysicalAddress(cached, phys_addr);
        }

        template<typename T>
        bool ReadValue(T *out, KProcess *process, uintptr_t address) {
            KPhysicalAddress   phys_addr;
            KMemoryInfo        mem_info;
            ams::svc::PageInfo page_info;

            if (!util::IsAligned(address, sizeof(T))) {
                return false;
            }
            if (R_FAILED(process->GetPageTable().QueryInfo(std::addressof(mem_info), std::addressof(page_info), address))) {
                return false;
            }
            if ((mem_info.GetPermission() & KMemoryPermission_UserRead) != KMemoryPermission_UserRead) {
                return false;
            }
            if (!process->GetPageTable().GetPhysicalAddress(std::addressof(phys_addr), address)) {
                return false;
            }
            if (!IsHeapPhysicalAddress(phys_addr)) {
                return false;
            }

            *out = *GetPointer<T>(process->GetPageTable().GetHeapVirtualAddress(phys_addr));
            return true;
        }

        bool GetModuleName(char *dst, size_t dst_size, KProcess *process, uintptr_t base_address) {
            /* Locate .rodata. */
            KMemoryInfo        mem_info;
            ams::svc::PageInfo page_info;
            KMemoryState       mem_state = KMemoryState_None;

            while (true) {
                if (R_FAILED(process->GetPageTable().QueryInfo(std::addressof(mem_info), std::addressof(page_info), base_address))) {
                    return false;
                }
                if (mem_state == KMemoryState_None) {
                    mem_state = mem_info.GetState();
                    if (mem_state != KMemoryState_Code && mem_state != KMemoryState_AliasCode) {
                        return false;
                    }
                }
                if (mem_info.GetState() != mem_state) {
                    return false;
                }
                if (mem_info.GetPermission() == KMemoryPermission_UserRead) {
                    break;
                }
                base_address = mem_info.GetEndAddress();
            }

            /* Check that first value is 0. */
            u32 val;
            if (!ReadValue(std::addressof(val), process, base_address)) {
                return false;
            }
            if (val != 0) {
                return false;
            }

            /* Read the name length. */
            if (!ReadValue(std::addressof(val), process, base_address + sizeof(u32))) {
                return false;
            }
            if (!(0 < val && val < dst_size)) {
                return false;
            }
            const size_t name_len = val;

            /* Read the name, one character at a time. */
            for (size_t i = 0; i < name_len; ++i) {
                if (!ReadValue(dst + i, process, base_address + 2 * sizeof(u32) + i)) {
                    return false;
                }
                if (!(0 < dst[i] && dst[i] <= 0x7F)) {
                    return false;
                }
            }

            /* NULL-terminate. */
            dst[name_len] = 0;

            return true;
        }

        void PrintAddress(uintptr_t address) {
            MESOSPHERE_RELEASE_LOG("   %p\n", reinterpret_cast<void *>(address));
        }

        void PrintAddressWithModuleName(uintptr_t address, bool has_module_name, const char *module_name, uintptr_t base_address) {
            if (has_module_name) {
                MESOSPHERE_RELEASE_LOG("   %p [%10s + %8lx]\n", reinterpret_cast<void *>(address), module_name, address - base_address);
            } else {
                MESOSPHERE_RELEASE_LOG("   %p [%10lx + %8lx]\n", reinterpret_cast<void *>(address), base_address, address - base_address);
            }
        }

        void PrintAddressWithSymbol(uintptr_t address, bool has_module_name, const char *module_name, uintptr_t base_address, const char *symbol_name, uintptr_t func_address) {
            if (has_module_name) {
                MESOSPHERE_RELEASE_LOG("   %p [%10s + %8lx] (%s + %lx)\n", reinterpret_cast<void *>(address), module_name, address - base_address, symbol_name, address - func_address);
            } else {
                MESOSPHERE_RELEASE_LOG("   %p [%10lx + %8lx] (%s + %lx)\n", reinterpret_cast<void *>(address), base_address, address - base_address, symbol_name, address - func_address);
            }
        }

        void PrintCodeAddress(KProcess *process, uintptr_t address, bool is_lr = true) {
            /* Prepare to parse + print the address. */
            uintptr_t          test_address = is_lr ? address - sizeof(u32) : address;
            uintptr_t          base_address = address;
            uintptr_t          dyn_address  = 0;
            uintptr_t          sym_tab      = 0;
            uintptr_t          str_tab      = 0;
            size_t             num_sym      = 0;

            u64                temp_64;
            u32                temp_32;

            /* Locate the start of .text. */
            KMemoryInfo        mem_info;
            ams::svc::PageInfo page_info;
            KMemoryState       mem_state = KMemoryState_None;
            while (true) {
                if (R_FAILED(process->GetPageTable().QueryInfo(std::addressof(mem_info), std::addressof(page_info), base_address))) {
                    return PrintAddress(address);
                }
                if (mem_state == KMemoryState_None) {
                    mem_state = mem_info.GetState();
                    if (mem_state != KMemoryState_Code && mem_state != KMemoryState_AliasCode) {
                        return PrintAddress(address);
                    }
                } else if (mem_info.GetState() != mem_state) {
                    return PrintAddress(address);
                }
                if (mem_info.GetPermission() != KMemoryPermission_UserReadExecute) {
                    return PrintAddress(address);
                }
                base_address = mem_info.GetAddress();

                if (R_FAILED(process->GetPageTable().QueryInfo(std::addressof(mem_info), std::addressof(page_info), base_address - 1))) {
                    return PrintAddress(address);
                }
                if (mem_info.GetState() != mem_state) {
                    break;
                }
                if (mem_info.GetPermission() != KMemoryPermission_UserReadExecute) {
                    break;
                }
            }

            /* Read the first instruction. */
            if (!ReadValue(std::addressof(temp_32), process, base_address)) {
                return PrintAddress(address);
            }

            /* Get the module name. */
            char module_name[0x20];
            const bool has_module_name = GetModuleName(module_name, sizeof(module_name), process, base_address);

            /* If the process is 32-bit, just print the module. */
            if (!process->Is64Bit()) {
                return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
            }

            if (temp_32 == 0) {
                /* Module is dynamically loaded by rtld. */
                u32 mod_offset;
                if (!ReadValue(std::addressof(mod_offset), process, base_address + sizeof(u32))) {
                    return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
                }
                if (!ReadValue(std::addressof(temp_32), process, base_address + mod_offset)) {
                    return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
                }
                if (temp_32 != 0x30444F4D) { /* MOD0 */
                    return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
                }
                if (!ReadValue(std::addressof(temp_32), process, base_address + mod_offset + sizeof(u32))) {
                    return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
                }
                dyn_address = base_address + mod_offset + temp_32;
            } else if (temp_32 == 0x14000002) {
                /* Module embeds rtld. */
                if (!ReadValue(std::addressof(temp_32), process, base_address + 0x5C)) {
                    return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
                }
                if (temp_32 != 0x94000002) {
                    return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
                }
                if (!ReadValue(std::addressof(temp_32), process, base_address + 0x60)) {
                    return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
                }
                dyn_address = base_address + 0x60 + temp_32;
            } else {
                return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
            }

            /* Locate tables inside .dyn. */
            for (size_t ofs = 0; /* ... */; ofs += 0x10) {
                /* Read the DynamicTag. */
                if (!ReadValue(std::addressof(temp_64), process, dyn_address + ofs)) {
                    return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
                }
                if (temp_64 == 0) {
                    /* We're done parsing .dyn. */
                    break;
                } else if (temp_64 == 4) {
                    /* We found DT_HASH */
                    if (!ReadValue(std::addressof(temp_64), process, dyn_address + ofs + sizeof(u64))) {
                        return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
                    }
                    /* Read nchain, to get the number of symbols. */
                    if (!ReadValue(std::addressof(temp_32), process, base_address + temp_64 + sizeof(u32))) {
                        return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
                    }

                    num_sym = temp_32;
                } else if (temp_64 == 5) {
                    /* We found DT_STRTAB */
                    if (!ReadValue(std::addressof(temp_64), process, dyn_address + ofs + sizeof(u64))) {
                        return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
                    }

                    str_tab = base_address + temp_64;
                } else if (temp_64 == 6) {
                    /* We found DT_SYMTAB */
                    if (!ReadValue(std::addressof(temp_64), process, dyn_address + ofs + sizeof(u64))) {
                        return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
                    }

                    sym_tab = base_address + temp_64;
                }
            }

            /* Check that we found all the tables. */
            if (!(sym_tab != 0 && str_tab != 0 && num_sym != 0)) {
                return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
            }

            /* Try to locate an appropriate symbol. */
            for (size_t i = 0; i < num_sym; ++i) {
                /* Read the symbol from userspace. */
                struct {
                    u32 st_name;
                    u8  st_info;
                    u8  st_other;
                    u16 st_shndx;
                    u64 st_value;
                    u64 st_size;
                } sym;
                {
                    u64 x[sizeof(sym) / sizeof(u64)];
                    for (size_t j = 0; j < util::size(x); ++j) {
                        if (!ReadValue(x + j, process, sym_tab + sizeof(sym) * i + sizeof(u64) * j)) {
                            return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
                        }
                    }
                    std::memcpy(std::addressof(sym), x, sizeof(sym));
                }

                /* Check the symbol is valid/STT_FUNC. */
                if (sym.st_shndx == 0 || ((sym.st_shndx & 0xFF00) == 0xFF00)) {
                    continue;
                }
                if ((sym.st_info & 0xF) != 2) {
                    continue;
                }

                /* Check the address. */
                const uintptr_t func_start = base_address + sym.st_value;
                if (func_start <= test_address && test_address < func_start + sym.st_size) {
                    /* Read the symbol name. */
                    const uintptr_t sym_address = str_tab + sym.st_name;
                    char sym_name[0x80];
                    sym_name[util::size(sym_name) - 1] = 0;
                    for (size_t j = 0; j < util::size(sym_name) - 1; ++j) {
                        if (!ReadValue(sym_name + j, process, sym_address + j)) {
                            return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
                        }
                        if (sym_name[j] == 0) {
                            break;
                        }
                    }

                    /* Print the symbol. */
                    return PrintAddressWithSymbol(address, has_module_name, module_name, base_address, sym_name, func_start);
                }
            }

            /* Fall back to printing the module. */
            return PrintAddressWithModuleName(address, has_module_name, module_name, base_address);
        }

    }
    #endif

    void KDebug::PrintBacktrace(KThread *thread) {
        #if defined(MESOSPHERE_BUILD_FOR_DEBUGGING)
        {
            /* Treat no thread as current thread. */
            if (thread == nullptr) {
                thread = GetCurrentThreadPointer();
            }

            /* Get the exception context. */
            KExceptionContext *e_ctx = GetExceptionContext(thread);

            /* Get the owner process. */
            if (auto *process = thread->GetOwnerProcess(); process != nullptr) {
                /* Lock the owner process. */
                KScopedLightLock state_lk(process->GetStateLock());
                KScopedLightLock list_lk(process->GetListLock());

                /* Suspend all the process's threads. */
                {
                    KScopedSchedulerLock sl;

                    auto end = process->GetThreadList().end();
                    for (auto it = process->GetThreadList().begin(); it != end; ++it) {
                        if (std::addressof(*it) != GetCurrentThreadPointer()) {
                            it->RequestSuspend(KThread::SuspendType_Backtrace);
                        }
                    }
                }

                /* Print the backtrace. */
                MESOSPHERE_RELEASE_LOG("User Backtrace\n");
                if ((e_ctx->psr & 0x10) == 0) {
                    /* 64-bit thread. */
                    PrintCodeAddress(process, e_ctx->pc, false);
                    PrintCodeAddress(process, e_ctx->x[30]);

                    /* Walk the stack frames. */
                    uintptr_t fp = static_cast<uintptr_t>(e_ctx->x[29]);
                    for (auto i = 0; i < 0x20 && fp != 0 && util::IsAligned(fp, 0x10); ++i) {
                        /* Read the next frame. */
                        struct {
                            u64 fp;
                            u64 lr;
                        } stack_frame;
                        {
                            KMemoryInfo        mem_info;
                            ams::svc::PageInfo page_info;
                            KPhysicalAddress   phys_addr;

                            if (R_FAILED(process->GetPageTable().QueryInfo(std::addressof(mem_info), std::addressof(page_info), fp))) {
                                break;
                            }
                            if ((mem_info.GetState() & KMemoryState_FlagReferenceCounted) == 0) {
                                break;
                            }
                            if ((mem_info.GetAttribute() & KMemoryAttribute_Uncached) != 0) {
                                break;
                            }
                            if ((mem_info.GetPermission() & KMemoryPermission_UserRead) != KMemoryPermission_UserRead) {
                                break;
                            }
                            if (!process->GetPageTable().GetPhysicalAddress(std::addressof(phys_addr), fp)) {
                                break;
                            }
                            if (!IsHeapPhysicalAddress(phys_addr)) {
                                break;
                            }

                            u64 *frame_ptr = GetPointer<u64>(process->GetPageTable().GetHeapVirtualAddress(phys_addr));
                            stack_frame.fp = frame_ptr[0];
                            stack_frame.lr = frame_ptr[1];
                        }

                        /* Print and advance. */
                        PrintCodeAddress(process, stack_frame.lr);
                        fp = stack_frame.fp;
                    }
                } else {
                    /* 32-bit thread. */
                    PrintCodeAddress(process, e_ctx->pc, false);
                    PrintCodeAddress(process, e_ctx->x[14]);

                    /* Walk the stack frames. */
                    uintptr_t fp = static_cast<uintptr_t>(e_ctx->x[11]);
                    for (auto i = 0; i < 0x20 && fp != 0 && util::IsAligned(fp, 4); ++i) {
                        /* Read the next frame. */
                        struct {
                            u32 fp;
                            u32 lr;
                        } stack_frame;
                        {
                            KMemoryInfo        mem_info;
                            ams::svc::PageInfo page_info;
                            KPhysicalAddress   phys_addr;

                            /* Read FP */
                            if (R_FAILED(process->GetPageTable().QueryInfo(std::addressof(mem_info), std::addressof(page_info), fp))) {
                                break;
                            }
                            if ((mem_info.GetState() & KMemoryState_FlagReferenceCounted) == 0) {
                                break;
                            }
                            if ((mem_info.GetAttribute() & KMemoryAttribute_Uncached) != 0) {
                                break;
                            }
                            if ((mem_info.GetPermission() & KMemoryPermission_UserRead) != KMemoryPermission_UserRead) {
                                break;
                            }
                            if (!process->GetPageTable().GetPhysicalAddress(std::addressof(phys_addr), fp)) {
                                break;
                            }
                            if (!IsHeapPhysicalAddress(phys_addr)) {
                                break;
                            }

                            stack_frame.fp = *GetPointer<u32>(process->GetPageTable().GetHeapVirtualAddress(phys_addr));

                            /* Read LR. */
                            uintptr_t lr_ptr = (e_ctx->x[13] <= stack_frame.fp && stack_frame.fp < e_ctx->x[13] + PageSize) ? fp + 4 : fp - 4;
                            if (R_FAILED(process->GetPageTable().QueryInfo(std::addressof(mem_info), std::addressof(page_info), lr_ptr))) {
                                break;
                            }
                            if ((mem_info.GetState() & KMemoryState_FlagReferenceCounted) == 0) {
                                break;
                            }
                            if ((mem_info.GetAttribute() & KMemoryAttribute_Uncached) != 0) {
                                break;
                            }
                            if ((mem_info.GetPermission() & KMemoryPermission_UserRead) != KMemoryPermission_UserRead) {
                                break;
                            }
                            if (!process->GetPageTable().GetPhysicalAddress(std::addressof(phys_addr), lr_ptr)) {
                                break;
                            }
                            if (!IsHeapPhysicalAddress(phys_addr)) {
                                break;
                            }

                            stack_frame.lr = *GetPointer<u32>(process->GetPageTable().GetHeapVirtualAddress(phys_addr));
                        }

                        /* Print and advance. */
                        PrintCodeAddress(process, stack_frame.lr);
                        fp = stack_frame.fp;
                    }
                }

                /* Resume the threads that we suspended. */
                {
                    KScopedSchedulerLock sl;

                    auto end = process->GetThreadList().end();
                    for (auto it = process->GetThreadList().begin(); it != end; ++it) {
                        if (std::addressof(*it) != GetCurrentThreadPointer()) {
                            it->Resume(KThread::SuspendType_Backtrace);
                        }
                    }
                }
            }
        }
        #else
            MESOSPHERE_UNUSED(thread);
        #endif
    }

}
