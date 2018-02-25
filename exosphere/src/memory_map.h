#ifndef EXOSPHERE_MEMORY_MAP_H
#define EXOSPHERE_MEMORY_MAP_H

#include "mmu.h"

#define ATTRIB_MEMTYPE_NORMAL MMU_PTE_BLOCK_MEMTYPE(0)
#define ATTRIB_MEMTYPE_DEVICE MMU_PTE_BLOCK_MEMTYPE(1)

static const struct {
    uintptr_t address;
    uint64_t size;
    uint64_t attributes;
    bool is_block_range;
} g_identity_mappings[] = {
    { 0x40020000,  0x20000,                                   0, false }, /* iRAM-C+D (contains the secmon's coldboot crt0) */
    { 0x7C010000,  0x10000,                                   0, false }, /* TZRAM (contains the secmon's warmboot crt0) */
    { 0x80000000, 4u << 30, MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_NS,  true }, /* DRAM (4GB) */
};

static const struct {
    uintptr_t pa;
    size_t size;
    bool is_secure;
} g_devices[] = {
    { 0x50041000, 0x1000, true  }, /* ARM Interrupt Distributor */
    { 0x50042000, 0x2000, true  }, /* Interrupt Controller Physical CPU interface */
    { 0x70006000, 0x1000, false }, /* UART-A */
    { 0x60006000, 0x1000, false }, /* Clock and Reset */
    { 0x7000E000, 0x1000, true  }, /* RTC, PMC */
    { 0x60005000, 0x1000, true  }, /* TMRs, WDTs */
    { 0x6000C000, 0x1000, true  }, /* System Registers */
    { 0x70012000, 0x2000, true  }, /* SE */
    { 0x700F0000, 0x1000, true  }, /* SYSCTR0 */
    { 0x70019000, 0x1000, true  }, /* MC */
    { 0x7000F000, 0x1000, true  }, /* FUSE (0x7000F800) */
    { 0x70000000, 0x4000, true  }, /* MISC */
    { 0x60007000, 0x1000, true  }, /* Flow Controller */
    { 0x40002000, 0x1000, true  }, /* NX bootloader mailbox page */
    { 0x7000D000, 0x1000, true  }, /* I2C-5,6 - SPI 2B-1 to 4 */
    { 0x6000D000, 0x1000, true  }, /* GPIO-1 - GPIO-8 */
    { 0x7000C000, 0x1000, true  }, /* I2C-I2C4 */
    { 0x6000F000, 0x1000, true  }, /* Exception vectors */
};

static const struct {
    uintptr_t pa;
    size_t size;
    uint64_t attributes;
} g_lp0_entry_ram_segments[] = {
    { 0x40020000, 0x10000, MMU_PTE_BLOCK_NS | ATTRIB_MEMTYPE_DEVICE }, /* Encrypted TZRAM */
    { 0x40003000, 0x01000, MMU_PTE_BLOCK_NS | ATTRIB_MEMTYPE_DEVICE }, /* LP0 entry code */
    { 0x7C010000, 0x10000, MMU_AP_PRIV_RO   | ATTRIB_MEMTYPE_NORMAL }, /* TZRAM to encrypt */
};

static const struct {
    uintptr_t pa;
    size_t size;
    uint64_t attributes;
} g_warmboot_ram_segments[] = {
    { 0x8000F000, 0x01000, MMU_PTE_BLOCK_NS | ATTRIB_MEMTYPE_DEVICE }, /* Encrypted SE state for bootROM */
    { 0x80010000, 0x10000, MMU_PTE_BLOCK_NS | ATTRIB_MEMTYPE_DEVICE }, /* Encrypted TZRAM for warmboot.bin */
};

static const struct {
    size_t tzram_offset;
    size_t map_size;
    size_t increment;       /* for alignment, guard pages, etc. */
    bool is_code_segment;   /* note: code is RWX */
} g_tzram_segments[] = {
    {           0x3000, 0x10000 - 0x2000 - 0x3000, 0x10000,  true }, /* Warmboot crt0 sections and main code segment */
    { 0x10000 - 0x2000,                    0x2000, 0x04000,  true }, /* pk2ldr segment */
    {                0,                         0, 0x02000, false }, /* SPL .bss buffer, NOT mapped at startup */
    { 0x10000 - 0x2000,                    0x1000, 0x02000, false }, /* Core 0,1,2 stack */
    { 0x10000 - 0x1000,                    0x1000, 0x02000, false }, /* Core 3 stack */
    {                0,                    0x1000, 0x02000,  true }, /* Secure Monitor exception vectors, some init stacks */
    {           0x1000,                    0x1000, 0x02000, false }, /* L2 translation table */
    {           0x2000,                    0x1000, 0x02000, false }, /* L3 translation table */
};

#define MMIO_BASE                       0x1F0080000ull
#define LP0_ENTRY_RAM_SEGMENT_BASE      (MMIO_BASE + 0x000100000)
#define WARMBOOT_RAM_SEGMENT_BASE       (LP0_ENTRY_RAM_SEGMENT_BASE + 0x000047000) /* increment seems to be arbitrary ? */
#define TZRAM_SEGMENT_BASE              (MMIO_BASE + 0x0001E0000)

#define IDENTIY_MAPPING_ID_MAX 3

#define MMIO_DEVID_GICD                 0
#define MMIO_DEVID_GICC                 1
#define MMIO_DEVID_UART_A               2
#define MMIO_DEVID_CLKRST               3
#define MMIO_DEVID_RTC_PMC              4
#define MMIO_DEVID_TMRs_WDTs            5
#define MMIO_DEVID_SYSREGS              6
#define MMIO_DEVID_SE                   7
#define MMIO_DEVID_SYSCTR0              8
#define MMIO_DEVID_MC                   9
#define MMIO_DEVID_FUSE                 10
#define MMIO_DEVID_MISC                 11
#define MMIO_DEVID_FLOWCTRL             12
#define MMIO_DEVID_NXBOOTLOADER_MAILBOX 13
#define MMIO_DEVID_I2C56_SPI2B          14
#define MMIO_DEVID_GPIO                 15
#define MMIO_DEVID_DTV_I2C234           16
#define MMIO_DEVID_EXCEPTION_VECTORS    17
#define MMIO_DEVID_MAX                  18

#define LP0_ENTRY_RAM_SEGMENT_ID_DECRYPTED_TZRAM    0
#define LP0_ENTRY_RAM_SEGMENT_ID_LP0_ENTRY_CODE     1
#define LP0_ENTRY_RAM_SEGMENT_ID_CURRENT_TZRAM      2
#define LP0_ENTRY_RAM_SEGMENT_ID_MAX                3

#define WARMBOOT_RAM_SEGMENT_ID_SE_STATE            0
#define WARMBOOT_RAM_SEGMENT_ID_TZRAM               1
#define WARMBOOT_RAM_SEGMENT_ID_MAX                 2

#define TZRAM_SEGMENT_ID_WARMBOOT_CRT0_AND_MAIN     0
#define TZRAM_SEGMENT_ID_PK2LDR                     1
#define TZRAM_SEGMENT_ID_USERPAGE                   2
#define TZRAM_SEGMENT_ID_CORE012_STACK              3
#define TZRAM_SEGMENT_ID_CORE3_STACK                4
#define TZRAM_SEGEMENT_ID_SECMON_EVT                5
#define TZRAM_SEGMENT_ID_L2_TRANSLATION_TABLE       6
#define TZRAM_SEGMENT_ID_L3_TRANSLATION_TABLE       7
#define TZRAM_SEGMENT_ID_MAX                        8

/**********************************************************************************************/

INLINE_UNROLL static inline void identity_map_all_mappings(uintptr_t *mmu_l1_tbl, uintptr_t *mmu_l3_tbl) {
    static uint64_t base_attributes = MMU_PTE_BLOCK_INNER_SHAREBLE | ATTRIB_MEMTYPE_NORMAL;
    for(size_t i = 0; i < IDENTIY_MAPPING_ID_MAX; i++) {
        uint64_t attributes = base_attributes | g_identity_mappings[i].attributes;
        if(g_identity_mappings[i].is_block_range) {
            mmu_map_block_range(1, mmu_l1_tbl, g_identity_mappings[i].address, g_identity_mappings[i].address,
                                g_identity_mappings[i].size, attributes);
        }
        else {
            mmu_map_page_range(mmu_l3_tbl, g_identity_mappings[i].address, g_identity_mappings[i].address,
                               g_identity_mappings[i].size, attributes);
        }
    }
}

INLINE_UNROLL static inline void identity_unmap_all_mappings(uintptr_t *mmu_l1_tbl, uintptr_t *mmu_l3_tbl) {
    for(size_t i = 0; i < IDENTIY_MAPPING_ID_MAX; i++) {
        if(g_identity_mappings[i].is_block_range) {
            mmu_unmap_range(1, mmu_l1_tbl, g_identity_mappings[i].address, g_identity_mappings[i].size);
        }
        else {
            mmu_unmap_range(3, mmu_l3_tbl, g_identity_mappings[i].address, g_identity_mappings[i].size);
        }
    }
}

/**********************************************************************************************/

INLINE_UNROLL static inline uintptr_t mmio_get_device_pa(unsigned int device_id) {
    return g_devices[device_id].pa;
}

#ifndef MEMORY_MAP_USE_IDENTIY_MAPPING
INLINE_UNROLL static inline const uintptr_t mmio_get_device_address(unsigned int device_id) {
    size_t offset = 0;
    for(unsigned int i = 0; i < device_id; i++) {
        offset += g_devices[i].size;
        offset += 0x1000; /* guard page */
    }

    return MMIO_BASE + offset;
}

#else
INLINE_UNROLL static inline uintptr_t mmio_get_device_address(unsigned int device_id) {
    return mmio_get_device_pa(device_id);
}
#endif

INLINE_UNROLL static inline void mmio_map_all_devices(uintptr_t *mmu_l3_tbl) {
    static const uint64_t secure_device_attributes  = MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_INNER_SHAREBLE | ATTRIB_MEMTYPE_DEVICE;
    static const uint64_t device_attributes         = MMU_PTE_BLOCK_NS | secure_device_attributes;

    for(size_t i = 0, offset = 0; i < MMIO_DEVID_MAX; i++) {
        uint64_t attributes = g_devices[i].is_secure ? secure_device_attributes : device_attributes;
        mmu_map_page_range(mmu_l3_tbl, MMIO_BASE + offset, g_devices[i].pa, g_devices[i].size, attributes);

        offset += g_devices[i].size;
        offset += 0x1000; /* insert guard page */
    }
}

INLINE_UNROLL static inline void mmio_unmap_all_devices(uintptr_t *mmu_l3_tbl) {
    for(size_t i = 0, offset = 0; i < MMIO_DEVID_MAX; i++) {
        mmu_unmap_range(3, mmu_l3_tbl, MMIO_BASE + offset, g_devices[i].size);

        offset += g_devices[i].size;
        offset += 0x1000; /* insert guard page */
    }
}

/**********************************************************************************************/

INLINE_UNROLL static inline uintptr_t lp0_get_plaintext_ram_segment_pa(unsigned int segment_id) {
    return g_lp0_entry_ram_segments[segment_id].pa;
}

#ifndef MEMORY_MAP_USE_IDENTIY_MAPPING
INLINE_UNROLL static inline uintptr_t lp0_get_plaintext_ram_segment_address(unsigned int segment_id) {
    return LP0_ENTRY_RAM_SEGMENT_BASE + 0x10000 * segment_id;
}
#else
INLINE_UNROLL static inline uintptr_t lp0_get_plaintext_ram_segment_address(unsigned int segment_id) {
    return lp0_get_plaintext_ram_segment_pa(segment_id);
}
#endif

INLINE_UNROLL static inline void lp0_map_all_plaintext_ram_segments(uintptr_t *mmu_l3_tbl) {
    for(size_t i = 0, offset = 0; i < LP0_ENTRY_RAM_SEGMENT_ID_MAX; i++) {
        uint64_t attributes =  MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_INNER_SHAREBLE | g_lp0_entry_ram_segments[i].attributes;
        mmu_map_page_range(mmu_l3_tbl, LP0_ENTRY_RAM_SEGMENT_BASE + offset, g_lp0_entry_ram_segments[i].pa,
                           g_lp0_entry_ram_segments[i].size, attributes);
        offset += 0x10000;
    }
}

INLINE_UNROLL static inline void lp0_unmap_all_plaintext_ram_segments(uintptr_t *mmu_l3_tbl) {
    for(size_t i = 0, offset = 0; i < LP0_ENTRY_RAM_SEGMENT_ID_MAX; i++) {
        mmu_unmap_range(3, mmu_l3_tbl, LP0_ENTRY_RAM_SEGMENT_BASE + offset, g_lp0_entry_ram_segments[i].size);

        offset += 0x10000;
    }
}

/**********************************************************************************************/

INLINE_UNROLL static inline uintptr_t warmboot_get_ram_segment_pa(unsigned int segment_id) {
    return g_warmboot_ram_segments[segment_id].pa;
}

#ifndef MEMORY_MAP_USE_IDENTIY_MAPPING
INLINE_UNROLL static inline uintptr_t warmboot_get_ram_segment_address(unsigned int segment_id) {
    size_t offset = 0;
    for(unsigned int i = 0; i < segment_id; i++) {
        offset += g_warmboot_ram_segments[i].size;
    }

    return WARMBOOT_RAM_SEGMENT_BASE + offset;
}
#else
INLINE_UNROLL static inline uintptr_t warmboot_get_ram_segment_address(unsigned int segment_id) {
    return warmboot_get_ram_segment_pa(segment_id);
}
#endif

INLINE_UNROLL static inline void warmboot_map_all_ram_segments(uintptr_t *mmu_l3_tbl) {
    for(size_t i = 0, offset = 0; i < WARMBOOT_RAM_SEGMENT_ID_MAX; i++) {
        uint64_t attributes =  MMU_PTE_BLOCK_XN | MMU_PTE_BLOCK_INNER_SHAREBLE | g_warmboot_ram_segments[i].attributes;
        mmu_map_page_range(mmu_l3_tbl, WARMBOOT_RAM_SEGMENT_BASE + offset, g_warmboot_ram_segments[i].pa,
                           g_warmboot_ram_segments[i].size, attributes);
        offset += g_warmboot_ram_segments[i].size;
    }
}

INLINE_UNROLL static inline void warmboot_unmap_all_ram_segments(uintptr_t *mmu_l3_tbl) {
    for(size_t i = 0, offset = 0; i < WARMBOOT_RAM_SEGMENT_ID_MAX; i++) {
        mmu_unmap_range(3, mmu_l3_tbl, WARMBOOT_RAM_SEGMENT_BASE + offset, g_warmboot_ram_segments[i].size);

        offset += g_warmboot_ram_segments[i].size;
    }
}

/**********************************************************************************************/

INLINE_UNROLL static inline uintptr_t tzram_get_segment_pa(unsigned int segment_id) {
    return 0x7C010000 + g_tzram_segments[segment_id].tzram_offset;
}

#ifndef MEMORY_MAP_USE_IDENTIY_MAPPING
INLINE_UNROLL static inline uintptr_t tzram_get_segment_address(unsigned int segment_id) {
    size_t offset = 0;
    for(unsigned int i = 0; i < segment_id; i++) {
        offset += g_tzram_segments[i].increment;
    }

    return TZRAM_SEGMENT_BASE + offset;
}
#else
INLINE_UNROLL static inline uintptr_t tzram_get_segment_address(unsigned int segment_id) {
    return tzram_get_segment_pa(segment_id);
}
#endif

INLINE_UNROLL static inline void tzram_map_all_segments(uintptr_t *mmu_l3_tbl) {
    /* Except the SPL userpage */
    for(size_t i = 0, offset = 0; i < TZRAM_SEGMENT_ID_MAX; i++) {
        uint64_t attributes = (g_tzram_segments[i].is_code_segment ? 0 : MMU_PTE_BLOCK_XN) | MMU_PTE_BLOCK_INNER_SHAREBLE | ATTRIB_MEMTYPE_NORMAL;
        if(g_tzram_segments[i].map_size == 0) {
            continue; 
        }
        mmu_map_page_range(mmu_l3_tbl, TZRAM_SEGMENT_BASE + offset, 0x7C010000 + g_tzram_segments[i].tzram_offset,
                           g_tzram_segments[i].map_size, attributes);
        offset += g_tzram_segments[i].increment;
    }
}

INLINE_UNROLL static inline void tzram_unmap_all_segments(uintptr_t *mmu_l3_tbl) {
    /* Except the SPL userpage */
    for(size_t i = 0, offset = 0; i < TZRAM_SEGMENT_ID_MAX; i++) {
        if(g_tzram_segments[i].map_size == 0) {
            continue; 
        }
        mmu_unmap_range(3, mmu_l3_tbl, TZRAM_SEGMENT_BASE + offset, g_tzram_segments[i].map_size);

        offset += g_tzram_segments[i].increment;
    }
}

#endif
