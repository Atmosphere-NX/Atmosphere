/*
 * Copyright (c) 2018 Atmosphère-NX
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

#define MC_SMMU_CONFIG_0     MAKE_MC_REG(0x010)
#define MC_SMMU_TLB_CONFIG_0 MAKE_MC_REG(0x014)
#define MC_SMMU_PTC_CONFIG_0 MAKE_MC_REG(0x018)
#define MC_SMMU_PTB_ASID_0   MAKE_MC_REG(0x01C)
#define MC_SMMU_PTB_DATA_0   MAKE_MC_REG(0x020)
#define MC_SMMU_TLB_FLUSH_0  MAKE_MC_REG(0x030)
#define MC_SMMU_PTC_FLUSH_0  MAKE_MC_REG(0x034)
#define MC_SMMU_AFI_ASID_0   MAKE_MC_REG(0x238)
#define MC_SMMU_AVPC_ASID_0  MAKE_MC_REG(0x23C)

#define MC_SMMU_TRANSLATION_ENABLE_0_0 MAKE_MC_REG(0x228)
#define MC_SMMU_TRANSLATION_ENABLE_1_0 MAKE_MC_REG(0x22C)
#define MC_SMMU_TRANSLATION_ENABLE_2_0 MAKE_MC_REG(0x230)
#define MC_SMMU_TRANSLATION_ENABLE_3_0 MAKE_MC_REG(0x234)
#define MC_SMMU_TRANSLATION_ENABLE_4_0 MAKE_MC_REG(0xB98)

#define MC_SMMU_PPCS1_ASID_0 MAKE_MC_REG(0x298)

#define MC_SECURITY_CFG0_0 MAKE_MC_REG(0x070)
#define MC_SECURITY_CFG1_0 MAKE_MC_REG(0x074)
#define MC_SECURITY_CFG3_0 MAKE_MC_REG(0x9BC)


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
    uint8_t padding[0x18];
} security_carveout_t;


volatile security_carveout_t *get_carveout_by_id(unsigned int carveout);
void configure_default_carveouts(void);
void configure_kernel_carveout(unsigned int carveout_id, uint64_t address, uint64_t size);


#endif
