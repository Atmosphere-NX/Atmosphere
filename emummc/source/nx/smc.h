/**
 * @file smc.h
 * @brief Wrappers for secure monitor calls.
 * @copyright libnx Authors
 */
#pragma once
#include "../utils/types.h"
#include "svc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SplConfigItem_DisableProgramVerification = 1,
    SplConfigItem_DramId = 2,
    SplConfigItem_SecurityEngineIrqNumber = 3,
    SplConfigItem_Version = 4,
    SplConfigItem_HardwareType = 5,
    SplConfigItem_IsRetail = 6,
    SplConfigItem_IsRecoveryBoot = 7,
    SplConfigItem_DeviceId = 8,
    SplConfigItem_BootReason = 9,
    SplConfigItem_MemoryArrange = 10,
    SplConfigItem_IsDebugMode = 11,
    SplConfigItem_KernelMemoryConfiguration = 12,
    SplConfigItem_IsChargerHiZModeEnabled = 13,
    SplConfigItem_IsKiosk = 14,
    SplConfigItem_NewHardwareType = 15,
    SplConfigItem_NewKeyGeneration = 16,
    SplConfigItem_Package2Hash = 17,

    SplConfigItem_ExosphereVersion = 65000,
    SplConfigItem_NeedsReboot      = 65001,
    SplConfigItem_NeedsShutdown    = 65002,
    SplConfigItem_ExosphereVerHash = 65003,
    SplConfigItem_HasRcmBugPatch   = 65004,
} SplConfigItem;

typedef enum {
    SplSocType_Erista = 0,
    SplSocType_Mariko = 1,
} SplSocType;

typedef enum {
    SplHardwareType_Icosa = 0,
    SplHardwareType_Copper = 1,
    SplHardwareType_Hoag = 2,
    SplHardwareType_Iowa = 3,
    SplHardwareType_Calcio = 4,
    SplHardwareType_Five = 5,
} SplHardwareType;

typedef enum {
   EXO_EMUMMC_TYPE_NONE = 0,
   EXO_EMUMMC_TYPE_PARTITION = 1,
   EXO_EMUMMC_TYPE_FILES = 2,
} exo_emummc_type_t;

typedef enum {
    EXO_EMUMMC_MMC_NAND = 0,
    EXO_EMUMMC_MMC_SD = 1,
    EXO_EMUMMC_MMC_GC = 2,
} exo_emummc_mmc_t;

typedef struct {
    uint32_t magic;
    uint32_t type;
    uint32_t id;
    uint32_t fs_version;
} exo_emummc_base_config_t;

typedef struct {
    uint64_t start_sector;
} exo_emummc_partition_config_t;

typedef struct {
    exo_emummc_base_config_t base_cfg;
    union {
        exo_emummc_partition_config_t partition_cfg;
    };
} exo_emummc_config_t;

Result smcGetConfig(SplConfigItem config_item, u64 *out_config);

SplHardwareType splGetHardwareType(void);
SplSocType splGetSocType(void);

void smcRebootToRcm(void);
void smcRebootToIramPayload(void);
void smcPerformShutdown(void);

Result smcCopyToIram(uintptr_t iram_addr, const void *src_addr, u32 size);
Result smcCopyFromIram(void *dst_addr, uintptr_t iram_addr, u32 size);

Result smcReadWriteRegister(u32 phys_addr, u32 value, u32 mask);

Result smcGetEmummcConfig(exo_emummc_mmc_t mmc_id, exo_emummc_config_t *out_cfg, void *out_paths);

Result smcGenerateRandomBytes(void *dst, u32 size);
u64 smcGenerateRandomU64(void);

#ifdef __cplusplus
}
#endif
