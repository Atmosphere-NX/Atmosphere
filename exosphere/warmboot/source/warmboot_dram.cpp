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
#include "warmboot_dram.hpp"

namespace ams::warmboot {

    namespace {

        constexpr inline const uintptr_t APB_MISC = secmon::MemoryRegionPhysicalDeviceApbMisc.GetAddress();
        constexpr inline const uintptr_t EMC      = EMC_ADDRESS(0);
        constexpr inline const uintptr_t EMC0     = EMC0_ADDRESS(0);
        constexpr inline const uintptr_t EMC1     = EMC1_ADDRESS(0);
        constexpr inline const uintptr_t MC       = secmon::MemoryRegionPhysicalDeviceMemoryController.GetAddress();
        constexpr inline const uintptr_t MC0      = secmon::MemoryRegionPhysicalDeviceMemoryController0.GetAddress();
        constexpr inline const uintptr_t MC1      = secmon::MemoryRegionPhysicalDeviceMemoryController1.GetAddress();

    }

    void RestrictBpmpAccessToMainMemory() {
        /* Bpmp memory access is restricted by forcing internal access to an invalid carveout. */
        constexpr u32 ForceInternalAccess0 = reg::Encode(MC_REG_BITS_ENUM(CLIENT_ACCESS0_AVPCARM7R, ENABLE));
        constexpr u32 ForceInternalAccess1 = reg::Encode(MC_REG_BITS_ENUM(CLIENT_ACCESS1_AVPCARM7W, ENABLE));

        constexpr u32 CarveoutConfig       = reg::Encode(MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_IS_WPR,                                 DISABLED),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_FORCE_APERTURE_ID_MATCH,                DISABLED),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_ALLOW_APERTURE_ID_MISMATCH,             DISABLED),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_TZ_GLOBAL_RD_EN,                        DISABLED),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_TZ_GLOBAL_WR_EN,                        DISABLED),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_SEND_CFG_TO_GPU,                        DISABLED),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL3, ENABLE_CHECKS),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL2, ENABLE_CHECKS),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL1, ENABLE_CHECKS),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_WRITE_CHECK_ACCESS_LEVEL0, ENABLE_CHECKS),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL3,  ENABLE_CHECKS),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL2,  ENABLE_CHECKS),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL1,  ENABLE_CHECKS),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_DISABLE_READ_CHECK_ACCESS_LEVEL0,  ENABLE_CHECKS),
                                                         MC_REG_BITS_VALUE(SECURITY_CARVEOUT_CFG0_APERTURE_ID,                                   0),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL3,                    DISABLED),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL2,                    DISABLED),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL1,                    DISABLED),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_WRITE_ACCESS_LEVEL0,                     ENABLED),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL3,                     DISABLED),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL2,                     DISABLED),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL1,                     DISABLED),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_READ_ACCESS_LEVEL0,                      ENABLED),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_ADDRESS_TYPE,                  UNTRANSLATED_ONLY),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_LOCK_MODE,                                LOCKED),
                                                         MC_REG_BITS_ENUM (SECURITY_CARVEOUT_CFG0_PROTECT_MODE,                          TZ_SECURE));

        /* Specify a 128KB carveout at NULL with no clients allowed access, and bpmp forced to access. */
        reg::Write(MC + MC_SECURITY_CARVEOUT4_BOM,                                              0);
        reg::Write(MC + MC_SECURITY_CARVEOUT4_BOM_HI,                                           0);
        reg::Write(MC + MC_SECURITY_CARVEOUT4_SIZE_128KB,                                       1);
        reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_ACCESS0,                                   0);
        reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_ACCESS1,                                   0);
        reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_ACCESS2,                                   0);
        reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_ACCESS3,                                   0);
        reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_ACCESS4,                                   0);
        reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS0, ForceInternalAccess0);
        reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS1, ForceInternalAccess1);
        reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS2,                    0);
        reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS3,                    0);
        reg::Write(MC + MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS4,                    0);
        reg::Write(MC + MC_SECURITY_CARVEOUT4_CFG0,                                CarveoutConfig);
    }

    void RestoreRamSvop() {
        reg::ReadWrite(APB_MISC + APB_MISC_GP_ASDBGREG, APB_MISC_REG_BITS_VALUE(GP_ASDBGREG_CFG2TMC_RAM_SVOP_PDP, 2));
    }

    void ConfigureEmcPmacroTraining() {
        /* Disable writes to BYTE0-7. */
        reg::Write(EMC + EMC_PMACRO_CFG_PM_GLOBAL_0, EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE0,  ENABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE1,  ENABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE2,  ENABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE3,  ENABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE4,  ENABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE5,  ENABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE6,  ENABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE7,  ENABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_CMD0,  DISABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_CMD1,  DISABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_CMD2,  DISABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_CMD3,  DISABLE));

        /* Set E_WRPTR on Channel 0. */
        reg::Write(EMC + EMC_PMACRO_TRAINING_CTRL_0, EMC_REG_BITS_ENUM(PMACRO_TRAINING_CTRL_0_CH0_TRAINING_ENABLED,        DISABLED),
                                                     EMC_REG_BITS_ENUM(PMACRO_TRAINING_CTRL_0_CH0_TRAINING_TRAIN_QPOP,     DISABLED),
                                                     EMC_REG_BITS_ENUM(PMACRO_TRAINING_CTRL_0_CH0_TRAINING_RX_E_DIRECT_ZI, DISABLED),
                                                     EMC_REG_BITS_ENUM(PMACRO_TRAINING_CTRL_0_CH0_TRAINING_E_WRPTR,         ENABLED),
                                                     EMC_REG_BITS_ENUM(PMACRO_TRAINING_CTRL_0_CH0_TRAINING_DRV_DQS,        DISABLED));

        /* Set E_WRPTR on Channel 1. */
        reg::Write(EMC + EMC_PMACRO_TRAINING_CTRL_1, EMC_REG_BITS_ENUM(PMACRO_TRAINING_CTRL_1_CH1_TRAINING_ENABLED,        DISABLED),
                                                     EMC_REG_BITS_ENUM(PMACRO_TRAINING_CTRL_1_CH1_TRAINING_TRAIN_QPOP,     DISABLED),
                                                     EMC_REG_BITS_ENUM(PMACRO_TRAINING_CTRL_1_CH1_TRAINING_RX_E_DIRECT_ZI, DISABLED),
                                                     EMC_REG_BITS_ENUM(PMACRO_TRAINING_CTRL_1_CH1_TRAINING_E_WRPTR,         ENABLED),
                                                     EMC_REG_BITS_ENUM(PMACRO_TRAINING_CTRL_1_CH1_TRAINING_DRV_DQS,        DISABLED));

        /* Re-enable writes to BYTE0-7. */
        reg::Write(EMC + EMC_PMACRO_CFG_PM_GLOBAL_0, EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE0, DISABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE1, DISABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE2, DISABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE3, DISABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE4, DISABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE5, DISABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE6, DISABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE7, DISABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_CMD0,  DISABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_CMD1,  DISABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_CMD2,  DISABLE),
                                                     EMC_REG_BITS_ENUM(PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_CMD3,  DISABLE));
    }

}
