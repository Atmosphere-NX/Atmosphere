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

#include "hvisor_traps_sysreg.hpp"
#include "../hvisor_core_context.hpp"
#include "../hvisor_guest_timers.hpp"
#include "../cpu/hvisor_cpu_caches.hpp"

using namespace ams::hvisor;
using namespace ams::hvisor::traps;
using namespace ams::hvisor::cpu;

namespace {

    inline u64 DoSystemRegisterRead(const ExceptionStackFrame *frame, u32 normalizedIss)
    {
        // See https://developer.arm.com/architectures/learn-the-architecture/generic-timer/single-page

        u64 val;
        switch (normalizedIss) {
            case EncodeSysregIss(cntpct_el0): {
                u64 vct = ComputeCntvct(frame);
                val = vct;
                break;
            }
            case EncodeSysregIss(cntp_tval_el0): {
                u64 vct  = frame->cntpct_el0 - currentCoreCtx->GetTotalTimeInHypervisor();
                u64 cval = currentCoreCtx->GetEmulPtimerCval();
                val = (cval - vct) & 0xFFFFFFFF;
                break;
            }
            case EncodeSysregIss(cntp_ctl_el0): {
                val = frame->cntp_ctl_el0;
                break;
            }
            case EncodeSysregIss(cntp_cval_el0): {
                val = currentCoreCtx->GetEmulPtimerCval();
                break;
            }
            // NOTE: We should trap ID_AA64* register to lie to the guest about e.g. MemTag but it would take too much space
            default: {
                // We shouldn't have trapped on other registers other than debug regs
                // and we want the latter as RA0/WI
                val = 0;
                break;
            }
        }

        return val;
    }

    inline void DoSystemRegisterWrite(ExceptionStackFrame *frame, u32 normalizedIss, u64 val)
    {
        // See https://developer.arm.com/architectures/learn-the-architecture/generic-timer/single-page

        switch (normalizedIss) {
            case EncodeSysregIss(cntp_tval_el0): {
                // Sign-extend
                s32 signedVal = static_cast<s32>(val);
                u64 vct = ComputeCntvct(frame);
                WriteEmulatedPhysicalCompareValue(frame, vct + signedVal);
                break;
            }
            case EncodeSysregIss(cntp_ctl_el0): {
                frame->cntp_ctl_el0 = val;
                break;
            }
            case EncodeSysregIss(cntp_cval_el0): {
                WriteEmulatedPhysicalCompareValue(frame, val);
                break;
            }

            // If we didn't trap it, dc isw would behave like dc cisw because stage2 translations are enabled.
            // Turning dc csw into cisw is also harmless.
            case EncodeSysregIss(dc_csw):
            case EncodeSysregIss(dc_isw):
            case EncodeSysregIss(dc_cisw): {
                HandleTrappedSetWayOperation(static_cast<u32>(val));
                break;
            }

            default: {
                // We shouldn't have trapped on other registers other than debug regs
                // and we want the latter as RA0/WI
                break;
            }
        }
    }

    inline void DoMrs(ExceptionStackFrame *frame, u32 normalizedIss, u32 reg)
    {
        frame->WriteRegister(reg, DoSystemRegisterRead(frame, normalizedIss));
        frame->SkipInstruction(4);
    }

    inline void DoMsr(ExceptionStackFrame *frame, u32 normalizedIss, u32 reg)
    {
        DoSystemRegisterWrite(frame, normalizedIss, frame->ReadRegister(reg));
        frame->SkipInstruction(4);
    }

    inline void DoMrc(ExceptionStackFrame *frame, u32 normalizedIss, u32 instructionLength, u32 reg)
    {
        frame->WriteRegister(reg, DoSystemRegisterRead(frame, normalizedIss) & 0xFFFFFFFF);
        frame->SkipInstruction(instructionLength);
    }

    inline void DoMcr(ExceptionStackFrame *frame, u32 normalizedIss, u32 instructionLength, u32 reg)
    {
        DoSystemRegisterWrite(frame, normalizedIss, frame->ReadRegister<u32>(reg));
        frame->SkipInstruction(instructionLength);
    }

    inline void DoMrrc(ExceptionStackFrame *frame, u32 normalizedIss, u32 instructionLength, u32 reg, u32 reg2)
    {
        u64 val = DoSystemRegisterRead(frame, normalizedIss);
        frame->WriteRegister(reg, val & 0xFFFFFFFF);
        frame->WriteRegister(reg2, val >> 32);
        frame->SkipInstruction(instructionLength);
    }

    inline void DoMcrr(ExceptionStackFrame *frame, u32 normalizedIss, u32 instructionLength, u32 reg, u32 reg2)
    {
        u64 valLo = frame->ReadRegister(reg)  & 0xFFFFFFFF;
        u64 valHi = frame->ReadRegister(reg2) << 32;
        DoSystemRegisterWrite(frame, normalizedIss, valHi | valLo);
        frame->SkipInstruction(instructionLength);
    }

    inline bool EvaluateMcrMrcCondition(const ExceptionStackFrame *frame, u32 condition, bool condValid)
    {
        if (!condValid) {
            // Only T32 instructions can do that
            u32 it = frame->GetT32ItFlags();
            return it == 0 || frame->EvaluateConditionCode(it >> 4);
        } else {
            return frame->EvaluateConditionCode(condition);
        }
    }

}

namespace ams::hvisor::traps {

    void HandleMsrMrsTrap(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
    {
        u32 iss = esr.iss;
        u32 reg = (iss >> 5) & 31;
        bool isRead = (iss & 1) != 0;

        // Clear register field and set direction field to 'normalize' the ISS
        iss &= ~((0x1F << 5) | 1);

        if (isRead) {
            DoMrs(frame, iss, reg);
        } else {
            DoMsr(frame, iss, reg);
        }
    }

    void HandleMcrMrcCP15Trap(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
    {
        u32 iss = esr.iss;
        u32 instructionLength = esr.il == 0 ? 2 : 4;

        if (!EvaluateMcrMrcCondition(frame, (iss >> 20) & 0xF, (iss & BIT(24)) != 0)) {
            // If instruction not valid/condition code says no
            frame->SkipInstruction(instructionLength);
            return;
        }

        u32 opc2 = (iss >> 17) & 7;
        u32 opc1 = (iss >> 14) & 7;
        u32 CRn  = (iss >> 10) & 15;
        u32 Rt   = (iss >>  5) & 31;
        u32 CRm  = (iss >>  1) & 15;
        bool isRead = (iss & 1) != 0;

        ENSURE2(
            opc1 == 0 && CRn == 14 && CRm == 2 && opc2 <= 1,
            "unexpected cp15 register, instruction: %s p15, #%u, r%u, c%u, c%u, #%u\n",
            isRead ? "mrc" : "mcr", opc1, Rt, CRn, CRm, opc2
        );

        iss = opc2 == 0 ? EncodeSysregIss(cntp_tval_el0) : EncodeSysregIss(cntp_ctl_el0);

        if (isRead) {
            DoMrc(frame, iss, instructionLength, Rt);
        } else {
            DoMcr(frame, iss, instructionLength, Rt);
        }
    }

    void HandleMcrrMrrcCP15Trap(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
    {
        u32 iss = esr.iss;
        u32 instructionLength = esr.il == 0 ? 2 : 4;

        if (!EvaluateMcrMrcCondition(frame, (iss >> 20) & 0xF, (iss & BIT(24)) != 0)) {
            // If instruction not valid/condition code says no
            frame->SkipInstruction(instructionLength);
            return;
        }

        u32 opc1 = (iss >> 16) & 15;
        u32 Rt2  = (iss >> 10) & 31;
        u32 Rt   = (iss >>  5) & 31;
        u32 CRm  = (iss >>  1) & 15;

        bool isRead = (iss & 1) != 0;

        ENSURE2(
            CRm == 14 && (opc1 == 0 || opc1 == 2),
            "handleMcrrMrrcTrap: unexpected cp15 register, instruction: %s p15, #%u, r%u, r%u, c%u\n",
            isRead ? "mrrc" : "mcrr", opc1, Rt, Rt, CRm
        );

        iss = opc1 == 0 ? EncodeSysregIss(cntpct_el0) : EncodeSysregIss(cntp_cval_el0);

        if (isRead) {
            DoMrrc(frame, iss, instructionLength, Rt, Rt2);
        } else {
            DoMcrr(frame, iss, instructionLength, Rt, Rt2);
        }
    }

    void HandleA32CP14Trap(ExceptionStackFrame *frame, ExceptionSyndromeRegister esr)
    {
        // LDC/STC: Skip instruction, read 0 if necessary, since only one debug reg can be accessed with it
        // Other CP14 accesses: do the same thing

        if (esr.iss & 1 && EvaluateMcrMrcCondition(frame, (esr.iss >> 20) & 0xF, (esr.iss & BIT(24)) != 0)) {
            frame->WriteRegister((esr.iss >> 5) & 0x1F, 0);
        }
        frame->SkipInstruction(esr.il == 0 ? 2 : 4);
    }

}
