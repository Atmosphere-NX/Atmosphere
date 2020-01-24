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

#include "smmu.h"
#include "cluster.h"
#include "mc.h"
#include "timers.h"
#include "tsec.h"

#define TSEC_KEYGEN_MAX_RETRIES 25

void *smmu_heap = (void *)SMMU_HEAP_BASE_ADDR;

static void safe_memcpy(void *dst, void *src, uint32_t sz) {
    /* Aligned memcpy to read MMIO correctly. */
    for (size_t i = 0; i < (sz/4); i++) {
        ((volatile uint32_t *)dst)[i] = ((volatile uint32_t *)src)[i];
    }
}

static void smmu_flush_ppsb() {
    /* Read-back barrier for interactions between the PPSB and the APB/AHB. */
    (void)MAKE_MC_REG(MC_SMMU_TLB_CONFIG);
}

static void smmu_flush_regs() {
    /* Flush all TLB and PTC entries. */
    MAKE_MC_REG(MC_SMMU_PTC_FLUSH) = 0;
    smmu_flush_ppsb();
    MAKE_MC_REG(MC_SMMU_TLB_FLUSH) = 0;
    smmu_flush_ppsb();
}

static void *smmu_alloc_page(uint32_t page_count) {
    void *cur_page = smmu_heap;
    smmu_heap += (page_count * SMMU_PAGE_SIZE);
    memset(cur_page, 0, (page_count * SMMU_PAGE_SIZE));
    return cur_page;
}

static uint32_t *smmu_alloc_pdir() {
    uint32_t *pdir = (uint32_t *)smmu_alloc_page(1);
    for (int pdn = 0; pdn < SMMU_PDIR_COUNT; pdn++) {
        pdir[pdn] = _PDE_VACANT(pdn);
    }
    return pdir;
}

static uint32_t *smmu_locate_pte(uint32_t *pdir_page, uint32_t iova) {
    uint32_t ptn = SMMU_ADDR_TO_PFN(iova);
    uint32_t pdn = SMMU_ADDR_TO_PDN(iova);
    uint32_t *pdir = pdir_page;
    uint32_t *ptbl;

    if (pdir[pdn] != _PDE_VACANT(pdn)) {
        /* Mapped entry table already exists. */
        ptbl = (uint32_t *)SMMU_EX_PTBL_PAGE(pdir[pdn]);
    } else {
        /* Allocate page table. */
        ptbl = (uint32_t *)smmu_alloc_page(1);
        uint32_t addr = SMMU_PDN_TO_ADDR(pdn);
        for (int pn = 0; pn < SMMU_PTBL_COUNT; pn++, addr += SMMU_PAGE_SIZE) {
            ptbl[pn] = _PTE_VACANT(addr);
        }
        pdir[pdn] = SMMU_MK_PDE((uint32_t)ptbl, _PDE_ATTR | _PDE_NEXT);
        smmu_flush_regs();
    }

    return &ptbl[ptn % SMMU_PTBL_COUNT];
}

static void smmu_map(uint32_t *pdir, uint32_t addr, uint32_t ptpage, int pcount, uint32_t pte_attr) {
    for (int i = 0; i < pcount; i++) {
        uint32_t *pte = smmu_locate_pte(pdir, addr);
        *pte = SMMU_PFN_TO_PTE(SMMU_ADDR_TO_PFN(ptpage), pte_attr);
        addr += SMMU_PAGE_SIZE;
        ptpage += SMMU_PAGE_SIZE;
    }
    smmu_flush_regs();
}

static uint32_t *smmu_setup_tsec_as(uint32_t asid) {
    /* Allocate the page directory. */
    uint32_t *pdir_page = smmu_alloc_pdir();

    /* Set the PTB ASID and point it to the PDIR. */
    MAKE_MC_REG(MC_SMMU_PTB_ASID) = asid;
    MAKE_MC_REG(MC_SMMU_PTB_DATA) = SMMU_MK_PDIR((uint32_t)pdir_page, _PDIR_ATTR);
    smmu_flush_ppsb();
    
    /* Assign the ASID to TSEC. */
    MAKE_MC_REG(MC_SMMU_TSEC_ASID) = SMMU_ASID_ENABLE((asid << 24) | (asid << 16) | (asid << 8) | asid);
    smmu_flush_ppsb();

    return pdir_page;
}

static void smmu_clear_tsec_as(uint32_t asid) {
    /* Set the PTB ASID and clear it's data. */
    MAKE_MC_REG(MC_SMMU_PTB_ASID) = asid;
    MAKE_MC_REG(MC_SMMU_PTB_DATA) = 0;
    
    /* Clear the ASID from TSEC. */
    MAKE_MC_REG(MC_SMMU_TSEC_ASID) = SMMU_ASID_DISABLE;
    smmu_flush_ppsb();
}

static void smmu_enable() {
    /* AARCH64 payload for enabling the SMMU. */
    /* Write 1 to MC_SMMU_CONFIG, read back and write the result to 0x40003F80. */
    /* This will leave the CPU waiting until 0x40003FF0 is set to Exosphère's address. */
    static const uint32_t aarch64_payload[20] = {
        0x52800020, 0x58000162, 0x58000183, 0xB9000040,
        0xB9400041, 0xB9000061, 0x58000142, 0xF9400040,
        0xF100001F, 0x54FFFFA0, 0xD61F0000, 0x00000000,
        0x70019010, 0x00000000, 0x40003F80, 0x00000000,
        0x40003FF0, 0x00000000, 0x00000000, 0x00000000
    };
    
    /* Reset Translation Enable Registers. */
    MAKE_MC_REG(MC_SMMU_TRANSLATION_ENABLE_0) = 0xFFFFFFFF;
    MAKE_MC_REG(MC_SMMU_TRANSLATION_ENABLE_1) = 0xFFFFFFFF;
    MAKE_MC_REG(MC_SMMU_TRANSLATION_ENABLE_2) = 0xFFFFFFFF;
    MAKE_MC_REG(MC_SMMU_TRANSLATION_ENABLE_3) = 0xFFFFFFFF;
    MAKE_MC_REG(MC_SMMU_TRANSLATION_ENABLE_4) = 0xFFFFFFFF;
    
    /* Setup initial TLB and PTC configuration. */
    MAKE_MC_REG(MC_SMMU_PTB_ASID) = 0;
    MAKE_MC_REG(MC_SMMU_PTB_DATA) = 0;
    MAKE_MC_REG(MC_SMMU_TLB_CONFIG) = 0x30000030;
    MAKE_MC_REG(MC_SMMU_PTC_CONFIG) = 0x2800003F;
    smmu_flush_regs();
    
    /* Power on the CCPLEX to enable the SMMU globally (requires a secure write). */
    volatile uint32_t *aarch64_payload_res = (volatile uint32_t *)(SMMU_AARCH64_PAYLOAD_ADDR + 0x80);
    memset((void *)SMMU_AARCH64_PAYLOAD_ADDR, 0, 0x100);
    memcpy((void *)SMMU_AARCH64_PAYLOAD_ADDR, aarch64_payload, 20 * 4);
    cluster_boot_cpu0(SMMU_AARCH64_PAYLOAD_ADDR);
    mdelay(500);
    if (*aarch64_payload_res != 1) {
        fatal_error("[SMMU]: Failed to enable SMMU!\n");
    }
    
    /* Write magic for nxboot. */
    *(uint32_t *)(SMMU_AARCH64_PAYLOAD_ADDR + 0xFC) = 0xDEADC0DE;
    
    /* Flush TLB and PTC entries. */
    smmu_flush_regs();
}

void smmu_emulate_tsec(void *tsec_keys, const void *package1, size_t package1_size, void *package1_dec) {
    volatile tegra_tsec_t *tsec = tsec_get_regs();
    
    /* Backup IRAM to DRAM. */
    memcpy((void *)SMMU_IRAM_BACKUP_ADDR, (void *)0x40010000, 0x30000);
    
    /* Copy package1 into IRAM. */
    memcpy((void *)0x40010000, package1, package1_size);
    
    /* Setup TSEC's address space. */
    uint32_t *pdir = smmu_setup_tsec_as(1);

    /* Allocate pages for MMIO and IRAM. */
    volatile uint32_t *car_page = smmu_alloc_page(1);
    volatile uint32_t *fuse_page = smmu_alloc_page(1);
    volatile uint32_t *pmc_page = smmu_alloc_page(1);
    volatile uint32_t *flow_page = smmu_alloc_page(1);
    volatile uint32_t *se_page = smmu_alloc_page(1);
    volatile uint32_t *mc_page = smmu_alloc_page(1);
    volatile uint32_t *iram_pages = smmu_alloc_page(48);
    volatile uint32_t *expv_page = smmu_alloc_page(1);
    
    /* Map all necessary pages. */
    smmu_map(pdir, 0x60006000, (uint32_t)car_page, 1, _READABLE | _WRITABLE | _NONSECURE);
    smmu_map(pdir, 0x7000F000, (uint32_t)fuse_page, 1, _READABLE | _NONSECURE);
    smmu_map(pdir, 0x7000E000, (uint32_t)pmc_page, 1, _READABLE | _NONSECURE);
    smmu_map(pdir, 0x60007000, (uint32_t)flow_page, 1, _WRITABLE | _NONSECURE);
    smmu_map(pdir, 0x70012000, (uint32_t)se_page, 1, _READABLE | _WRITABLE | _NONSECURE);
    smmu_map(pdir, 0x70019000, (uint32_t)mc_page, 1, _READABLE | _NONSECURE);
    smmu_map(pdir, 0x40010000, (uint32_t)iram_pages, 48, _READABLE | _WRITABLE | _NONSECURE);
    smmu_map(pdir, 0x6000F000, (uint32_t)expv_page, 1, _READABLE | _WRITABLE | _NONSECURE);

    /* Enable the SMMU. */
    smmu_enable();
    
    /* Loop retrying TSEC firmware execution, in case we lose the SE keydata race. */
    uint32_t key_buf[0x20/4] = {0};
    unsigned int retries = 0;
    while (true) {
        if (retries++ > TSEC_KEYGEN_MAX_RETRIES) {
            fatal_error("[SMMU] TSEC key generation race was lost too many times!");
        }
    
        /* Load the TSEC firmware from IRAM. */
        if (tsec_load_fw((void *)(0x40010000 + 0xE00), 0x2900) < 0) {
            fatal_error("[SMMU]: Failed to load TSEC firmware!\n");
        }
    
        /* Disable the aperture since it has precedence over the SMMU. */
        mc_disable_ahb_redirect();
        
        /* Clear all pages. */
        memset((void *)car_page, 0, SMMU_PAGE_SIZE);
        memset((void *)fuse_page, 0, SMMU_PAGE_SIZE);
        memset((void *)pmc_page, 0, SMMU_PAGE_SIZE);
        memset((void *)flow_page, 0, SMMU_PAGE_SIZE);
        memset((void *)se_page, 0, SMMU_PAGE_SIZE);
        memset((void *)mc_page, 0, SMMU_PAGE_SIZE);
        memset((void *)iram_pages, 0, 48 * SMMU_PAGE_SIZE);
        memset((void *)expv_page, 0, SMMU_PAGE_SIZE);
        
        /* Copy CAR, MC and FUSE. */
        safe_memcpy((void *)car_page, (void *)0x60006000, 0x1000);
        safe_memcpy((void *)mc_page, (void *)0x70019000, 0x1000);
        safe_memcpy((void *)&fuse_page[0x800/4], (void *)0x7000F800, 0x400);
        
        /* Copy IRAM. */
        memcpy((void *)iram_pages, (void *)0x40010000, 0x30000);
            
        /* TSEC wants CLK_RST_CONTROLLER_CLK_SOURCE_TSEC_0 to be equal to 2. */
        car_page[0x1F4/4] = 2;
        
        /* TSEC wants the aperture fully open. */
        mc_page[0x65C/4] = 0;
        mc_page[0x660/4] = 0x80000000;
        
        /* Run the TSEC firmware. */
        tsec_run_fw();
    
        /* Extract the keys from SE. */
        volatile uint32_t *key_data = (volatile uint32_t *)((void *)se_page + 0x320);
        uint32_t old_key_data = *key_data;
        uint32_t buf_counter = 0;
        while (!(tsec->TSEC_FALCON_CPUCTL & 0x10)) {
            const uint32_t new_key_data = *key_data;
            if (new_key_data != old_key_data) {
                old_key_data = new_key_data;
                key_buf[buf_counter] = new_key_data;
                buf_counter++;
            }
        }
    
        /* Enable back the aperture. */
        mc_enable_ahb_redirect();
        
        if (buf_counter == 8) {
            break;
        }
    }
    
    /* Check if the TSEC firmware wrote over the exception vectors. */
    volatile uint32_t *tsec_done_check = (volatile uint32_t *)((void *)expv_page + 0x200);
    if (!(*tsec_done_check)) {
        fatal_error("[SMMU]: Failed to emulate the TSEC firmware!\n");
    }

    /* Copy back the extracted keys. */
    memcpy((void *)tsec_keys, (void *)key_buf, 0x20);
    
    /* Manually disable TSEC clocks. */
    tsec_disable_clkrst();
    
    /* Clear TSEC's address space. */
    smmu_clear_tsec_as(1);
    
    /* Return the decrypted package1 from emulated IRAM. */
    memcpy(package1_dec, (void *)iram_pages, package1_size);
    
    /* Restore IRAM from DRAM. */
    memcpy((void *)0x40010000, (void *)SMMU_IRAM_BACKUP_ADDR, 0x30000);
}