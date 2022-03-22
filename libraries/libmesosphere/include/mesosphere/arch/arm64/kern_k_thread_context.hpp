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
#pragma once
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_typed_address.hpp>

namespace ams::kern {

    class KThread;

}

namespace ams::kern::arch::arm64 {

    class KThreadContext {
        public:
            static constexpr size_t NumCalleeSavedRegisters = (29 - 19) + 1;
            static constexpr size_t NumFpuRegisters = 32;
        private:
            union {
                u64 registers[NumCalleeSavedRegisters];
                struct {
                    u64 x19;
                    u64 x20;
                    u64 x21;
                    u64 x22;
                    u64 x23;
                    u64 x24;
                    u64 x25;
                    u64 x26;
                    u64 x27;
                    u64 x28;
                    u64 x29;
                };
            } m_callee_saved;
            u64 m_lr;
            u64 m_sp;
            u64 m_cpacr;
            u64 m_fpcr;
            u64 m_fpsr;
            alignas(0x10) u128 m_fpu_registers[NumFpuRegisters];
            bool m_locked;
        private:
            static void RestoreFpuRegisters64(const KThreadContext &);
            static void RestoreFpuRegisters32(const KThreadContext &);
        public:
            constexpr explicit KThreadContext(util::ConstantInitializeTag) : m_callee_saved(), m_lr(), m_sp(), m_cpacr(), m_fpcr(), m_fpsr(), m_fpu_registers(), m_locked() { /* ... */ }
            explicit KThreadContext() { /* ... */ }

            Result Initialize(KVirtualAddress u_pc, KVirtualAddress k_sp, KVirtualAddress u_sp, uintptr_t arg, bool is_user, bool is_64_bit, bool is_main);
            Result Finalize();

            void SetArguments(uintptr_t arg0, uintptr_t arg1);

            static void FpuContextSwitchHandler(KThread *thread);

            u32 GetFpcr() const { return m_fpcr; }
            u32 GetFpsr() const { return m_fpsr; }

            void SetFpcr(u32 v) { m_fpcr = v; }
            void SetFpsr(u32 v) { m_fpsr = v; }

            void CloneFpuStatus();

            void SetFpuRegisters(const u128 *v, bool is_64_bit);

            const u128 *GetFpuRegisters() const { return m_fpu_registers; }
        public:
            static void OnThreadTerminating(const KThread *thread);
        public:
            static consteval bool ValidateOffsets();
    };

    consteval bool KThreadContext::ValidateOffsets() {
        static_assert(sizeof(KThreadContext) == THREAD_CONTEXT_SIZE);

        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved.registers) == THREAD_CONTEXT_CPU_REGISTERS);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved.x19)       == THREAD_CONTEXT_X19);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved.x20)       == THREAD_CONTEXT_X20);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved.x21)       == THREAD_CONTEXT_X21);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved.x22)       == THREAD_CONTEXT_X22);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved.x23)       == THREAD_CONTEXT_X23);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved.x24)       == THREAD_CONTEXT_X24);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved.x25)       == THREAD_CONTEXT_X25);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved.x26)       == THREAD_CONTEXT_X26);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved.x27)       == THREAD_CONTEXT_X27);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved.x28)       == THREAD_CONTEXT_X28);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved.x29)       == THREAD_CONTEXT_X29);
        static_assert(AMS_OFFSETOF(KThreadContext, m_lr)                     == THREAD_CONTEXT_LR);
        static_assert(AMS_OFFSETOF(KThreadContext, m_sp)                     == THREAD_CONTEXT_SP);
        static_assert(AMS_OFFSETOF(KThreadContext, m_cpacr)                  == THREAD_CONTEXT_CPACR);
        static_assert(AMS_OFFSETOF(KThreadContext, m_fpcr)                   == THREAD_CONTEXT_FPCR);
        static_assert(AMS_OFFSETOF(KThreadContext, m_fpsr)                   == THREAD_CONTEXT_FPSR);
        static_assert(AMS_OFFSETOF(KThreadContext, m_fpu_registers)          == THREAD_CONTEXT_FPU_REGISTERS);
        static_assert(AMS_OFFSETOF(KThreadContext, m_locked)                 == THREAD_CONTEXT_LOCKED);

        return true;
    }
    static_assert(KThreadContext::ValidateOffsets());


    void GetUserContext(ams::svc::ThreadContext *out, const KThread *thread);

}