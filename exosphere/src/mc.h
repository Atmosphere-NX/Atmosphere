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
    uint32_t INTSTATUS_0;
    uint32_t INTMASK_0;
    uint32_t ERR_STATUS_0;
    uint32_t ERR_ADR_0;
    uint32_t SMMU_CONFIG_0; /* 0x10 */
    uint32_t SMMU_TLB_CONFIG_0; /* Controls usage of the TLB */
    uint32_t SMMU_PTC_CONFIG_0; /* Controls usage of the PTC */
    uint32_t SMMU_PTB_ASID_0;
    uint32_t SMMU_PTB_DATA_0; /* 0x20 */
    uint32_t _0x24[3]; /* undefined */
    uint32_t SMMU_TLB_FLUSH_0; /* 0x30 */
    uint32_t SMMU_PTC_FLUSH_0;
    uint32_t _0x38; /* unknown */
    uint32_t _0x3C; /* unknown */
    uint32_t _0x40[4]; /* undefined */
    uint32_t EMEM_CFG_0; /* 0x50 */
    uint32_t EMEM_ADR_CFG_0;
    uint32_t EMEM_ADR_CFG_DEV0_0;
    uint32_t EMEM_ADR_CFG_DEV1_0;
    uint32_t EMEM_ADR_CFG_CHANNEL_MASK_0; /* 0x60 */
    uint32_t EMEM_ADR_CFG_BANK_MASK_0_0;
    uint32_t EMEM_ADR_CFG_BANK_MASK_1_0;
    uint32_t EMEM_ADR_CFG_BANK_MASK_2_0;

    /* SECURITY_BOM is the base of the secured region, limited to MB granularity.
    This must point to a region of the physical address map allocated to EMEM for it to be effective; the MC cannot secure address space it does not own. (In other words, this is an absolute address, not an offset.)
    Above is the list of clients with the TrustZone-security access. [18.11.1.20]
    Note that AXICIF clients will adhere to the standard AXI protocol "aprot[1]==0" indication for secure requests.
    0x70
    */
    uint32_t SECURITY_CFG0_0;
    uint32_t SECURITY_CFG1_0; /* SECURITY_SIZE_MB is the size, in megabytes, of the secured region. If set to 0, the security check in MC is disabled */
    uint32_t _0x78[6]; /* undefined */
    uint32_t EMEM_ARB_CFG_0; /* 0x90 */
    uint32_t EMEM_ARB_OUTSTANDING_REQ_0;
    uint32_t EMEM_ARB_TIMING_RCD_0;
    uint32_t EMEM_ARB_TIMING_RP_0;
    uint32_t EMEM_ARB_TIMING_RC_0; /* 0xA0 */
    uint32_t EMEM_ARB_TIMING_RAS_0;
    uint32_t EMEM_ARB_TIMING_FAW_0;
    uint32_t EMEM_ARB_TIMING_RRD_0;
    uint32_t EMEM_ARB_TIMING_RAP2PRE_0; /* 0xB0 */
    uint32_t EMEM_ARB_TIMING_WAP2PRE_0;
    uint32_t EMEM_ARB_TIMING_R2R_0;
    uint32_t EMEM_ARB_TIMING_W2W_0;
    uint32_t EMEM_ARB_TIMING_R2W_0; /* 0xC0 */
    uint32_t EMEM_ARB_TIMING_W2R_0;
    uint32_t EMEM_ARB_MISC2_0;
    uint32_t _0xCC[1]; /* undefined */
    uint32_t EMEM_ARB_DA_TURNS_0; /* 0x D0 */
    uint32_t EMEM_ARB_DA_COVERS_0;
    uint32_t EMEM_ARB_MISC0_0;
    uint32_t EMEM_ARB_MISC1_0;
    uint32_t EMEM_ARB_RING1_THROTTLE_0; /* 0xE0 */
    uint32_t EMEM_ARB_RING3_THROTTLE_0;
    uint32_t EMEM_ARB_OVERRIDE_0;
    uint32_t EMEM_ARB_RSV_0;
    uint32_t _0xF0; /* unknown */ /* 0xF0 */
    uint32_t CLKEN_OVERRIDE_0;
    uint32_t _0xF8[1]; /* undefined */
    uint32_t TIMING_CONTROL_0;
    uint32_t STAT_CONTROL_0; /* 0x100 */
    uint32_t _0x104[63]; /* undefined */
    uint32_t CLIENT_HOTRESET_CTRL_0; /* 0x200 */
    uint32_t CLIENT_HOTRESET_STATUS_0;
    uint32_t EMEM_ARB_ISOCHRONOUS_0_0;
    uint32_t EMEM_ARB_ISOCHRONOUS_1_0;
    uint32_t EMEM_ARB_ISOCHRONOUS_2_0; /* 0x210 */
    uint32_t EMEM_ARB_ISOCHRONOUS_3_0;
    uint32_t EMEM_ARB_HYSTERESIS_0_0;
    uint32_t EMEM_ARB_HYSTERESIS_1_0;
    uint32_t EMEM_ARB_HYSTERESIS_2_0; /* 0x220 */
    uint32_t EMEM_ARB_HYSTERESIS_3_0;
    uint32_t SMMU_TRANSLATION_ENABLE_0_0;
    uint32_t SMMU_TRANSLATION_ENABLE_1_0;
    uint32_t SMMU_TRANSLATION_ENABLE_2_0; /* 0x230 */
    uint32_t SMMU_TRANSLATION_ENABLE_3_0;
    uint32_t SMMU_AFI_ASID_0;
    uint32_t SMMU_AVPC_ASID_0;
    uint32_t SMMU_DC_ASID_0; /* 0x240 */
    uint32_t SMMU_DCB_ASID_0;
    uint32_t _0x228[2]; /* undefined */
    uint32_t SMMU_HC_ASID_0; /* 0x250 */
    uint32_t SMMU_HDA_ASID_0;
    uint32_t SMMU_ISP2_ASID_0;
    uint32_t _0x25C[2]; /* undefined */
    uint32_t SMMU_NVENC_ASID_0;
    uint32_t SMMU_NV_ASID_0;
    uint32_t SMMU_NV2_ASID_0;
    uint32_t SMMU_PPCS_ASID_0; /* 0x270 */
    uint32_t SMMU_SATA_ASID_0;
    uint32_t _0x278[2]; /* undefined */
    uint32_t SMMU_VI_ASID_0; /* 0x280 */
    uint32_t SMMU_VIC_ASID_0;
    uint32_t SMMU_XUSB_HOST_ASID_0;
    uint32_t SMMU_XUSB_DEV_ASID_0;
    uint32_t _0x290[1]; /* undefined */
    uint32_t SMMU_TSEC_ASID_0;
    uint32_t SMMU_PPCS1_ASID_0;
    uint32_t _0x29C[95]; /* undefined */
    uint32_t VIDEO_PROTECT_VPR_OVERRIDE_0;
    uint32_t _0x41C[93]; /* undefined */
    uint32_t VIDEO_PROTECT_VPR_OVERRIDE1_0; /* 0x590 */
    uint32_t _0x594[27]; /* undefined */
    uint32_t SMMU_TLB_SET_SELECTION_MASK_0_0; /* 0x600 */
    uint32_t _0x604[1]; /* undefined */
    uint32_t DISPLAY_SNAP_RING_0;
    uint32_t _0x648; /* unknown */
    uint32_t _0x64C; /* unknown */
    uint32_t _0x650; /* unknown */ /* 0x650 */
    uint32_t ERR_VPR_STATUS_0;
    uint32_t ERR_VPR_ADR_0;
    uint32_t _0x65C; /* unknown */
    uint32_t _0x660; /* unknown */ /* 0x660 */
    uint32_t EMEM_CFG_ACCESS_CTRL_0;
    uint32_t TZ_SECURITY_CTRL_0;
    uint32_t EMEM_ARB_OUTSTANDING_REQ_RING3_0; /* Access Control Bit for EMEM_CFG Registers - Sticky write access lock - 18.11.1.87 */
    uint32_t SEC_CARVEOUT_BOM_0; /* [PMC_SECURE] Base address for the SEC carveout address space */ /* 0x670 */
    uint32_t SEC_CARVEOUT_SIZE_MB_0; /* [PMC_SECURE] SEC_CARVEOUT_SIZE_MB is the size, in megabytes, of the SEC carveout region. If set to 0, the security check in MC is disabled */

    /* [PMC_SECURE] Sticky bit to control the writes to the other Sec Carveout aperture registers
    0 = Enabled
    1 = Disabled
    */
    uint32_t SEC_CARVEOUT_REG_CTRL_0;
    uint32_t ERR_SEC_STATUS_0;
    uint32_t ERR_SEC_ADR_0; /* 0x680 */
    uint32_t PC_IDLE_CLOCK_GATE_CONFIG_0;
    uint32_t STUTTER_CONTROL_0;
    uint32_t _0x68C[9]; /* undefined */
    uint32_t EMEM_ARB_NISO_THROTTLE_0; /* 0x6B0 */
    uint32_t EMEM_ARB_OUTSTANDING_REQ_NISO_0;
    uint32_t EMEM_ARB_NISO_THROTTLE_MASK_0;
    uint32_t EMEM_ARB_RING0_THROTTLE_MASK_0;
    uint32_t EMEM_ARB_TIMING_RFCPB_0; /* 0x6C0 */
    uint32_t EMEM_ARB_TIMING_CCDMW_0;
    uint32_t _0x6C8[10]; /* undefined */
    uint32_t EMEM_ARB_REFPB_HP_CTRL_0; /* 0x6F0 */
    uint32_t EMEM_ARB_REFPB_BANK_CTRL_0;
    uint32_t _0x6F8[155]; /* undefined */
    uint32_t IRAM_REG_CTRL_0;
    uint32_t EMEM_ARB_OVERRIDE_1_0;
    uint32_t _0x96C[1]; /* undefined */
    uint32_t CLIENT_HOTRESET_CTRL_1_0; /* 0x970 */
    uint32_t CLIENT_HOTRESET_STATUS_1_0;
    uint32_t _0x978[3]; /* undefined */
    uint32_t VIDEO_PROTECT_GPU_OVERRIDE_0_0;
    uint32_t VIDEO_PROTECT_GPU_OVERRIDE_1_0;
    uint32_t _0x98C[5]; /* undefined */
    uint32_t MTS_CARVEOUT_BOM_0; /* 0x9A0 */
    uint32_t MTS_CARVEOUT_SIZE_MB_0;
    uint32_t MTS_CARVEOUT_ADR_HI_0;

    /* MTS_CARVEOUT_WRITE_ACCESS
    0 = Enabled
    1 = Disabled
    */
    uint32_t MTS_CARVEOUT_REG_CTRL_0;
    uint32_t _0x9B0[2]; /* undefined */ /* 0x9B0 */
    uint32_t SMMU_PTC_FLUSH_1_0;

    /* Base Address Higher Bits
    SECURITY_BOM_HI has the higher address bits beyond 32 bits of the
    base of the secured region, limited to MB granularity
    */
    uint32_t SECURITY_CFG3_0;
    uint32_t EMEM_BANK_SWIZZLE_CFG0_0; /* 0x9C0 */
    uint32_t EMEM_BANK_SWIZZLE_CFG1_0;
    uint32_t EMEM_BANK_SWIZZLE_CFG2_0;
    uint32_t EMEM_BANK_SWIZZLE_CFG3_0;
    uint32_t _0x9D0[1]; /* undefined */
    uint32_t SEC_CARVEOUT_ADR_HI_0;
    uint32_t _0x9D8[44]; /* undefined */
    uint32_t SMMU_DC1_ASID_0; /* 0xA88 */
    uint32_t _0xA8C[2]; /* undefined */
    uint32_t SMMU_SDMMC1A_ASID_0;
    uint32_t SMMU_SDMMC2A_ASID_0;
    uint32_t SMMU_SDMMC3A_ASID_0;
    uint32_t SMMU_SDMMC4A_ASID_0; /* 0xAA0 */
    uint32_t SMMU_ISP2B_ASID_0;
    uint32_t SMMU_GPU_ASID_0;
    uint32_t SMMU_GPUB_ASID_0;
    uint32_t SMMU_PPCS2_ASID_0; /* 0xAB0 */
    uint32_t SMMU_NVDEC_ASID_0;
    uint32_t SMMU_APE_ASID_0;
    uint32_t SMMU_SE_ASID_0;
    uint32_t SMMU_NVJPG_ASID_0; /* 0xAC0 */
    uint32_t SMMU_HC1_ASID_0;
    uint32_t SMMU_SE1_ASID_0;
    uint32_t SMMU_AXIAP_ASID_0;
    uint32_t SMMU_ETR_ASID_0; /* 0xAD0 */
    uint32_t SMMU_TSECB_ASID_0;
    uint32_t SMMU_TSEC1_ASID_0;
    uint32_t SMMU_TSECB1_ASID_0;
    uint32_t SMMU_NVDEC1_ASID_0; /* 0xAE0 */
    uint32_t _0xAE4[39]; /* undefined */
    uint32_t EMEM_ARB_NISO_THROTTLE_MASK_1_0; /* 0xB80 */
    uint32_t EMEM_ARB_HYSTERESIS_4_0;
    uint32_t _0xB88[3]; /* undefined */
    uint32_t EMEM_ARB_ISOCHRONOUS_4_0;
    uint32_t SMMU_TRANSLATION_ENABLE_4_0; /* 0xB98 */
} mc_register_t; /* 0xB98 */

/* 18.11.1.6 MC_SMMU_TLB_CONFIG_0 */
typedef enum{
    /* Set the number of active lines.
    Allows the TLB to be made "virtually smaller" to save power.
    "Inactive" lines will never hit and never hold data
    */
    SMMU_TLB_CONFIG_ACTIVE_LINES             = 0x3F << 0,
    SMMU_TLB_CONFIG_ROUND_ROBIN_ARBITRATION  = 1 << 28, /* When enabled, forces round robin (RR) arbitration between TLB hit and miss FIFOs */

    /* Allow hits to pass misses in the TLB.
    This value may not be changed on the fly.
    Ideally, this should be set before enabling the SMMU.
    At the very least, the TLB needs to be flushed and traffic through the SMMU stopped before changing this value
    */
    SMMU_TLB_CONFIG_HIT_UNDER_MISS           = 1 << 29,
    SMMU_TLB_CONFIG_STATS_TEST               = 1 << 30, /* Set stats registers to all "1s" */
    SMMU_TLB_CONFIG_STATS_ENABLE             = 1 << 31, /* Enable TLB Hit and Miss counters */
} SMMU_TLB_CONFIG;

/* 18.11.1.7 MC_SMMU_PTC_CONFIG_0 */
typedef enum {
    SMMU_PTC_CONFIG_INDEX_MAP    = 0x7F << 0, /* XOR pattern for tag generation */
    SMMU_PTC_CONFIG_LINE_MASK    = 0xF << 8, /* XOR pattern for line generation */
    SMMU_PTC_CONFIG_REQ_LIMIT    = 0xF << 24, /* Limit outstanding PTC fill requests to the DRAM */
    SMMU_PTC_CONFIG_CACHE_ENABLE = 1 << 29, /* Enable the PTC cache */
    SMMU_PTC_CONFIG_STATS_TEST   = 1 << 30, /* Set stats registers to all "1s" */
    SMMU_PTC_CONFIG_STATS_ENABLE = 1 << 31 /* Enable PTC Hit and Miss counters */
} SMMU_PTC_CONFIG;

/* 18.11.1.9 MC_SMMU_PTB_DATA_0 */
typedef enum {
    SSM_PTB_DATA_ASID_PDE_BASE  = 0x3FFFFF << 0, /* Pointer to page of PDEs, bits [33:12] for this ASID */
    SSM_PTB_DATA_ASID_NONSECURE = 1 << 29, /* if set, non-secure accesses are allowed for this ASID */
    SSM_PTB_DATA_ASID_WRITABLE  = 1 << 30, /* if set, writes are allowed for this ASID */
    SSM_PTB_DATA_ASID_READABLE  = 1 << 31 /* if set, reads are allowed for this ASID */
} SMMU_PTB_DATA;

/* 18.11.1.63 MC_SMMU_AVPC_ASID_0 */
typedef enum {
    SMMU_AVPC_ASID_AVPC_ASID        = 0x7F << 0, /* If translation enabled, ASID to use when va[33:32] = 0x0 */
    SMMU_AVPC_ASID_AVPC_SMMU_ENABLE = 1 << 31 /* if set, translation is enabled for this client */
} SMMU_AVPC_ASID;

/* 18.11.1.79 MC_SMMU_PPCS1_ASID_0 */
typedef enum{
    SMMU_PPCS1_ASID_PPCS1_ASID        = 0x7F << 0, /* if set, translation is enabled for this client */
    SMMU_PPCS1_ASID_PPCS1_SMMU_ENABLE = 1 << 31 /* If translation enabled, ASID to use when va[33:32] = 0x0 */
} SMMU_PPCS1_ASID;

volatile security_carveout_t *get_carveout_by_id(unsigned int carveout);
void configure_default_carveouts(void);
void configure_kernel_carveout(unsigned int carveout_id, uint64_t address, uint64_t size);

volatile mc_register_t *get_mc_reg();

#endif
