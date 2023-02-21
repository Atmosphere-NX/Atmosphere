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

namespace ams::kern::arch::arm64 {

    /* These are implemented elsewhere (asm). */
    void UserModeThreadStarter();
    void SupervisorModeThreadStarter();

    void OnThreadStart() {
        MESOSPHERE_ASSERT(!KInterruptManager::AreInterruptsEnabled());
        /* Send KDebug event for this thread's creation. */
        {
            KScopedInterruptEnable ei;

            const uintptr_t params[2] = { GetCurrentThread().GetId(), GetInteger(GetCurrentThread().GetThreadLocalRegionAddress()) };
            KDebug::OnDebugEvent(ams::svc::DebugEvent_CreateThread, params, util::size(params));
        }

        /* Handle any pending dpc. */
        while (GetCurrentThread().HasDpc()) {
            KDpcManager::HandleDpc();
        }

        /* Clear our status as in an exception handler */
        GetCurrentThread().ClearInExceptionHandler();
    }

    namespace {

        ALWAYS_INLINE bool IsFpuEnabled() {
            return cpu::ArchitecturalFeatureAccessControlRegisterAccessor().IsFpEnabled();
        }

        ALWAYS_INLINE void EnableFpu() {
            cpu::ArchitecturalFeatureAccessControlRegisterAccessor().SetFpEnabled(true).Store();
            cpu::InstructionMemoryBarrier();
        }

        uintptr_t SetupStackForUserModeThreadStarter(KVirtualAddress pc, KVirtualAddress k_sp, KVirtualAddress u_sp, uintptr_t arg, const bool is_64_bit) {
            /* NOTE: Stack layout on entry looks like following:                          */
            /* SP                                                                         */
            /* |                                                                          */
            /* v                                                                          */
            /* | KExceptionContext (size 0x120) | KThread::StackParameters (size 0x130) | */
            KExceptionContext *ctx = GetPointer<KExceptionContext>(k_sp) - 1;

            /* Clear context. */
            std::memset(ctx, 0, sizeof(*ctx));

            /* Set PC and argument. */
            ctx->pc = GetInteger(pc) & ~(UINT64_C(1));
            ctx->x[0] = arg;

            /* Set PSR. */
            if (is_64_bit) {
                ctx->psr = 0;
            } else {
                constexpr u64 PsrArmValue   = 0x00;
                constexpr u64 PsrThumbValue = 0x20;
                ctx->psr = ((pc & 1) == 0 ? PsrArmValue : PsrThumbValue) | (0x10);
                MESOSPHERE_LOG("Creating User 32-Thread, %016lx\n", GetInteger(pc));
            }

            /* Set CFI-value. */
            if (is_64_bit) {
                ctx->x[18] = KSystemControl::GenerateRandomU64() | 1;
            }

            /* Set stack pointer. */
            if (is_64_bit) {
                ctx->sp    = GetInteger(u_sp);
            } else {
                ctx->x[13] = GetInteger(u_sp);
            }

            return reinterpret_cast<uintptr_t>(ctx);
        }

        uintptr_t SetupStackForSupervisorModeThreadStarter(KVirtualAddress pc, KVirtualAddress sp, uintptr_t arg) {
            /* NOTE: Stack layout on entry looks like following:                         */
            /* SP                                                                        */
            /* |                                                                         */
            /* v                                                                         */
            /* | u64 argument | u64 entrypoint | KThread::StackParameters (size 0x140) | */
            static_assert(sizeof(KThread::StackParameters) == 0x140);

            u64 *stack = GetPointer<u64>(sp);
            *(--stack) = GetInteger(pc);
            *(--stack) = arg;
            return reinterpret_cast<uintptr_t>(stack);
        }

    }

    Result KThreadContext::Initialize(KVirtualAddress u_pc, KVirtualAddress k_sp, KVirtualAddress u_sp, uintptr_t arg, bool is_user, bool is_64_bit, bool is_main) {
        MESOSPHERE_ASSERT(k_sp != Null<KVirtualAddress>);

        /* Ensure that the stack pointers are aligned. */
        k_sp = util::AlignDown(GetInteger(k_sp), 16);
        u_sp = util::AlignDown(GetInteger(u_sp), 16);

        /* Determine LR and SP. */
        if (is_user) {
            /* Usermode thread. */
            m_lr = reinterpret_cast<uintptr_t>(::ams::kern::arch::arm64::UserModeThreadStarter);
            m_sp = SetupStackForUserModeThreadStarter(u_pc, k_sp, u_sp, arg, is_64_bit);
        } else {
            /* Kernel thread. */
            MESOSPHERE_ASSERT(is_64_bit);

            if (is_main) {
                /* Main thread. */
                m_lr = GetInteger(u_pc);
                m_sp = GetInteger(k_sp);
            } else {
                /* Generic Kernel thread. */
                m_lr = reinterpret_cast<uintptr_t>(::ams::kern::arch::arm64::SupervisorModeThreadStarter);
                m_sp = SetupStackForSupervisorModeThreadStarter(u_pc, k_sp, arg);
            }
        }

        /* Clear callee-saved registers. */
        for (size_t i = 0; i < util::size(m_callee_saved.registers); i++) {
            m_callee_saved.registers[i] = 0;
        }

        /* Clear FPU state. */
        m_fpcr = 0;
        m_fpsr = 0;
        for (size_t i = 0; i < util::size(m_callee_saved_fpu.fpu64.v); ++i) {
            m_callee_saved_fpu.fpu64.v[i] = 0;
        }

        /* Lock the context, if we're a main thread. */
        m_locked = is_main;

        R_SUCCEED();
    }

    void KThreadContext::SetArguments(uintptr_t arg0, uintptr_t arg1) {
        u64 *stack = reinterpret_cast<u64 *>(m_sp);
        stack[0] = arg0;
        stack[1] = arg1;
    }

    void KThreadContext::CloneFpuStatus() {
        u64 pcr, psr;
        cpu::InstructionMemoryBarrier();

        KScopedInterruptDisable di;

        if (IsFpuEnabled()) {
            __asm__ __volatile__("mrs %[pcr], fpcr" : [pcr]"=r"(pcr) :: "memory");
            __asm__ __volatile__("mrs %[psr], fpsr" : [psr]"=r"(psr) :: "memory");
        } else {
            pcr = GetCurrentThread().GetContext().GetFpcr();
            psr = GetCurrentThread().GetContext().GetFpsr();
        }

        this->SetFpcr(pcr);
        this->SetFpsr(psr);
    }

    void GetUserContext(ams::svc::ThreadContext *out, const KThread *thread) {
        MESOSPHERE_ASSERT(KScheduler::IsSchedulerLockedByCurrentThread());
        MESOSPHERE_ASSERT(thread->IsSuspended());
        MESOSPHERE_ASSERT(thread->GetOwnerProcess() != nullptr);

        /* Get the contexts. */
        const KExceptionContext *e_ctx = GetExceptionContext(thread);
        const KThreadContext    *t_ctx = std::addressof(thread->GetContext());

        if (thread->GetOwnerProcess()->Is64Bit()) {
            /* Set special registers. */
            out->fp     = e_ctx->x[29];
            out->lr     = e_ctx->x[30];
            out->sp     = e_ctx->sp;
            out->pc     = e_ctx->pc;
            out->pstate = e_ctx->psr & cpu::El0Aarch64PsrMask;

            /* Get the thread's general purpose registers. */
            if (thread->IsCallingSvc()) {
                for (size_t i = 19; i < 29; ++i) {
                    out->r[i] = e_ctx->x[i];
                }
                if (e_ctx->write == 0) {
                    out->pc -= sizeof(u32);
                }
            } else {
                for (size_t i = 0; i < 29; ++i) {
                    out->r[i] = e_ctx->x[i];
                }
            }

            /* Copy tpidr. */
            out->tpidr = e_ctx->tpidr;

            /* Copy fpu registers. */
            static_assert(util::size(ams::svc::ThreadContext{}.v) == KThreadContext::NumFpuRegisters);
            static_assert(KThreadContext::NumCallerSavedFpuRegisters == KThreadContext::NumCalleeSavedFpuRegisters * 3);
            static_assert(KThreadContext::NumFpuRegisters == KThreadContext::NumCallerSavedFpuRegisters + KThreadContext::NumCalleeSavedFpuRegisters);
            const auto &caller_save_fpu = thread->GetCallerSaveFpuRegisters().fpu64;
            const auto &callee_save_fpu = t_ctx->GetCalleeSaveFpuRegisters().fpu64;

            if (!thread->IsCallingSvc() || thread->IsInUsermodeExceptionHandler()) {
                KThreadContext::GetFpuRegisters(out->v, caller_save_fpu, callee_save_fpu);
            } else {
                for (size_t i = 0; i < KThreadContext::NumCalleeSavedFpuRegisters; ++i) {
                    out->v[(KThreadContext::NumCallerSavedFpuRegisters / 3) + i] = caller_save_fpu.v[i];
                }
            }
        } else {
            /* Set special registers. */
            out->pc     = static_cast<u32>(e_ctx->pc);
            out->pstate = e_ctx->psr & cpu::El0Aarch32PsrMask;

            /* Get the thread's general purpose registers. */
            for (size_t i = 0; i < 15; ++i) {
                out->r[i] = static_cast<u32>(e_ctx->x[i]);
            }

            /* Adjust PC, if the thread is calling svc. */
            if (thread->IsCallingSvc()) {
                if (e_ctx->write == 0) {
                    /* Adjust by 2 if thumb mode, 4 if arm mode. */
                    out->pc -= ((e_ctx->psr & 0x20) == 0) ? sizeof(u32) : sizeof(u16);
                }
            }

            /* Copy tpidr. */
            out->tpidr = static_cast<u32>(e_ctx->tpidr);

            /* Copy fpu registers. */
            static_assert(util::size(ams::svc::ThreadContext{}.v) == KThreadContext::NumFpuRegisters);
            static_assert(KThreadContext::NumCallerSavedFpuRegisters == KThreadContext::NumCalleeSavedFpuRegisters * 3);
            static_assert(KThreadContext::NumFpuRegisters == KThreadContext::NumCallerSavedFpuRegisters + KThreadContext::NumCalleeSavedFpuRegisters);
            const auto &caller_save_fpu = thread->GetCallerSaveFpuRegisters().fpu32;
            const auto &callee_save_fpu = t_ctx->GetCalleeSaveFpuRegisters().fpu32;

            if (!thread->IsCallingSvc() || thread->IsInUsermodeExceptionHandler()) {
                KThreadContext::GetFpuRegisters(out->v, caller_save_fpu, callee_save_fpu);
            } else {
                for (size_t i = 0; i < KThreadContext::NumCalleeSavedFpuRegisters / 2; ++i) {
                    out->v[((KThreadContext::NumCallerSavedFpuRegisters / 3) / 2) + i] = caller_save_fpu.v[i];
                }
            }
        }

        /* Copy fpcr/fpsr. */
        out->fpcr = t_ctx->GetFpcr();
        out->fpsr = t_ctx->GetFpsr();
    }

    void KThreadContext::OnThreadTerminating(const KThread *thread) {
        MESOSPHERE_UNUSED(thread);
        /* ... */
    }

}
