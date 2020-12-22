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
            constexpr explicit KThreadContext() : m_callee_saved(), m_lr(), m_sp(), m_cpacr(), m_fpcr(), m_fpsr(), m_fpu_registers(), m_locked() { /* ... */ }

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
    };

    void GetUserContext(ams::svc::ThreadContext *out, const KThread *thread);

}