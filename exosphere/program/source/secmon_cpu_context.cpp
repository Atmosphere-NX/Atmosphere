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
#include <exosphere.hpp>
#include "secmon_cpu_context.hpp"
#include "secmon_error.hpp"

namespace ams::secmon {

    namespace {

        struct DebugRegisters {
            u32 osdttrx_el1;
            u32 osdtrtx_el1;
            u32 mdscr_el1;
            u32 oseccr_el1;
            u32 mdccint_el1;
            u32 dbgclaimclr_el1;
            u32 dbgvcr32_el2;
            u32 sder32_el3;
            u32 mdcr_el2;
            u32 mdcr_el3;
            u32 spsr_el3;
            u64 dbgbvcr_el1[12];
            u64 dbgwvcr_el1[ 8];
        };

        struct CoreContext {
            EntryContext   entry_context;
            bool           is_on;
            bool           is_reset_expected;
            bool           is_debug_registers_saved;
            DebugRegisters debug_registers;
        };

        void SaveDebugRegisters(DebugRegisters &dr) {
            /* Set the OS lock; this will be unlocked by entry code. */
            HW_CPU_SET_OSLAR_EL1(1);

            /* Save general debug registers. */
            HW_CPU_GET_OSDTRRX_EL1    (dr.osdttrx_el1);
            HW_CPU_GET_OSDTRTX_EL1    (dr.osdtrtx_el1);
            HW_CPU_GET_MDSCR_EL1      (dr.mdscr_el1);
            HW_CPU_GET_OSECCR_EL1     (dr.oseccr_el1);
            HW_CPU_GET_MDCCINT_EL1    (dr.mdccint_el1);
            HW_CPU_GET_DBGCLAIMCLR_EL1(dr.dbgclaimclr_el1);
            HW_CPU_GET_DBGVCR32_EL2   (dr.dbgvcr32_el2);
            HW_CPU_GET_SDER32_EL3     (dr.sder32_el3);
            HW_CPU_GET_MDCR_EL2       (dr.mdcr_el2);
            HW_CPU_GET_MDCR_EL3       (dr.mdcr_el3);
            HW_CPU_GET_SPSR_EL3       (dr.spsr_el3);

            /* Save debug breakpoints. */
            HW_CPU_GET_DBGBVR0_EL1(dr.dbgbvcr_el1[ 0]);
            HW_CPU_GET_DBGBCR0_EL1(dr.dbgbvcr_el1[ 1]);
            HW_CPU_GET_DBGBVR1_EL1(dr.dbgbvcr_el1[ 2]);
            HW_CPU_GET_DBGBCR1_EL1(dr.dbgbvcr_el1[ 3]);
            HW_CPU_GET_DBGBVR2_EL1(dr.dbgbvcr_el1[ 4]);
            HW_CPU_GET_DBGBCR2_EL1(dr.dbgbvcr_el1[ 5]);
            HW_CPU_GET_DBGBVR3_EL1(dr.dbgbvcr_el1[ 6]);
            HW_CPU_GET_DBGBCR3_EL1(dr.dbgbvcr_el1[ 7]);
            HW_CPU_GET_DBGBVR4_EL1(dr.dbgbvcr_el1[ 8]);
            HW_CPU_GET_DBGBCR4_EL1(dr.dbgbvcr_el1[ 9]);
            HW_CPU_GET_DBGBVR5_EL1(dr.dbgbvcr_el1[10]);
            HW_CPU_GET_DBGBCR5_EL1(dr.dbgbvcr_el1[11]);

            /* Save debug watchpoints. */
            HW_CPU_GET_DBGWVR0_EL1(dr.dbgwvcr_el1[0]);
            HW_CPU_GET_DBGWCR0_EL1(dr.dbgwvcr_el1[1]);
            HW_CPU_GET_DBGWVR1_EL1(dr.dbgwvcr_el1[2]);
            HW_CPU_GET_DBGWCR1_EL1(dr.dbgwvcr_el1[3]);
            HW_CPU_GET_DBGWVR2_EL1(dr.dbgwvcr_el1[4]);
            HW_CPU_GET_DBGWCR2_EL1(dr.dbgwvcr_el1[5]);
            HW_CPU_GET_DBGWVR3_EL1(dr.dbgwvcr_el1[6]);
            HW_CPU_GET_DBGWCR3_EL1(dr.dbgwvcr_el1[7]);
        }

        void RestoreDebugRegisters(const DebugRegisters &dr) {
            /* Restore general debug registers. */
            HW_CPU_SET_OSDTRRX_EL1    (dr.osdttrx_el1);
            HW_CPU_SET_OSDTRTX_EL1    (dr.osdtrtx_el1);
            HW_CPU_SET_MDSCR_EL1      (dr.mdscr_el1);
            HW_CPU_SET_OSECCR_EL1     (dr.oseccr_el1);
            HW_CPU_SET_MDCCINT_EL1    (dr.mdccint_el1);
            HW_CPU_SET_DBGCLAIMCLR_EL1(dr.dbgclaimclr_el1);
            HW_CPU_SET_DBGVCR32_EL2   (dr.dbgvcr32_el2);
            HW_CPU_SET_SDER32_EL3     (dr.sder32_el3);
            HW_CPU_SET_MDCR_EL2       (dr.mdcr_el2);
            HW_CPU_SET_MDCR_EL3       (dr.mdcr_el3);
            HW_CPU_SET_SPSR_EL3       (dr.spsr_el3);

            /* Restore debug breakpoints. */
            HW_CPU_SET_DBGBVR0_EL1(dr.dbgbvcr_el1[ 0]);
            HW_CPU_SET_DBGBCR0_EL1(dr.dbgbvcr_el1[ 1]);
            HW_CPU_SET_DBGBVR1_EL1(dr.dbgbvcr_el1[ 2]);
            HW_CPU_SET_DBGBCR1_EL1(dr.dbgbvcr_el1[ 3]);
            HW_CPU_SET_DBGBVR2_EL1(dr.dbgbvcr_el1[ 4]);
            HW_CPU_SET_DBGBCR2_EL1(dr.dbgbvcr_el1[ 5]);
            HW_CPU_SET_DBGBVR3_EL1(dr.dbgbvcr_el1[ 6]);
            HW_CPU_SET_DBGBCR3_EL1(dr.dbgbvcr_el1[ 7]);
            HW_CPU_SET_DBGBVR4_EL1(dr.dbgbvcr_el1[ 8]);
            HW_CPU_SET_DBGBCR4_EL1(dr.dbgbvcr_el1[ 9]);
            HW_CPU_SET_DBGBVR5_EL1(dr.dbgbvcr_el1[10]);
            HW_CPU_SET_DBGBCR5_EL1(dr.dbgbvcr_el1[11]);

            /* Restore debug watchpoints. */
            HW_CPU_SET_DBGWVR0_EL1(dr.dbgwvcr_el1[0]);
            HW_CPU_SET_DBGWCR0_EL1(dr.dbgwvcr_el1[1]);
            HW_CPU_SET_DBGWVR1_EL1(dr.dbgwvcr_el1[2]);
            HW_CPU_SET_DBGWCR1_EL1(dr.dbgwvcr_el1[3]);
            HW_CPU_SET_DBGWVR2_EL1(dr.dbgwvcr_el1[4]);
            HW_CPU_SET_DBGWCR2_EL1(dr.dbgwvcr_el1[5]);
            HW_CPU_SET_DBGWVR3_EL1(dr.dbgwvcr_el1[6]);
            HW_CPU_SET_DBGWCR3_EL1(dr.dbgwvcr_el1[7]);
        }

        constinit CoreContext g_core_contexts[NumCores] = {};

    }

    bool IsCoreOn(int core) {
        return g_core_contexts[core].is_on;
    }

    void SetCoreOff() {
        g_core_contexts[hw::GetCurrentCoreId()].is_on = false;
    }

    bool IsResetExpected() {
        return g_core_contexts[hw::GetCurrentCoreId()].is_reset_expected;
    }

    void SetResetExpected(int core, bool expected) {
        g_core_contexts[core].is_reset_expected = expected;
    }

    void SetResetExpected(bool expected) {
        SetResetExpected(hw::GetCurrentCoreId(), expected);
    }

    void SetEntryContext(int core, uintptr_t address, uintptr_t arg) {
        g_core_contexts[core].entry_context.pc = address;
        g_core_contexts[core].entry_context.x0 = arg;
    }

    void GetEntryContext(EntryContext *out) {
        auto &ctx = g_core_contexts[hw::GetCurrentCoreId()];

        const auto pc = ctx.entry_context.pc;
        const auto x0 = ctx.entry_context.x0;

        if (pc == 0 || ctx.is_on) {
            SetError(pkg1::ErrorInfo_InvalidCoreContext);
            AMS_ABORT("Invalid core context");
        }

        ctx.entry_context = {};
        ctx.is_on         = true;

        out->pc = pc;
        out->x0 = x0;
    }

    void SaveDebugRegisters() {
        auto &ctx = g_core_contexts[hw::GetCurrentCoreId()];

        SaveDebugRegisters(ctx.debug_registers);
        ctx.is_debug_registers_saved = true;
    }

    void RestoreDebugRegisters() {
        auto &ctx = g_core_contexts[hw::GetCurrentCoreId()];

        if (ctx.is_debug_registers_saved) {
            RestoreDebugRegisters(ctx.debug_registers);
            ctx.is_debug_registers_saved = false;
        }
    }

}
