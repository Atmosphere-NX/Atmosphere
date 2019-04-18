/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
 
#include "utils.h"
#include "memory_map.h"
#include "mc.h"
#include "arm.h"
#include "synchronization.h"
#include "exocfg.h"
#include "pmc.h"

#undef  MC_BASE
#define MC_BASE (MMIO_GET_DEVICE_PA(MMIO_DEVID_MC))

#define WARMBOOT_GET_TZRAM_SEGMENT_PA(x) ((g_exosphere_target_firmware_for_init < ATMOSPHERE_TARGET_FIRMWARE_500) \
                                            ? TZRAM_GET_SEGMENT_PA(x) : TZRAM_GET_SEGMENT_5X_PA(x))

/* start.s */
void __set_memory_registers(uintptr_t ttbr0, uintptr_t vbar, uint64_t cpuectlr, uint32_t scr,
                            uint32_t tcr, uint32_t cptr, uint64_t mair, uint32_t sctlr);

unsigned int g_exosphere_target_firmware_for_init = 0;

uintptr_t get_warmboot_crt0_stack_address(void) {
    return WARMBOOT_GET_TZRAM_SEGMENT_PA(TZRAM_SEGMENT_ID_CORE012_STACK) + 0x800;
}

uintptr_t get_warmboot_crt0_stack_address_critsec_enter(void) {
    unsigned int core_id = get_core_id();

    if (core_id == 3) {
        return WARMBOOT_GET_TZRAM_SEGMENT_PA(TZRAM_SEGMENT_ID_CORE3_STACK) + 0x1000;
    } else {
        return WARMBOOT_GET_TZRAM_SEGMENT_PA(TZRAM_SEGEMENT_ID_SECMON_EVT) + 0x80 * (core_id + 1);
    }
}

void warmboot_crt0_critical_section_enter(volatile critical_section_t *critical_section) {
    critical_section_enter(critical_section);
}

void init_dma_controllers(unsigned int target_firmware) {
    if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_400) {
        /* Set some unknown registers in HOST1X. */
        MAKE_REG32(0x500038F8) &= 0xFFFFFFFE;
        MAKE_REG32(0x50003300) = 0;

        /* AHB_MASTER_SWID_0 - Enable SWID[0] for all bits. */
        MAKE_REG32(0x6000C018) = 0xFFFFFFFF;

        /* AHB_MASTER_SWID_1 */
        MAKE_REG32(0x6000C038) = 0x0;

        /* MSELECT_CONFIG_0 |= WRAP_TO_INCR_SLAVE0(APC) | WRAP_TO_INCR_SLAVE1(PCIe) | WRAP_TO_INCR_SLAVE2(GPU) */
        MAKE_REG32(0x50060000) |= 0x38000000;

        /* AHB_ARBITRATION_DISABLE_0 - Disables USB, USB2, and AHB-DMA from arbitration */
        MAKE_REG32(0x6000C004) = 0x40060;

        /* AHB_ARBITRATION_PRIORITY_CTRL_0 - Select high prio group with prio 7 */
        MAKE_REG32(0x6000C008) = 0xE0000001;

        /* AHB_GIZMO_TZRAM_0 |= DONT_SPLIT_AHB_WR */
        MAKE_REG32(0x6000C054) = 0x80;
    } else {
        /* SYSCTR0_CNTCR_0 = ENABLE | HALT_ON_DEBUG (write-once init) */
        MAKE_REG32(0x700F0000) = 3;

        /* Set some unknown registers in HOST1X. */
        MAKE_REG32(0x500038F8) &= 0xFFFFFFFE;
        MAKE_REG32(0x50003300) = 0;

        /* AHB_MASTER_SWID_0 */
        MAKE_REG32(0x6000C018) = 0;

        /* AHB_MASTER_SWID_1 - Makes USB1/USB2 use SWID[1] */
        MAKE_REG32(0x6000C038) = 0x40040;

        /* APBDMA_CHANNEL_SWID_0 = ~0 (SWID = 1 for all APB-DMA channels) */
        MAKE_REG32(0x6002003C) = 0xFFFFFFFF;

        /* APBDMA_CHANNEL_SWID1_0 = 0 (See above) */
        MAKE_REG32(0x60020054) = 0;

        /* APBDMA_SECURITY_REG_0 = 0 (All APB-DMA channels non-secure) */
        MAKE_REG32(0x60020038) = 0;

        /* MSELECT_CONFIG_0 |= WRAP_TO_INCR_SLAVE0(APC) | WRAP_TO_INCR_SLAVE1(PCIe) | WRAP_TO_INCR_SLAVE2(GPU) */
        MAKE_REG32(0x50060000) |= 0x38000000;

        /* AHB_ARBITRATION_PRIORITY_CTRL_0 - Select high prio group with prio 7 */
        MAKE_REG32(0x6000C008) = 0xE0000001;

        /* AHB_GIZMO_TZRAM_0 |= DONT_SPLIT_AHB_WR */
        MAKE_REG32(0x6000C054) = 0x80;
    }
}

void _set_memory_registers_enable_mmu(const uintptr_t ttbr0) {
    static const uintptr_t vbar  = TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGEMENT_ID_SECMON_EVT) + 0x800;

    /*
        - Disable table walk descriptor access prefetch.
        - L2 instruction fetch prefetch distance = 3 (reset value)
        - L2 load/store data prefetch distance = 8 (reset value)
        - Enable the processor to receive instruction cache and TLB maintenance operations broadcast from other processors in the cluster
    */
    static const uint64_t cpuectlr = 0x1B00000040ull;

    /*
        - The next lower level is Aarch64
        - Secure instruction fetch (when the PE is in Secure state, this bit disables instruction fetch from Non-secure memory)
        - External Abort/SError taken to EL3
        - FIQ taken to EL3
        - NS (EL0 and EL1 are nonsecure)
    */
    static const uint32_t scr = 0x63D;

    /*
        - PA size: 36-bit (64 GB)
        - Granule size: 4KB
        - Shareability attribute for memory associated with translation table walks using TTBR0_EL3: Inner Shareable
        - Outer cacheability attribute for memory associated with translation table walks using TTBR0_EL3: Normal memory, Outer Write-Back Read-Allocate Write-Allocate Cacheable
        - Inner cacheability attribute for memory associated with translation table walks using TTBR0_EL3: Normal memory, Inner Write-Back Read-Allocate Write-Allocate Cacheable
        - T0SZ = 31 (33-bit address space)
    */
    static const uint32_t tcr = TCR_EL3_RSVD | TCR_PS(1) | TCR_TG0_4K | TCR_SHARED_INNER | TCR_ORGN_WBWA | TCR_IRGN_WBWA | TCR_T0SZ(33);

    /* Nothing trapped */
    static const uint32_t cptr = 0;

    /*
        - Attribute 0: Normal memory, Inner and Outer Write-Back Read-Allocate Write-Allocate Non-transient
        - Attribute 1: Device-nGnRE memory
        - Other attributes: Device-nGnRnE memory
    */
    static const uint64_t mair = 0x4FFull;

    /*
        - Cacheability control, for EL3 instruction accesses DISABLED
        (- SP Alignment check bit NOT SET)
        - Cacheability control, for EL3 data accesses DISABLED (normal memory accesses from EL3 are cacheable)
        (- Alignement check bit NOT SET)
        - MMU enabled for EL3 stage 1 address translation
    */
    static const uint32_t sctlr = 0x30C51835ull;

    __set_memory_registers(ttbr0, vbar, cpuectlr, scr, tcr, cptr, mair, sctlr);
}

void set_memory_registers_enable_mmu_1x_ttbr0(void) {
    static const uintptr_t ttbr0 = TZRAM_GET_SEGMENT_PA(TZRAM_SEGEMENT_ID_SECMON_EVT) + 0x800 - 64;
    _set_memory_registers_enable_mmu(ttbr0);
}

void set_memory_registers_enable_mmu_5x_ttbr0(void) {
    static const uintptr_t ttbr0 = TZRAM_GET_SEGMENT_5X_PA(TZRAM_SEGEMENT_ID_SECMON_EVT) + 0x800 - 64;
    _set_memory_registers_enable_mmu(ttbr0);
}

#if 0 /* Since we decided not to identity-unmap TZRAM */
static void identity_remap_tzram(void) {
    /* See also: configure_ttbls (in coldboot_init.c). */
    uintptr_t *mmu_l1_tbl = (uintptr_t *)(WARMBOOT_GET_TZRAM_SEGMENT_PA(TZRAM_SEGEMENT_ID_SECMON_EVT) + 0x800 - 64);
    uintptr_t *mmu_l2_tbl = (uintptr_t *)WARMBOOT_GET_TZRAM_SEGMENT_PA(TZRAM_SEGMENT_ID_L2_TRANSLATION_TABLE);
    uintptr_t *mmu_l3_tbl = (uintptr_t *)WARMBOOT_GET_TZRAM_SEGMENT_PA(TZRAM_SEGMENT_ID_L3_TRANSLATION_TABLE);

    mmu_map_table(1, mmu_l1_tbl, 0x40000000, mmu_l2_tbl, 0);
    mmu_map_table(2, mmu_l2_tbl, 0x7C000000, mmu_l3_tbl, 0);

    identity_map_mapping(mmu_l1_tbl, mmu_l3_tbl, IDENTITY_GET_MAPPING_ADDRESS(IDENTITY_MAPPING_TZRAM),
                         IDENTITY_GET_MAPPING_SIZE(IDENTITY_MAPPING_TZRAM), IDENTITY_GET_MAPPING_ATTRIBS(IDENTITY_MAPPING_TZRAM),
                         IDENTITY_IS_MAPPING_BLOCK_RANGE(IDENTITY_MAPPING_TZRAM));
}
#endif

void warmboot_init(void) {
    /*
        From https://events.static.linuxfound.org/sites/events/files/slides/slides_17.pdf :
        Caches may write back dirty lines at any time:
            - To make space for new allocations
            - Even if MMU is off
            - Even if Cacheable accesses are disabled (caches are never 'off')
    */
    flush_dcache_all();
    invalidate_icache_all();
    
    /* On warmboot (not cpu_on) only */
    if (VIRT_MC_SECURITY_CFG3 == 0) {
        init_dma_controllers(g_exosphere_target_firmware_for_init);
    }
    
    /*identity_remap_tzram();*/
    /* Nintendo pointlessly fully invalidate the TLB & invalidate the data cache on the modified ranges here */
    if (g_exosphere_target_firmware_for_init < ATMOSPHERE_TARGET_FIRMWARE_500) {
        set_memory_registers_enable_mmu_1x_ttbr0();
    } else {
        set_memory_registers_enable_mmu_5x_ttbr0();
    }
}
