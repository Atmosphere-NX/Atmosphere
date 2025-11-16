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
            static constexpr size_t NumCalleeSavedRegisters    = (29 - 19) + 1;
            static constexpr size_t NumCalleeSavedFpuRegisters = 8;
            static constexpr size_t NumCallerSavedFpuRegisters = 24;
            static constexpr size_t NumFpuRegisters            = NumCalleeSavedFpuRegisters + NumCallerSavedFpuRegisters;
        public:
            union CalleeSaveRegisters {
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
            };

            union CalleeSaveFpu64Registers {
                u128 v[NumCalleeSavedFpuRegisters];
                struct {
                    u128 q8;
                    u128 q9;
                    u128 q10;
                    u128 q11;
                    u128 q12;
                    u128 q13;
                    u128 q14;
                    u128 q15;
                };
            };

            union CalleeSaveFpu32Registers {
                u128 v[NumCalleeSavedFpuRegisters / 2];
                struct {
                    u128 q4;
                    u128 q5;
                    u128 q6;
                    u128 q7;
                };
            };

            union CalleeSaveFpuRegisters {
                CalleeSaveFpu64Registers fpu64;
                CalleeSaveFpu32Registers fpu32;
            };

            union CallerSaveFpu64Registers {
                u128 v[NumCallerSavedFpuRegisters];
                struct {
                    union {
                        u128 v0_7[NumCallerSavedFpuRegisters / 3];
                        struct {
                            u128 q0;
                            u128 q1;
                            u128 q2;
                            u128 q3;
                            u128 q4;
                            u128 q5;
                            u128 q6;
                            u128 q7;
                        };
                    };
                    union {
                        u128 v16_31[2 * NumCallerSavedFpuRegisters / 3];
                        struct {
                            u128 q16;
                            u128 q17;
                            u128 q18;
                            u128 q19;
                            u128 q20;
                            u128 q21;
                            u128 q22;
                            u128 q23;
                            u128 q24;
                            u128 q25;
                            u128 q26;
                            u128 q27;
                            u128 q28;
                            u128 q29;
                            u128 q30;
                            u128 q31;
                        };
                    };
                };
            };

            union CallerSaveFpu32Registers {
                u128 v[NumCallerSavedFpuRegisters / 2];
                struct {
                    union {
                        u128 v0_3[(NumCallerSavedFpuRegisters / 3) / 2];
                        struct {
                            u128 q0;
                            u128 q1;
                            u128 q2;
                            u128 q3;
                        };
                    };
                    union {
                        u128 v8_15[(2 * NumCallerSavedFpuRegisters / 3) / 2];
                        struct {
                            u128 q8;
                            u128 q9;
                            u128 q10;
                            u128 q11;
                            u128 q12;
                            u128 q13;
                            u128 q14;
                            u128 q15;
                        };
                    };
                };
            };

            union CallerSaveFpuRegisters {
                CallerSaveFpu64Registers fpu64;
                CallerSaveFpu32Registers fpu32;
            };
        private:
            CalleeSaveRegisters m_callee_saved;
            u64 m_lr;
            u64 m_sp;
            u32 m_fpcr;
            u32 m_fpsr;
            alignas(0x10) CalleeSaveFpuRegisters m_callee_saved_fpu;
            bool m_locked;
        private:
            static void RestoreFpuRegisters64(const KThreadContext &);
            static void RestoreFpuRegisters32(const KThreadContext &);
        public:
            constexpr explicit KThreadContext(util::ConstantInitializeTag) : m_callee_saved(), m_lr(), m_sp(), m_fpcr(), m_fpsr(), m_callee_saved_fpu(), m_locked() { /* ... */ }
            explicit KThreadContext() { /* ... */ }

            void Initialize(KVirtualAddress u_pc, KVirtualAddress k_sp, KVirtualAddress u_sp, uintptr_t arg, bool is_user, bool is_64_bit, bool is_main);

            void SetArguments(uintptr_t arg0, uintptr_t arg1);

            static void FpuContextSwitchHandler(KThread *thread);

            u32 GetFpcr() const { return m_fpcr; }
            u32 GetFpsr() const { return m_fpsr; }

            void SetFpcr(u32 v) { m_fpcr = v; }
            void SetFpsr(u32 v) { m_fpsr = v; }

            void CloneFpuStatus();

            const auto &GetCalleeSaveFpuRegisters() const { return m_callee_saved_fpu; }
                  auto &GetCalleeSaveFpuRegisters()       { return m_callee_saved_fpu; }
        public:
            static void OnThreadTerminating(const KThread *thread);
        public:
            static consteval bool ValidateOffsets();

            template<typename CallerSave, typename CalleeSave> requires ((std::same_as<CallerSave, CallerSaveFpu64Registers> && std::same_as<CalleeSave, CalleeSaveFpu64Registers>) || (std::same_as<CallerSave, CallerSaveFpu32Registers> && std::same_as<CalleeSave, CalleeSaveFpu32Registers>))
            static void GetFpuRegisters(u128 *out, const CallerSave &caller_save, const CalleeSave &callee_save) {
                /* Check that the register counts are correct. */
                constexpr size_t RegisterUnitCount = util::size(CalleeSave{}.v);
                static_assert(util::size(CalleeSave{}.v) == 1 * RegisterUnitCount);
                static_assert(util::size(CallerSave{}.v) == 3 * RegisterUnitCount);

                /* Copy the low caller-save registers. */
                for (size_t i = 0; i < RegisterUnitCount; ++i) {
                    *(out++) = caller_save.v[i];
                }

                /* Copy the callee-save registers. */
                for (size_t i = 0; i < RegisterUnitCount; ++i) {
                    *(out++) = callee_save.v[i];
                }

                /* Copy the remaining caller-save registers. */
                for (size_t i = 0; i < 2 * RegisterUnitCount; ++i) {
                    *(out++) = caller_save.v[RegisterUnitCount + i];
                }
            }

            template<typename CallerSave, typename CalleeSave> requires ((std::same_as<CallerSave, CallerSaveFpu64Registers> && std::same_as<CalleeSave, CalleeSaveFpu64Registers>) || (std::same_as<CallerSave, CallerSaveFpu32Registers> && std::same_as<CalleeSave, CalleeSaveFpu32Registers>))
            static ALWAYS_INLINE void SetFpuRegisters(CallerSave &caller_save, CalleeSave &callee_save, const u128 *v) {
                /* Check that the register counts are correct. */
                constexpr size_t RegisterUnitCount = util::size(CalleeSave{}.v);
                static_assert(util::size(CalleeSave{}.v) == 1 * RegisterUnitCount);
                static_assert(util::size(CallerSave{}.v) == 3 * RegisterUnitCount);

                /* Copy the low caller-save registers. */
                for (size_t i = 0; i < RegisterUnitCount; ++i) {
                    caller_save.v[i] = *(v++);
                }

                /* Copy the callee-save registers. */
                for (size_t i = 0; i < RegisterUnitCount; ++i) {
                    callee_save.v[i] = *(v++);
                }

                /* Copy the remaining caller-save registers. */
                for (size_t i = 0; i < 2 * RegisterUnitCount; ++i) {
                    caller_save.v[RegisterUnitCount + i] = *(v++);
                }
            }
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
        static_assert(AMS_OFFSETOF(KThreadContext, m_fpcr)                   == THREAD_CONTEXT_FPCR);
        static_assert(AMS_OFFSETOF(KThreadContext, m_fpsr)                   == THREAD_CONTEXT_FPSR);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved_fpu)       == THREAD_CONTEXT_FPU_REGISTERS);
        static_assert(AMS_OFFSETOF(KThreadContext, m_locked)                 == THREAD_CONTEXT_LOCKED);

        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved_fpu.fpu64.q8 ) == THREAD_CONTEXT_FPU64_Q8 );
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved_fpu.fpu64.q9 ) == THREAD_CONTEXT_FPU64_Q9 );
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved_fpu.fpu64.q10) == THREAD_CONTEXT_FPU64_Q10);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved_fpu.fpu64.q11) == THREAD_CONTEXT_FPU64_Q11);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved_fpu.fpu64.q12) == THREAD_CONTEXT_FPU64_Q12);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved_fpu.fpu64.q13) == THREAD_CONTEXT_FPU64_Q13);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved_fpu.fpu64.q14) == THREAD_CONTEXT_FPU64_Q14);
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved_fpu.fpu64.q15) == THREAD_CONTEXT_FPU64_Q15);

        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved_fpu.fpu32.q4 ) == THREAD_CONTEXT_FPU32_Q4 );
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved_fpu.fpu32.q5 ) == THREAD_CONTEXT_FPU32_Q5 );
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved_fpu.fpu32.q6 ) == THREAD_CONTEXT_FPU32_Q6 );
        static_assert(AMS_OFFSETOF(KThreadContext, m_callee_saved_fpu.fpu32.q7 ) == THREAD_CONTEXT_FPU32_Q7 );

        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q0 ) == THREAD_FPU64_CONTEXT_Q0 );
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q1 ) == THREAD_FPU64_CONTEXT_Q1 );
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q2 ) == THREAD_FPU64_CONTEXT_Q2 );
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q3 ) == THREAD_FPU64_CONTEXT_Q3 );
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q4 ) == THREAD_FPU64_CONTEXT_Q4 );
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q5 ) == THREAD_FPU64_CONTEXT_Q5 );
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q6 ) == THREAD_FPU64_CONTEXT_Q6 );
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q7 ) == THREAD_FPU64_CONTEXT_Q7 );
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q16) == THREAD_FPU64_CONTEXT_Q16);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q17) == THREAD_FPU64_CONTEXT_Q17);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q18) == THREAD_FPU64_CONTEXT_Q18);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q19) == THREAD_FPU64_CONTEXT_Q19);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q20) == THREAD_FPU64_CONTEXT_Q20);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q21) == THREAD_FPU64_CONTEXT_Q21);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q22) == THREAD_FPU64_CONTEXT_Q22);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q23) == THREAD_FPU64_CONTEXT_Q23);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q24) == THREAD_FPU64_CONTEXT_Q24);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q25) == THREAD_FPU64_CONTEXT_Q25);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q26) == THREAD_FPU64_CONTEXT_Q26);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q27) == THREAD_FPU64_CONTEXT_Q27);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q28) == THREAD_FPU64_CONTEXT_Q28);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q29) == THREAD_FPU64_CONTEXT_Q29);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q30) == THREAD_FPU64_CONTEXT_Q30);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu64.q31) == THREAD_FPU64_CONTEXT_Q31);

        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu32.q0 ) == THREAD_FPU32_CONTEXT_Q0 );
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu32.q1 ) == THREAD_FPU32_CONTEXT_Q1 );
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu32.q2 ) == THREAD_FPU32_CONTEXT_Q2 );
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu32.q3 ) == THREAD_FPU32_CONTEXT_Q3 );
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu32.q8 ) == THREAD_FPU32_CONTEXT_Q8 );
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu32.q9 ) == THREAD_FPU32_CONTEXT_Q9 );
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu32.q10) == THREAD_FPU32_CONTEXT_Q10);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu32.q11) == THREAD_FPU32_CONTEXT_Q11);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu32.q12) == THREAD_FPU32_CONTEXT_Q12);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu32.q13) == THREAD_FPU32_CONTEXT_Q13);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu32.q14) == THREAD_FPU32_CONTEXT_Q14);
        static_assert(AMS_OFFSETOF(KThreadContext::CallerSaveFpuRegisters, fpu32.q15) == THREAD_FPU32_CONTEXT_Q15);

        return true;
    }
    static_assert(KThreadContext::ValidateOffsets());

    void GetUserContext(ams::svc::ThreadContext *out, const KThread *thread);

}