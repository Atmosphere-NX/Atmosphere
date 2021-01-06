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
#include "kern_init_loader_asm.hpp"
#include "kern_init_loader_board_setup.hpp"

namespace ams::kern::init::loader {

    void PerformDefaultAarch64SpecificSetup() {
        SavedRegisterState saved_registers;
        SaveRegistersToTpidrEl1(std::addressof(saved_registers));
        ON_SCOPE_EXIT { VerifyAndClearTpidrEl1(std::addressof(saved_registers)); };

        /* Main ID specific setup. */
        cpu::MainIdRegisterAccessor midr_el1;
        if (midr_el1.GetImplementer() == cpu::MainIdRegisterAccessor::Implementer::ArmLimited) {
            /* ARM limited specific setup. */
            const auto cpu_primary_part = midr_el1.GetPrimaryPartNumber();
            const auto cpu_variant      = midr_el1.GetVariant();
            const auto cpu_revision     = midr_el1.GetRevision();
            if (cpu_primary_part == cpu::MainIdRegisterAccessor::PrimaryPartNumber::CortexA57) {
                /* Cortex-A57 specific setup. */

                /* Non-cacheable load forwarding enabled. */
                u64 cpuactlr_value  = 0x1000000;

                /* Enable the processor to receive instruction cache and TLB maintenance */
                /* operations broadcast from other processors in the cluster; */
                /* set the L2 load/store data prefetch distance to 8 requests; */
                /* set the L2 instruction fetch prefetch distance to 3 requests. */
                u64 cpuectlr_value = 0x1B00000040;

                /* Disable load-pass DMB on certain hardware variants. */
                if (cpu_variant == 0 || (cpu_variant == 1 && cpu_revision <= 1)) {
                    cpuactlr_value |= 0x800000000000000;
                }

                /* Set actlr and ectlr. */
                if (cpu::GetCpuActlrEl1() != cpuactlr_value) {
                    cpu::SetCpuActlrEl1(cpuactlr_value);
                }
                if (cpu::GetCpuEctlrEl1() != cpuectlr_value) {
                    cpu::SetCpuEctlrEl1(cpuectlr_value);
                }
            } else if (cpu_primary_part == cpu::MainIdRegisterAccessor::PrimaryPartNumber::CortexA53) {
                /* Cortex-A53 specific setup. */

                /* Set L1 data prefetch control to allow 5 outstanding prefetches; */
                /* enable device split throttle; */
                /* set the number of independent data prefetch streams to 2; */
                /* disable transient and no-read-allocate hints for loads; */
                /* set write streaming no-allocate threshold so the 128th consecutive streaming */
                /* cache line does not allocate in the L1 or L2 cache. */
                u64 cpuactlr_value = 0x90CA000;

                /* Enable hardware management of data coherency with other cores in the cluster. */
                u64 cpuectlr_value = 0x40;

                /* If supported, enable data cache clean as data cache clean/invalidate. */
                if (cpu_variant != 0 || (cpu_variant == 0 && cpu_revision > 2)) {
                    cpuactlr_value |= 0x100000000000;
                }

                /* Set actlr and ectlr. */
                if (cpu::GetCpuActlrEl1() != cpuactlr_value) {
                    cpu::SetCpuActlrEl1(cpuactlr_value);
                }
                if (cpu::GetCpuEctlrEl1() != cpuectlr_value) {
                    cpu::SetCpuEctlrEl1(cpuectlr_value);
                }
            }
        }
    }

    /* This is a default implementation, which should be overridden in a source file in board/ */
    WEAK_SYMBOL void PerformBoardSpecificSetup() {
        return PerformDefaultAarch64SpecificSetup();
    }

}
