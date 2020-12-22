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
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_debug_base.hpp>

namespace ams::kern {

    class KThread;
    class KProcess;

}

namespace ams::kern::arch::arm64 {

    class KDebug final : public KAutoObjectWithSlabHeapAndContainer<KDebug, KDebugBase> {
        MESOSPHERE_AUTOOBJECT_TRAITS(KDebug, KSynchronizationObject);
        public:
            explicit KDebug() { /* ... */ }
            virtual ~KDebug() { /* ... */ }

            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }
        public:
            virtual Result GetThreadContextImpl(ams::svc::ThreadContext *out, KThread *thread, u32 context_flags) override;
            virtual Result SetThreadContextImpl(const ams::svc::ThreadContext &ctx, KThread *thread, u32 context_flags) override;
        private:
            Result GetFpuContext(ams::svc::ThreadContext *out, KThread *thread, u32 context_flags);
            Result SetFpuContext(const ams::svc::ThreadContext &ctx, KThread *thread, u32 context_flags);
        public:
            static uintptr_t GetProgramCounter(const KThread &thread);
            static void SetPreviousProgramCounter();

            static void PrintRegister(KThread *thread = nullptr);
            static void PrintBacktrace(KThread *thread = nullptr);

            static Result BreakIfAttached(ams::svc::BreakReason break_reason, uintptr_t address, size_t size);
            static Result SetHardwareBreakPoint(ams::svc::HardwareBreakPointRegisterName name, u64 flags, u64 value);

            static constexpr bool IsBreakInstruction(u32 insn, u32 psr) {
                constexpr u32 BreakInstructionAarch64 = 0xE7FFFFFF;
                constexpr u32 BreakInstructionAarch32 = 0xE7FFDEFE;
                constexpr u32 BreakInstructionThumb32 = 0xB68E;
                if ((psr & 0x10) == 0) {
                    return insn == BreakInstructionAarch64;
                } else {
                    if ((psr & 0x20) == 0) {
                        return insn == BreakInstructionAarch32;
                    } else {
                        return insn == BreakInstructionThumb32;
                    }
                }
            }
    };

}
