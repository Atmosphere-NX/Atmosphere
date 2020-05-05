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
#include "secmon_setup.hpp"

namespace ams::secmon {

    namespace setup {

        #include "secmon_cache_impl.inc"

    }

    namespace {

        using namespace ams::mmu;

        void SetupCpuCommonControllers() {
            /* Set cpuactlr_el1. */
            {
                util::BitPack64 cpuactlr = {};
                cpuactlr.Set<hw::CpuactlrEl1CortexA57::NonCacheableStreamingEnhancement>(1);
                cpuactlr.Set<hw::CpuactlrEl1CortexA57::DisableLoadPassDmb>(1);
                HW_CPU_SET_CPUACTLR_EL1(cpuactlr);
            }

            /* Set cpuectlr_el1. */
            {
                util::BitPack64 cpuectlr = {};
                cpuectlr.Set<hw::CpuectlrEl1CortexA57::Smpen>(1);
                cpuectlr.Set<hw::CpuectlrEl1CortexA57::L2LoadStoreDataPrefetchDistance>(3);
                cpuectlr.Set<hw::CpuectlrEl1CortexA57::L2InstructionFetchPrefetchDistance>(3);
                HW_CPU_SET_CPUECTLR_EL1(cpuectlr);
            }

            /* Prevent instruction reordering. */
            hw::InstructionSynchronizationBarrier();
        }

        void SetupCpuEl3Controllers() {
            /* Set scr_el3. */
            {
                util::BitPack32 scr = {};
                scr.Set<hw::ScrEl3::Ns         >(1); /* Set EL0/EL1 as Non-Secure. */
                scr.Set<hw::ScrEl3::Irq        >(0); /* IRQs are taken in IRQ mode. */
                scr.Set<hw::ScrEl3::Fiq        >(1); /* FIQs are taken in Monitor mode. */
                scr.Set<hw::ScrEl3::Ea         >(1); /* External aborts are taken in Monitor mode. */
                scr.Set<hw::ScrEl3::Fw         >(1); /* CPSR.F is non-secure writable. */
                scr.Set<hw::ScrEl3::Aw         >(1); /* CPSR.A is non-secure writable. */
                scr.Set<hw::ScrEl3::Net        >(0); /* This bit is not implemented. */
                scr.Set<hw::ScrEl3::Smd        >(0); /* Secure Monitor Call is allowed. */
                scr.Set<hw::ScrEl3::Hce        >(0); /* Hypervisor Calls are disabled. */ /* TODO: Enable for thermosphere? */
                scr.Set<hw::ScrEl3::Sif        >(1); /* Secure mode cannot fetch instructions from non-secure memory. */
                scr.Set<hw::ScrEl3::RwCortexA53>(1); /* Reserved bit. N probably sets it because on Cortex A53, this sets kernel as aarch64. */
                scr.Set<hw::ScrEl3::StCortexA53>(0); /* Reserved bit. On Cortex A53, this sets secure registers to EL3 only. */
                scr.Set<hw::ScrEl3::Twi        >(0); /* WFI is not trapped. */
                scr.Set<hw::ScrEl3::Twe        >(0); /* WFE is not trapped. */

                HW_CPU_SET_SCR_EL3(scr);
            }

            /* Set ttbr0_el3. */
            {
                constexpr u64 ttbr0 = MemoryRegionPhysicalTzramL1PageTable.GetAddress();
                HW_CPU_SET_TTBR0_EL3(ttbr0);
            }

            /* Set tcr_el3. */
            {
                util::BitPack32 tcr = { hw::TcrEl3::Res1 };
                tcr.Set<hw::TcrEl3::T0sz >(31); /* Configure TTBR0 addressed size to be 64 GiB */
                tcr.Set<hw::TcrEl3::Irgn0>(1);  /* Configure PTE walks as inner write-back write-allocate cacheable */
                tcr.Set<hw::TcrEl3::Orgn0>(1);  /* Configure PTE walks as outer write-back write-allocate cacheable */
                tcr.Set<hw::TcrEl3::Sh0  >(3);  /* Configure PTE walks as inner shareable */
                tcr.Set<hw::TcrEl3::Tg0  >(0);  /* Set TTBR0_EL3 granule as 4 KiB */
                tcr.Set<hw::TcrEl3::Ps   >(1);  /* Set the physical addrss size as 36-bit (64 GiB) */
                tcr.Set<hw::TcrEl3::Tbi  >(0);  /* Top byte is not ignored in addrss calculations */

                HW_CPU_SET_TCR_EL3(tcr);
            }

            /* Clear cptr_el3. */
            {
                util::BitPack32 cptr = {};

                cptr.Set<hw::CptrEl3::Tfp  >(0); /* FP/SIMD instructions don't trap. */
                cptr.Set<hw::CptrEl3::Tta  >(0); /* Reserved bit (no trace functionality present). */
                cptr.Set<hw::CptrEl3::Tcpac>(0); /* Access to cpacr_El1 does not trap. */

                HW_CPU_SET_CPTR_EL3(cptr);
            }

            /* Set mair_el3. */
            {
                u64 mair = (MemoryRegionAttributes_Normal << (MemoryRegionAttributeWidth * MemoryAttributeIndexNormal)) |
                           (MemoryRegionAttributes_Device << (MemoryRegionAttributeWidth * MemoryAttributeIndexDevice));
                HW_CPU_SET_MAIR_EL3(mair);
            }

            /* Set vectors. */
            {
                constexpr u64 vectors = MemoryRegionVirtualTzramProgramExceptionVectors.GetAddress();
                HW_CPU_SET_VBAR_EL3(vectors);
            }

            /* Prevent instruction re-ordering around this point. */
            hw::InstructionSynchronizationBarrier();
        }

        void EnableMmu() {
            /* Create sctlr value. */
            util::BitPack32 sctlr = { hw::SctlrEl3::Res1 };
            sctlr.Set<hw::SctlrEl3::M>(1);   /* Globally enable the MMU. */
            sctlr.Set<hw::SctlrEl3::A>(0);   /* Disable alignment fault checking. */
            sctlr.Set<hw::SctlrEl3::C>(1);   /* Globally enable the data and unified caches. */
            sctlr.Set<hw::SctlrEl3::Sa>(0);  /* Disable stack alignment checking. */
            sctlr.Set<hw::SctlrEl3::I>(1);   /* Globally enable the instruction cache. */
            sctlr.Set<hw::SctlrEl3::Wxn>(0); /* Do not force writable pages to be ExecuteNever. */
            sctlr.Set<hw::SctlrEl3::Ee>(0);  /* Exceptions should be little endian. */

            /* Ensure all writes are done before turning on the mmu. */
            hw::DataSynchronizationBarrierInnerShareable();

            /* Invalidate the entire tlb. */
            hw::InvalidateEntireTlb();

            /* Ensure instruction consistency. */
            hw::DataSynchronizationBarrierInnerShareable();
            hw::InstructionSynchronizationBarrier();

            /* Set sctlr_el3. */
            HW_CPU_SET_SCTLR_EL3(sctlr);
            hw::InstructionSynchronizationBarrier();
        }

    }

    void SetupCpuMemoryControllersEnableMmu() {
        SetupCpuCommonControllers();
        SetupCpuEl3Controllers();
        EnableMmu();
    }

    void SetupSocDmaControllers() {
        /* Ensure that our caches are managed. */
        setup::InvalidateEntireDataCache();
        setup::EnsureInstructionConsistency();

        /* Lock tsec. */
        tsec::Lock();

        /* Enable SWID[0] for all bits. */
        reg::Write(AHB_ARBC(AHB_MASTER_SWID),   ~0u);

        /* Clear SWID1 for all bits. */
        reg::Write(AHB_ARBC(AHB_MASTER_SWID_1),  0u);

        /* Set MSELECT config to set WRAP_TO_INCR_SLAVE0(APC) | WRAP_TO_INCR_SLAVE1(PCIe) | WRAP_TO_INCR_SLAVE2(GPU) */
        /* and clear ERR_RESP_EN_SLAVE1(PCIe) | ERR_RESP_EN_SLAVE2(GPU) */
        {
            auto mselect_cfg = reg::Read(MSELECT(MSELECT_CONFIG));
            mselect_cfg &= ~(1u << 24); /* Clear ERR_RESP_EN_SLAVE1(PCIe)  */
            mselect_cfg &= ~(1u << 25); /* Clear ERR_RESP_EN_SLAVE2(GPU)   */
            mselect_cfg |=  (1u << 27); /* Set   WRAP_TO_INCR_SLAVE0(APC)  */
            mselect_cfg |=  (1u << 28); /* Set   WRAP_TO_INCR_SLAVE1(PCIe) */
            mselect_cfg |=  (1u << 29); /* Set   WRAP_TO_INCR_SLAVE2(GPU)  */
            reg::Write(MSELECT(MSELECT_CONFIG), mselect_cfg);
        }

        /* Disable USB, USB2, AHB-DMA from arbitration. */
        {
            auto arb_dis = reg::Read(AHB_ARBC(AHB_ARBITRATION_DISABLE));
            arb_dis |= (1u <<  5); /* Disable AHB-DMA */
            arb_dis |= (1u <<  6); /* Disable USB */
            arb_dis |= (1u << 18); /* Disable USB2 */
            reg::Write(AHB_ARBC(AHB_ARBITRATION_DISABLE), arb_dis);
        }

        /* Select high priority group with priority 7. */
        {
            u32 priority_ctrl = {};
            priority_ctrl |= (7u << 29); /* Set group 7. */
            priority_ctrl |= (1u <<  0); /* Set high priority. */
            reg::Write(AHB_ARBC(AHB_ARBITRATION_PRIORITY_CTRL), priority_ctrl);
        }

        /* Prevent splitting AHB writes to TZRAM. */
        {
            reg::Write(AHB_ARBC(AHB_GIZMO_TZRAM), (1u << 7));
        }
    }

}