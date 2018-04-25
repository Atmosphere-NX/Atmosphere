#ifndef EXOSPHERE_MC_H
#define EXOSPHERE_MC_H

#include <stdint.h>
#include "memory_map.h"

/* Exosphere driver for the Tegra X1 Memory Controller. */

static inline uintptr_t get_mc_base(void) {
    return MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_MC);
}

#define MC_BASE  (get_mc_base())

#define MAKE_MC_REG(n) MAKE_REG32(MC_BASE + n)

#define MC_SMMU_PTB_ASID_0 MAKE_MC_REG(MC_REGISTER_SMMU_PTB_ASID_0)
#define MC_SMMU_PTB_DATA_0 MAKE_MC_REG(MC_REGISTER_SMMU_PTB_DATA_0)
#define MC_SMMU_AVPC_ASID_0 MAKE_MC_REG(MC_REGISTER_SMMU_AVPC_ASID_0)
#define MC_SMMU_PPCS1_ASID_0 MAKE_MC_REG(MC_REGISTER_SMMU_PPCS1_ASID_0)

#define MC_SECURITY_CFG0_0 MAKE_MC_REG(MC_REGISTER_SECURITY_BOM)
#define MC_SECURITY_CFG1_0 MAKE_MC_REG(MC_REGISTER_SECURITY_SIZE_MB)
#define MC_SECURITY_CFG3_0 MAKE_MC_REG(MC_REGISTER_SECURITY_BOM_HI)


#define CARVEOUT_ID_MIN 1
#define CARVEOUT_ID_MAX 5

#define KERNEL_CARVEOUT_SIZE_MAX 0x1FFE0000

typedef struct {
    uint32_t allowed_clients;
    uint32_t paddr_low;
    uint32_t paddr_high;
    uint32_t size_big_pages;
    uint32_t flags_0;
    uint32_t flags_1;
    uint32_t flags_2;
    uint32_t flags_3;
    uint32_t flags_4;
    uint32_t flags_5;
    uint32_t flags_6;
    uint32_t flags_7;
    uint32_t flags_8;
    uint32_t flags_9;
    uint8_t padding[0x28];
} security_carveout_t;

/* 18.11.1 MC Registers */
typedef enum {
    MC_REGISTER_SMMU_CONFIG_0 = 0x10,
    MC_REGISTER_SMMU_TLB_CONFIG_0 = 0x14, /* Controls usage of the TLB */
    MC_REGISTER_SMMU_PTC_CONFIG_0 = 0x18, /* Controls usage of the PTC */
    MC_REGISTER_SMMU_PTB_ASID_0 = 0x1C,
    MC_REGISTER_SMMU_PTB_DATA_0 = 0x20,

    MC_REGISTER_SMMU_TLB_FLUSH_0 = 0x30,
    MC_REGISTER_SMMU_PTC_FLUSH_0 = 0x34,
    MC_REGISTER_0x38 = 0x38,
    MC_REGISTER_0x3C = 0x3C,

    /* SECURITY_BOM is the base of the secured region, limited to MB granularity.
    This must point to a region of the physical address map allocated to EMEM for it to be effective; the MC cannot secure address space it does not own. (In other words, this is an absolute address, not an offset.)
    Above is the list of clients with the TrustZone-security access. [18.11.1.20]
    Note that AXICIF clients will adhere to the standard AXI protocol "aprot[1]==0" indication for secure requests. */
    MC_REGISTER_SECURITY_BOM = 0x70,
    MC_REGISTER_SECURITY_CFG0_0 = MC_REGISTER_SECURITY_BOM,

    MC_REGISTER_SECURITY_SIZE_MB = 0x74, /* SECURITY_SIZE_MB is the size, in megabytes, of the secured region. If set to 0, the security check in MC is disabled */
    MC_REGISTER_SECURITY_CFG1_0 = MC_REGISTER_SECURITY_SIZE_MB,

    MC_REGISTER_EMEM_ARB_RING1_THROTTLE_0 = 0xE0,
    MC_REGISTER_EMEM_ARB_RING3_THROTTLE_0 = 0xE4,
    MC_REGISTER_EMEM_ARB_OVERRIDE_0 = 0xE8,
    MC_REGISTER_EMEM_ARB_RSV_0 = 0xEC,
    MC_REGISTER_0xF0 = 0xF0,
    MC_REGISTER_CLKEN_OVERRIDE_0 = 0xF4,

    MC_REGISTER_SMMU_TRANSLATION_ENABLE_0_0 = 0x228,
    MC_REGISTER_SMMU_TRANSLATION_ENABLE_1_0 = 0x22C,
    MC_REGISTER_SMMU_TRANSLATION_ENABLE_2_0 = 0x230,
    MC_REGISTER_SMMU_TRANSLATION_ENABLE_3_0 = 0x234,

    MC_REGISTER_SMMU_AVPC_ASID_0 = 0x23C,

    MC_REGISTER_SMMU_PPCS1_ASID_0 = 0x298,

    MC_REGISTER_0x648 = 0x648,
    MC_REGISTER_0x64C = 0x64C,
    MC_REGISTER_0x650 = 0x650,

    MC_REGISTER_0x65C = 0x65C,
    MC_REGISTER_0x660 = 0x660,

    MC_REGISTER_IRAM_REG_CTRL_0 = 0x964,
    MC_REGISTER_EMEM_CFG_ACCESS_CTRL_0 = MC_REGISTER_IRAM_REG_CTRL_0,

    MC_REGISTER_SEC_CARVEOUT_BOM_0 = 0x670, /* [PMC_SECURE] Base address for the SEC carveout address space */
    MC_REGISTER_SEC_CARVEOUT_SIZE_MB_0 = 0x674, /* [PMC_SECURE] SEC_CARVEOUT_SIZE_MB is the size, in megabytes, of the SEC carveout region. If set to 0, the security check in MC is disabled */
    /* [PMC_SECURE] Sticky bit to control the writes to the other Sec Carveout aperture registers
    0 = Enabled
    1 = Disabled */
    MC_REGISTER_SEC_CARVEOUT_REG_CTRL_0 = 0x678,

    MC_REGISTER_VIDEO_PROTECT_GPU_OVERRIDE_0_0 = 0x984,

    MC_REGISTER_MTS_CARVEOUT_BOM_0 = 0x9A0,
    MC_REGISTER_MTS_CARVEOUT_SIZE_MB_0 = 0x9A4,
    MC_REGISTER_MTS_CARVEOUT_ADR_HI_0 = 0x9A8,
    /* MTS_CARVEOUT_WRITE_ACCESS
    0 = Enabled
    1 = Disabled */
    MC_REGISTER_MTS_CARVEOUT_REG_CTRL_0 = 0x9AC,

    /* Base Address Higher Bits
    SECURITY_BOM_HI has the higher address bits beyond 32 bits of the
    base of the secured region, limited to MB granularity */
    MC_REGISTER_SECURITY_BOM_HI = 0x9BC,
    MC_REGISTER_SECURITY_CFG3_0 = MC_REGISTER_SECURITY_BOM_HI,

    MC_REGISTER_SMMU_TRANSLATION_ENABLE_4_0 = 0xB98
} MC_REGISTER;

volatile security_carveout_t *get_carveout_by_id(unsigned int carveout);
void configure_default_carveouts(void);
void configure_kernel_carveout(unsigned int carveout_id, uint64_t address, uint64_t size);


#endif
