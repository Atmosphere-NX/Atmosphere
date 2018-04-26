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
typedef struct {
    uint32_t _0x0[4];
    uint32_t SMMU_CONFIG_0;
    uint32_t SMMU_TLB_CONFIG_0; /* Controls usage of the TLB */
    uint32_t SMMU_PTC_CONFIG_0; /* Controls usage of the PTC */
    uint32_t SMMU_PTB_ASID_0;
    uint32_t SMMU_PTB_DATA_0;
    uint32_t _0x24[3];
    uint32_t SMMU_TLB_FLUSH_0;
    uint32_t SMMU_PTC_FLUSH_0;
    uint32_t _0x38;
    uint32_t _0x3C;
    uint32_t _0x40[12];
    /* SECURITY_BOM is the base of the secured region, limited to MB granularity.
    This must point to a region of the physical address map allocated to EMEM for it to be effective; the MC cannot secure address space it does not own. (In other words, this is an absolute address, not an offset.)
    Above is the list of clients with the TrustZone-security access. [18.11.1.20]
    Note that AXICIF clients will adhere to the standard AXI protocol "aprot[1]==0" indication for secure requests. */
    uint32_t SECURITY_BOM; /* MC_SECURITY_CFG0_0 */
    uint32_t SECURITY_SIZE_MB; /* MC_SECURITY_CFG1_0 -- SECURITY_SIZE_MB is the size, in megabytes, of the secured region. If set to 0, the security check in MC is disabled */
    uint32_t _0x78[26];
    uint32_t EMEM_ARB_RING1_THROTTLE_0;
    uint32_t EMEM_ARB_RING3_THROTTLE_0;
    uint32_t EMEM_ARB_OVERRIDE_0;
    uint32_t EMEM_ARB_RSV_0;
    uint32_t _0xF0;
    uint32_t CLKEN_OVERRIDE_0;
    uint32_t _0xF8[76];
    uint32_t SMMU_TRANSLATION_ENABLE_0_0;
    uint32_t SMMU_TRANSLATION_ENABLE_1_0;
    uint32_t SMMU_TRANSLATION_ENABLE_2_0;
    uint32_t SMMU_TRANSLATION_ENABLE_3_0;
    uint32_t _0x234[1];
    uint32_t SMMU_AVPC_ASID_0;
    uint32_t _0x240[22];
    uint32_t SMMU_PPCS1_ASID_0;
    uint32_t _0x29C[235];
    uint32_t _0x648;
    uint32_t _0x64C;
    uint32_t _0x650;
    uint32_t _0x654[2];
    uint32_t _0x65C;
    uint32_t _0x660;
    uint32_t _0x664[3];
    uint32_t SEC_CARVEOUT_BOM_0; /* [PMC_SECURE] Base address for the SEC carveout address space */
    uint32_t SEC_CARVEOUT_SIZE_MB_0; /* [PMC_SECURE] SEC_CARVEOUT_SIZE_MB is the size, in megabytes, of the SEC carveout region. If set to 0, the security check in MC is disabled */
    /* [PMC_SECURE] Sticky bit to control the writes to the other Sec Carveout aperture registers
    0 = Enabled
    1 = Disabled */
    uint32_t SEC_CARVEOUT_REG_CTRL_0;
    uint32_t _0x67C[186];
    uint32_t EMEM_CFG_ACCESS_CTRL_0_AND_IRAM_REG_CTRL_0;
    uint32_t _0x968[7];
    uint32_t VIDEO_PROTECT_GPU_OVERRIDE_0_0;
    uint32_t _0x988[6];
    uint32_t MTS_CARVEOUT_BOM_0;
    uint32_t MTS_CARVEOUT_SIZE_MB_0;
    uint32_t MTS_CARVEOUT_ADR_HI_0;
    /* MTS_CARVEOUT_WRITE_ACCESS
    0 = Enabled
    1 = Disabled */
    uint32_t MTS_CARVEOUT_REG_CTRL_0;
    uint32_t _0x9B0[3];
    /* Base Address Higher Bits
    SECURITY_BOM_HI has the higher address bits beyond 32 bits of the
    base of the secured region, limited to MB granularity */
    uint32_t SECURITY_BOM_HI; /* MC_SECURITY_CFG3_0 */
    uint32_t _0x9C0[118];
    uint32_t SMMU_TRANSLATION_ENABLE_4_0;
} mc_register_t; /* 0xB98 */

volatile security_carveout_t *get_carveout_by_id(unsigned int carveout);
void configure_default_carveouts(void);
void configure_kernel_carveout(unsigned int carveout_id, uint64_t address, uint64_t size);

volatile mc_register_t *get_mc_reg();

#endif
