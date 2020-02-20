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

    /* These are implemented elsewhere (asm). */
    void UserModeThreadStarter();
    void SupervisorModeThreadStarter();

    void OnThreadStart() {
        MESOSPHERE_ASSERT(!KInterruptManager::AreInterruptsEnabled());
        /* Send KDebug event for this thread's creation. */
        {
            KScopedInterruptEnable ei;
            /* TODO */
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

        uintptr_t SetupStackForUserModeThreadStarter(KVirtualAddress pc, KVirtualAddress k_sp, KVirtualAddress u_sp, uintptr_t arg, bool is_64_bit) {
            /* NOTE: Stack layout on entry looks like following:                         */
            /* SP                                                                        */
            /* |                                                                         */
            /* v                                                                         */
            /* | KExceptionContext (size 0x120) | KThread::StackParameters (size 0x30) | */
            KExceptionContext *ctx = GetPointer<KExceptionContext>(k_sp) - 1;

            /* Clear context. */
            std::memset(ctx, 0, sizeof(*ctx));

            /* Set PC and argument. */
            ctx->pc = GetInteger(pc);
            ctx->x[0] = arg;

            /* Set PSR. */
            if (is_64_bit) {
                ctx->psr = 0;
            } else {
                constexpr u64 PsrArmValue   = 0x20;
                constexpr u64 PsrThumbValue = 0x00;
                ctx->psr = ((pc & 1) == 0 ? PsrArmValue : PsrThumbValue) | (0x10);
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
            /* NOTE: Stack layout on entry looks like following:                        */
            /* SP                                                                       */
            /* |                                                                        */
            /* v                                                                        */
            /* | u64 argument | u64 entrypoint | KThread::StackParameters (size 0x30) | */
            static_assert(sizeof(KThread::StackParameters) == 0x30);

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
            this->lr = reinterpret_cast<uintptr_t>(::ams::kern::arch::arm64::UserModeThreadStarter);
            this->sp = SetupStackForUserModeThreadStarter(u_pc, k_sp, u_sp, arg, is_64_bit);
        } else {
            /* Kernel thread. */
            MESOSPHERE_ASSERT(is_64_bit);

            if (is_main) {
                /* Main thread. */
                this->lr = GetInteger(u_pc);
                this->sp = GetInteger(k_sp);
            } else {
                /* Generic Kernel thread. */
                this->lr = reinterpret_cast<uintptr_t>(::ams::kern::arch::arm64::SupervisorModeThreadStarter);
                this->sp = SetupStackForSupervisorModeThreadStarter(u_pc, k_sp, arg);
            }
        }

        /* Clear callee-saved registers. */
        for (size_t i = 0; i < util::size(this->callee_saved.registers); i++) {
            this->callee_saved.registers[i] = 0;
        }

        /* Clear FPU state. */
        this->fpcr = 0;
        this->fpsr = 0;
        this->cpacr = 0;
        for (size_t i = 0; i < util::size(this->fpu_registers); i++) {
            this->fpu_registers[i] = 0;
        }

        /* Lock the context, if we're a main thread. */
        this->locked = is_main;

        return ResultSuccess();
    }

    Result KThreadContext::Finalize() {
        /* This doesn't actually do anything. */
        return ResultSuccess();
    }

    void KThreadContext::SetArguments(uintptr_t arg0, uintptr_t arg1) {
        u64 *stack = reinterpret_cast<u64 *>(this->sp);
        stack[0] = arg0;
        stack[1] = arg1;
    }

    void KThreadContext::FpuContextSwitchHandler(KThread *thread) {
        MESOSPHERE_ASSERT(!KInterruptManager::AreInterruptsEnabled());
        MESOSPHERE_ASSERT(!IsFpuEnabled());

        /* Enable the FPU. */
        EnableFpu();

        /* Restore the FPU registers. */
        KProcess *process = thread->GetOwnerProcess();
        MESOSPHERE_ASSERT(process != nullptr);
        if (process->Is64Bit()) {
            RestoreFpuRegisters64(thread->GetContext());
        } else {
            RestoreFpuRegisters32(thread->GetContext());
        }
    }

}
