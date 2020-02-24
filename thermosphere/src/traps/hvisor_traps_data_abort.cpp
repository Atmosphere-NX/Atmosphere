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

#include "hvisor_traps_data_abort.hpp"

#include "../hvisor_virtual_gic.hpp"

/*
#include "../hvisor_core_context.hpp"
#include "../cpu/hvisor_cpu_sysreg_general.hpp"
#include "../cpu/hvisor_cpu_instructions.hpp"
*/

// Lower el

namespace ams::hvisor::traps {

    void DumpUnhandledDataAbort(cpu::DataAbortIss dabtIss, u64 far, const char *msg)
    {
        char s1[64], s2[32], s3[64] = "";
        static_cast<void>(s1);
        static_cast<void>(s2);
        static_cast<void>(s3);
        sprintf(s1, "Unhandled%s %s", msg , dabtIss.wnr ? "write" : "read");
        if (dabtIss.fnv) {
            sprintf(s2, "<unk>");
        } else {
            sprintf(s2, "%016lx", far);
        }

        if (dabtIss.isv) {
            sprintf(s3, ", size %u Rt=%u", BIT(dabtIss.sas), dabtIss.srt);
        }

        DEBUG("%s at %s%s\n", s1, s2, s3);
    }

    void HandleLowerElDataAbortException(ExceptionStackFrame *frame, cpu::ExceptionSyndromeRegister esr)
    {
        cpu::DataAbortIss dabtIss;
        u32 iss = esr.iss;
        std::memcpy(&iss, &dabtIss, 4);

        u64 far = frame->far_el2;
        u64 farpg = far & ~0xFFFull;
        size_t off = far & 0xFFF;
        size_t sz = dabtIss.GetAccessSize();

        // We don't support 8-byte writes for MMIO
        if (!dabtIss.HasValidFar() || sz > 4) {
            DumpUnhandledDataAbort(dabtIss, far, "");
        }

        if (farpg == VirtualGic::gicdPhysicalAddress) {
            if (VirtualGic::ValidateGicdRegisterAccess(off, sz)) {
                if (dabtIss.wnr) {
                    // Register write
                    u32 reg = frame->ReadFrameRegister<u32>(dabtIss.srt);
                    VirtualGic::GetInstance().WriteGicdRegister(reg, off, sz);
                } else {
                    // Reigster read
                    frame->WriteFrameRegister(dabtIss.srt, VirtualGic::GetInstance().ReadGicdRegister(off, sz));
                }
            } else {
                // Invalid GICD access
                DumpUnhandledDataAbort(dabtIss, far, "GICD");
            }
        } else {
            DumpUnhandledDataAbort(dabtIss, far, "");
        }

        // Skip instruction anyway
        frame->SkipInstruction(esr.il == 0 ? 2 : 4);
    }

}