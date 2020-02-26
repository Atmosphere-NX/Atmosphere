/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "hvisor_traps_smc.hpp"
#include "../hvisor_core_context.hpp"
#include "../cpu/hvisor_cpu_caches.hpp"

#include "../debug_manager.h"
#include "../memory_map.h"

namespace {

    void CpuOnHook(ams::hvisor::ExceptionStackFrame *frame)
    {
        // Note: preserve contexId which is passed by EL3, we'll save it later
        // Note: full affinity mask in x1. We assume there's only one cluster

        // Note: not safe if called concurrently for the same target core
        // But I guess sane code would call it with the same ep,ctxId values anyway...
        u32 cpuId = frame->ReadRegister<u32>(1);
        uintptr_t ep = frame->ReadRegister(2);
        // x3 is contextId
        if (cpuId < MAX_CORE) {
            auto &ctx = ams::hvisor::CoreContext::GetInstanceFor(cpuId);
            ctx.SetKernelEntrypoint(ep);
            frame->WriteRegister(2, g_loadImageLayout.startPa + 4); //FIXME
        }
        ams::hvisor::cpu::dmb();
    }

    inline void CpuOffHook(ams::hvisor::ExceptionStackFrame *frame)
    {
        debugManagerReportEvent(DBGEVENT_CORE_OFF);
        ams::hvisor::cpu::dmb();
    }

    void CpuSuspendHook(ams::hvisor::ExceptionStackFrame *frame, size_t epIdx)
    {
        // We may trigger warmboot, depending on powerState (x1 or default value)
        uintptr_t ep = frame->ReadRegister(epIdx);
        ams::hvisor::currentCoreCtx->SetKernelEntrypoint(ep, true);
        frame->WriteRegister(epIdx, g_loadImageLayout.startPa + 4); //FIXME

        ams::hvisor::cpu::dmb();
    }

}
namespace ams::hvisor::traps {

    void CallSmcIndirect(ExceptionStackFrame *frame, u32 smcId) {
        if (AMS_LIKELY(smcId == 0)) {
            CallSmc0(frame);
        } else if (smcId == 1) {
            CallSmc1(frame);
        } else {
            // Need to do self-modifying code here
            // Assume num < 16 to avoid a VLA and save some instructions
            size_t num = callSmcTemplateInstructionSize / 4;
            u32 codebuf[16];

            std::copy(callSmcTemplate, callSmcTemplate + num, codebuf);
            codebuf[callSmcTemplateInstructionOffset / 4] |= smcId << 5;

            cpu::HandleSelfModifyingCodePoU(codebuf, sizeof(codebuf));
            reinterpret_cast<decltype(&CallSmc0)>(codebuf)(frame);
        }
    }

    void HandleSmcTrap(ExceptionStackFrame *frame, cpu::ExceptionSyndromeRegister esr)
    {
        // TODO: Arm PSCI 0.2+ stuff
        u32 smcId = esr.iss;

        // Note: don't differenciate PSCI calls by smc immediate since HOS uses #1 for that
        // Note: funcId is actually u32 according to Arm PSCI. Not sure what to do if x0>>32 != 0.
        // NN doesn't truncate, Arm TF does.
        // Note: clear NN ABI-breaking bits
        u32 funcId = frame->x[0] & ~0xFF00;

        // Hooks go here
        switch (funcId) {
            case 0xC4000001: {
                // CPU_SUSPEND
                CpuSuspendHook(frame, 2);
                break;
            }
            case 0xC400000C:
            case 0xC400000E: {
                // CPU_DEFAULT_SUSPEND
                // SYSTEM_SUSPEND
                // (neither is implemented in NN's secure monitor...)
                CpuSuspendHook(frame, 1);
                break;
            }
            case 0x84000002: {
                // CPU_OFF
                CpuOffHook(frame);
                break;
            }
            case 0xC4000003: {
                // CPU_ON
                CpuOnHook(frame);
                break;
            }
            case 0x84000008:
            case 0x84000009:
                // SYSTEM_OFF, not implemented by Nintendo & we dont't support it in our debugger model (yet?)
                // SYSTEM_RESET, same
                break;

            default:
                // Other unimportant functions we don't care about
                break;
        }
        CallSmcIndirect(frame, smcId);
        frame->SkipInstruction(4);
    }

}
