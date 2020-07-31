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

}
