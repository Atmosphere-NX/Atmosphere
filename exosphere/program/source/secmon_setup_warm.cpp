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

        constexpr inline uintptr_t MC = MemoryRegionPhysicalDeviceMemoryController.GetAddress();

        using namespace ams::mmu;

        constexpr inline PageTableMappingAttribute MappingAttributesEl3SecureRwCode = AddMappingAttributeIndex(PageTableMappingAttributes_El3SecureRwCode, MemoryAttributeIndexNormal);

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
            util::BitPack64 sctlr = { hw::SctlrEl3::Res1 };
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

        bool IsExitLp0() {
            return reg::Read(MC + MC_SECURITY_CFG3) == 0;
        }

        constexpr void AddPhysicalTzramIdentityMappingImpl(u64 *l1, u64 *l2, u64 *l3) {
            /* Define extents. */
            const uintptr_t start_address = MemoryRegionPhysicalTzram.GetAddress();
            const size_t size             = MemoryRegionPhysicalTzram.GetSize();
            const uintptr_t end_address   = start_address + size;

            /* Flush cache for the L3 page table entries. */
            {
                const uintptr_t start = GetL3EntryIndex(start_address);
                const uintptr_t end   = GetL3EntryIndex(end_address);
                for (uintptr_t i = start; i < end; i += hw::DataCacheLineSize / sizeof(*l3)) {
                    if (!std::is_constant_evaluated()) { hw::FlushDataCacheLine(l3 + i); }
                }
            }

            /* Flush cache for the L2 page table entry. */
            if (!std::is_constant_evaluated()) { hw::FlushDataCacheLine(l2 + GetL2EntryIndex(start_address)); }

            /* Flush cache for the L1 page table entry. */
            if (!std::is_constant_evaluated()) { hw::FlushDataCacheLine(l1 + GetL1EntryIndex(start_address)); }

            /* Add the L3 mappings. */
            SetL3BlockEntry(l3, start_address, start_address, size, MappingAttributesEl3SecureRwCode);

            /* Add the L2 entry for the physical tzram region. */
            SetL2TableEntry(l2, MemoryRegionPhysicalTzramL2.GetAddress(), MemoryRegionPhysicalTzramL2L3PageTable.GetAddress(), PageTableTableAttributes_El3SecureCode);

            /* Add the L1 entry for the physical region. */
            SetL1TableEntry(l1, MemoryRegionPhysical.GetAddress(), MemoryRegionPhysicalTzramL2L3PageTable.GetAddress(), PageTableTableAttributes_El3SecureCode);
            static_assert(GetL1EntryIndex(MemoryRegionPhysical.GetAddress()) == 1);

            /* Invalidate the data cache for the L3 page table entries. */
            {
                const uintptr_t start = GetL3EntryIndex(start_address);
                const uintptr_t end   = GetL3EntryIndex(end_address);
                for (uintptr_t i = start; i < end; i += hw::DataCacheLineSize / sizeof(*l3)) {
                    if (!std::is_constant_evaluated()) { hw::InvalidateDataCacheLine(l3 + i); }
                }
            }

            /* Flush cache for the L2 page table entry. */
            if (!std::is_constant_evaluated()) { hw::InvalidateDataCacheLine(l2 + GetL2EntryIndex(start_address)); }

            /* Flush cache for the L1 page table entry. */
            if (!std::is_constant_evaluated()) { hw::InvalidateDataCacheLine(l1 + GetL1EntryIndex(start_address)); }
        }

        void RestoreDebugCode() {
            #if defined(AMS_BUILD_FOR_DEBUGGING) || defined(AMS_BUILD_FOR_AUDITING)
            {
                const u64 *src = MemoryRegionPhysicalDramDebugDataStore.GetPointer<u64>();
                volatile u64 *dst = MemoryRegionPhysicalDebugCode.GetPointer<u64>();
                for (size_t i = 0; i < MemoryRegionPhysicalDramDebugDataStore.GetSize() / sizeof(u64); ++i) {
                    dst[i] = src[i];
                }
            }
            #endif
        }

        void AddPhysicalTzramIdentityMapping() {
            /* Get page table extents. */
            u64 * const l1    = MemoryRegionPhysicalTzramL1PageTable.GetPointer<u64>();
            u64 * const l2_l3 = MemoryRegionPhysicalTzramL2L3PageTable.GetPointer<u64>();

            /* Add the mapping. */
            AddPhysicalTzramIdentityMappingImpl(l1, l2_l3, l2_l3);

            /* Ensure that mappings are consistent. */
            setup::EnsureMappingConsistency();
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
            reg::ReadWrite(MSELECT(MSELECT_CONFIG), MSELECT_REG_BITS_ENUM(CONFIG_ERR_RESP_EN_SLAVE1,  DISABLE),
                                                    MSELECT_REG_BITS_ENUM(CONFIG_ERR_RESP_EN_SLAVE2,  DISABLE),
                                                    MSELECT_REG_BITS_ENUM(CONFIG_WRAP_TO_INCR_SLAVE0,  ENABLE),
                                                    MSELECT_REG_BITS_ENUM(CONFIG_WRAP_TO_INCR_SLAVE1,  ENABLE),
                                                    MSELECT_REG_BITS_ENUM(CONFIG_WRAP_TO_INCR_SLAVE2,  ENABLE));
        }

        /* Disable USB, USB2, AHB-DMA from arbitration. */
        {
            reg::ReadWrite(AHB_ARBC(AHB_ARBITRATION_DISABLE), AHB_REG_BITS_ENUM(ARBITRATION_DISABLE_AHBDMA, DISABLE),
                                                              AHB_REG_BITS_ENUM(ARBITRATION_DISABLE_USB,    DISABLE),
                                                              AHB_REG_BITS_ENUM(ARBITRATION_DISABLE_USB2,   DISABLE));
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

        /* NOTE: This is Mariko only in Nintendo's firmware. */
        /* Still, it seems to have no adverse effects on Erista... */
        /* TODO: Find a way to get access to SocType this early (fuse driver isn't alive yet), only write on mariko? */
        {
            reg::ReadWrite(AHB_ARBC(AHB_AHB_SPARE_REG), AHB_REG_BITS_VALUE(AHB_SPARE_REG_AHB_SPARE_REG, 0xE0000));
        }
    }

    void SetupSocDmaControllersCpuMemoryControllersEnableMmuWarmboot() {
        /* If this is being called from lp0 exit, we want to setup the soc dma controllers. */
        if (IsExitLp0()) {
            RestoreDebugCode();
            SetupSocDmaControllers();
        }

        /* Add a physical TZRAM identity map. */
        AddPhysicalTzramIdentityMapping();

        /* Initialize cpu memory controllers and the MMU. */
        SetupCpuMemoryControllersEnableMmu();
    }

}