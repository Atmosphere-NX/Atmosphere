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

#ifndef FUSEE_FUSE_H
#define FUSEE_FUSE_H

#define FUSE_BASE 0x7000F800
#define FUSE_CHIP_BASE (FUSE_BASE + 0x98)
#define MAKE_FUSE_REG(n) MAKE_REG32(FUSE_BASE + n)
#define MAKE_FUSE_CHIP_REG(n) MAKE_REG32(FUSE_CHIP_BASE + n)

typedef struct {
    uint32_t FUSE_FUSECTRL;
    uint32_t FUSE_FUSEADDR;
    uint32_t FUSE_FUSERDATA;
    uint32_t FUSE_FUSEWDATA;
    uint32_t FUSE_FUSETIME_RD1;
    uint32_t FUSE_FUSETIME_RD2;
    uint32_t FUSE_FUSETIME_PGM1;
    uint32_t FUSE_FUSETIME_PGM2;
    uint32_t FUSE_PRIV2INTFC_START;
    uint32_t FUSE_FUSEBYPASS;
    uint32_t FUSE_PRIVATEKEYDISABLE;
    uint32_t FUSE_DISABLEREGPROGRAM;
    uint32_t FUSE_WRITE_ACCESS_SW;
    uint32_t FUSE_PWR_GOOD_SW;
    uint32_t _0x38;
    uint32_t FUSE_PRIV2RESHIFT;
    uint32_t _0x40[0x3];
    uint32_t FUSE_FUSETIME_RD3;
    uint32_t _0x50[0xC];
    uint32_t FUSE_PRIVATE_KEY0_NONZERO;
    uint32_t FUSE_PRIVATE_KEY1_NONZERO;
    uint32_t FUSE_PRIVATE_KEY2_NONZERO;
    uint32_t FUSE_PRIVATE_KEY3_NONZERO;
    uint32_t FUSE_PRIVATE_KEY4_NONZERO;
    uint32_t _0x94;
} tegra_fuse_t;

typedef struct {
    uint32_t _0x98[0x1A];
    uint32_t FUSE_PRODUCTION_MODE;
    uint32_t FUSE_JTAG_SECUREID_VALID;
    uint32_t FUSE_ODM_LOCK;
    uint32_t FUSE_OPT_OPENGL_EN;
    uint32_t FUSE_SKU_INFO;
    uint32_t FUSE_CPU_SPEEDO_0_CALIB;
    uint32_t FUSE_CPU_IDDQ_CALIB;
    uint32_t _0x11C[0x3];
    uint32_t FUSE_OPT_FT_REV;
    uint32_t FUSE_CPU_SPEEDO_1_CALIB;
    uint32_t FUSE_CPU_SPEEDO_2_CALIB;
    uint32_t FUSE_SOC_SPEEDO_0_CALIB;
    uint32_t FUSE_SOC_SPEEDO_1_CALIB;
    uint32_t FUSE_SOC_SPEEDO_2_CALIB;
    uint32_t FUSE_SOC_IDDQ_CALIB;
    uint32_t _0x144;
    uint32_t FUSE_FA;
    uint32_t FUSE_RESERVED_PRODUCTION;
    uint32_t FUSE_HDMI_LANE0_CALIB;
    uint32_t FUSE_HDMI_LANE1_CALIB;
    uint32_t FUSE_HDMI_LANE2_CALIB;
    uint32_t FUSE_HDMI_LANE3_CALIB;
    uint32_t FUSE_ENCRYPTION_RATE;
    uint32_t FUSE_PUBLIC_KEY[0x8];
    uint32_t FUSE_TSENSOR1_CALIB;
    uint32_t FUSE_TSENSOR2_CALIB;
    uint32_t _0x18C;
    uint32_t FUSE_OPT_CP_REV;
    uint32_t FUSE_OPT_PFG;
    uint32_t FUSE_TSENSOR0_CALIB;
    uint32_t FUSE_FIRST_BOOTROM_PATCH_SIZE;
    uint32_t FUSE_SECURITY_MODE;
    uint32_t FUSE_PRIVATE_KEY[0x5];
    uint32_t FUSE_ARM_JTAG_DIS;
    uint32_t FUSE_BOOT_DEVICE_INFO;
    uint32_t FUSE_RESERVED_SW;
    uint32_t FUSE_OPT_VP9_DISABLE;
    uint32_t FUSE_RESERVED_ODM0[0x8];
    uint32_t FUSE_OBS_DIS;
    uint32_t _0x1EC;
    uint32_t FUSE_USB_CALIB;
    uint32_t FUSE_SKU_DIRECT_CONFIG;
    uint32_t FUSE_KFUSE_PRIVKEY_CTRL;
    uint32_t FUSE_PACKAGE_INFO;
    uint32_t FUSE_OPT_VENDOR_CODE;
    uint32_t FUSE_OPT_FAB_CODE;
    uint32_t FUSE_OPT_LOT_CODE_0;
    uint32_t FUSE_OPT_LOT_CODE_1;
    uint32_t FUSE_OPT_WAFER_ID;
    uint32_t FUSE_OPT_X_COORDINATE;
    uint32_t FUSE_OPT_Y_COORDINATE;
    uint32_t FUSE_OPT_SEC_DEBUG_EN;
    uint32_t FUSE_OPT_OPS_RESERVED;
    uint32_t _0x224;
    uint32_t FUSE_GPU_IDDQ_CALIB;
    uint32_t FUSE_TSENSOR3_CALIB;
    uint32_t FUSE_CLOCK_BOUNDOUT0;
    uint32_t FUSE_CLOCK_BOUNDOUT1;
    uint32_t _0x238[0x3];
    uint32_t FUSE_OPT_SAMPLE_TYPE;
    uint32_t FUSE_OPT_SUBREVISION;
    uint32_t FUSE_OPT_SW_RESERVED_0;
    uint32_t FUSE_OPT_SW_RESERVED_1;
    uint32_t FUSE_TSENSOR4_CALIB;
    uint32_t FUSE_TSENSOR5_CALIB;
    uint32_t FUSE_TSENSOR6_CALIB;
    uint32_t FUSE_TSENSOR7_CALIB;
    uint32_t FUSE_OPT_PRIV_SEC_EN;
    uint32_t _0x268[0x5];
    uint32_t FUSE_FUSE2TSEC_DEBUG_DISABLE;
    uint32_t FUSE_TSENSOR_COMMON;
    uint32_t FUSE_OPT_CP_BIN;
    uint32_t FUSE_OPT_GPU_DISABLE;
    uint32_t FUSE_OPT_FT_BIN;
    uint32_t FUSE_OPT_DONE_MAP;
    uint32_t _0x294;
    uint32_t FUSE_APB2JTAG_DISABLE;
    uint32_t FUSE_ODM_INFO;
    uint32_t _0x2A0[0x2];
    uint32_t FUSE_ARM_CRYPT_DE_FEATURE;
    uint32_t _0x2AC[0x5];
    uint32_t FUSE_WOA_SKU_FLAG;
    uint32_t FUSE_ECO_RESERVE_1;
    uint32_t FUSE_GCPLEX_CONFIG_FUSE;
    uint32_t FUSE_PRODUCTION_MONTH;
    uint32_t FUSE_RAM_REPAIR_INDICATOR;
    uint32_t FUSE_TSENSOR9_CALIB;
    uint32_t _0x2D8;
    uint32_t FUSE_VMIN_CALIBRATION;
    uint32_t FUSE_AGING_SENSOR_CALIBRATION;
    uint32_t FUSE_DEBUG_AUTHENTICATION;
    uint32_t FUSE_SECURE_PROVISION_INDEX;
    uint32_t FUSE_SECURE_PROVISION_INFO;
    uint32_t FUSE_OPT_GPU_DISABLE_CP1;
    uint32_t FUSE_SPARE_ENDIS;
    uint32_t FUSE_ECO_RESERVE_0;
    uint32_t _0x2FC[0x2];
    uint32_t FUSE_RESERVED_CALIB0;
    uint32_t FUSE_RESERVED_CALIB1;
    uint32_t FUSE_OPT_GPU_TPC0_DISABLE;
    uint32_t FUSE_OPT_GPU_TPC0_DISABLE_CP1;
    uint32_t FUSE_OPT_CPU_DISABLE;
    uint32_t FUSE_OPT_CPU_DISABLE_CP1;
    uint32_t FUSE_TSENSOR10_CALIB;
    uint32_t FUSE_TSENSOR10_CALIB_AUX;
    uint32_t _0x324[0x5];
    uint32_t FUSE_OPT_GPU_TPC0_DISABLE_CP2;
    uint32_t FUSE_OPT_GPU_TPC1_DISABLE;
    uint32_t FUSE_OPT_GPU_TPC1_DISABLE_CP1;
    uint32_t FUSE_OPT_GPU_TPC1_DISABLE_CP2;
    uint32_t FUSE_OPT_CPU_DISABLE_CP2;
    uint32_t FUSE_OPT_GPU_DISABLE_CP2;
    uint32_t FUSE_USB_CALIB_EXT;
    uint32_t FUSE_RESERVED_FIELD;
    uint32_t _0x358[0x9];
    uint32_t FUSE_SPARE_REALIGNMENT_REG;
    uint32_t FUSE_SPARE_BIT[0x20];
} tegra_fuse_chip_common_t;

typedef struct {
    uint32_t _0x98[0x1A];
    uint32_t FUSE_PRODUCTION_MODE;
    uint32_t FUSE_JTAG_SECUREID_VALID;
    uint32_t FUSE_ODM_LOCK;
    uint32_t FUSE_OPT_OPENGL_EN;
    uint32_t FUSE_SKU_INFO;
    uint32_t FUSE_CPU_SPEEDO_0_CALIB;
    uint32_t FUSE_CPU_IDDQ_CALIB;
    uint32_t _0x11C[0x3];
    uint32_t FUSE_OPT_FT_REV;
    uint32_t FUSE_CPU_SPEEDO_1_CALIB;
    uint32_t FUSE_CPU_SPEEDO_2_CALIB;
    uint32_t FUSE_SOC_SPEEDO_0_CALIB;
    uint32_t FUSE_SOC_SPEEDO_1_CALIB;
    uint32_t FUSE_SOC_SPEEDO_2_CALIB;
    uint32_t FUSE_SOC_IDDQ_CALIB;
    uint32_t _0x144;
    uint32_t FUSE_FA;
    uint32_t FUSE_RESERVED_PRODUCTION;
    uint32_t FUSE_HDMI_LANE0_CALIB;
    uint32_t FUSE_HDMI_LANE1_CALIB;
    uint32_t FUSE_HDMI_LANE2_CALIB;
    uint32_t FUSE_HDMI_LANE3_CALIB;
    uint32_t FUSE_ENCRYPTION_RATE;
    uint32_t FUSE_PUBLIC_KEY[0x8];
    uint32_t FUSE_TSENSOR1_CALIB;
    uint32_t FUSE_TSENSOR2_CALIB;
    uint32_t _0x18C;
    uint32_t FUSE_OPT_CP_REV;
    uint32_t FUSE_OPT_PFG;
    uint32_t FUSE_TSENSOR0_CALIB;
    uint32_t FUSE_FIRST_BOOTROM_PATCH_SIZE;
    uint32_t FUSE_SECURITY_MODE;
    uint32_t FUSE_PRIVATE_KEY[0x5];
    uint32_t FUSE_ARM_JTAG_DIS;
    uint32_t FUSE_BOOT_DEVICE_INFO;
    uint32_t FUSE_RESERVED_SW;
    uint32_t FUSE_OPT_VP9_DISABLE;
    uint32_t FUSE_RESERVED_ODM0[0x8];
    uint32_t FUSE_OBS_DIS;
    uint32_t _0x1EC;
    uint32_t FUSE_USB_CALIB;
    uint32_t FUSE_SKU_DIRECT_CONFIG;
    uint32_t FUSE_KFUSE_PRIVKEY_CTRL;
    uint32_t FUSE_PACKAGE_INFO;
    uint32_t FUSE_OPT_VENDOR_CODE;
    uint32_t FUSE_OPT_FAB_CODE;
    uint32_t FUSE_OPT_LOT_CODE_0;
    uint32_t FUSE_OPT_LOT_CODE_1;
    uint32_t FUSE_OPT_WAFER_ID;
    uint32_t FUSE_OPT_X_COORDINATE;
    uint32_t FUSE_OPT_Y_COORDINATE;
    uint32_t FUSE_OPT_SEC_DEBUG_EN;
    uint32_t FUSE_OPT_OPS_RESERVED;
    uint32_t FUSE_SATA_CALIB;               /* Erista only. */
    uint32_t FUSE_GPU_IDDQ_CALIB;
    uint32_t FUSE_TSENSOR3_CALIB;
    uint32_t FUSE_CLOCK_BOUNDOUT0;
    uint32_t FUSE_CLOCK_BOUNDOUT1;
    uint32_t _0x238[0x3];
    uint32_t FUSE_OPT_SAMPLE_TYPE;
    uint32_t FUSE_OPT_SUBREVISION;
    uint32_t FUSE_OPT_SW_RESERVED_0;
    uint32_t FUSE_OPT_SW_RESERVED_1;
    uint32_t FUSE_TSENSOR4_CALIB;
    uint32_t FUSE_TSENSOR5_CALIB;
    uint32_t FUSE_TSENSOR6_CALIB;
    uint32_t FUSE_TSENSOR7_CALIB;
    uint32_t FUSE_OPT_PRIV_SEC_EN;
    uint32_t FUSE_PKC_DISABLE;              /* Erista only. */
    uint32_t _0x26C[0x4];
    uint32_t FUSE_FUSE2TSEC_DEBUG_DISABLE;
    uint32_t FUSE_TSENSOR_COMMON;
    uint32_t FUSE_OPT_CP_BIN;
    uint32_t FUSE_OPT_GPU_DISABLE;
    uint32_t FUSE_OPT_FT_BIN;
    uint32_t FUSE_OPT_DONE_MAP;
    uint32_t _0x294;
    uint32_t FUSE_APB2JTAG_DISABLE;
    uint32_t FUSE_ODM_INFO;
    uint32_t _0x2A0[0x2];
    uint32_t FUSE_ARM_CRYPT_DE_FEATURE;
    uint32_t _0x2AC[0x5];
    uint32_t FUSE_WOA_SKU_FLAG;
    uint32_t FUSE_ECO_RESERVE_1;
    uint32_t FUSE_GCPLEX_CONFIG_FUSE;
    uint32_t FUSE_PRODUCTION_MONTH;
    uint32_t FUSE_RAM_REPAIR_INDICATOR;
    uint32_t FUSE_TSENSOR9_CALIB;
    uint32_t _0x2D8;
    uint32_t FUSE_VMIN_CALIBRATION;
    uint32_t FUSE_AGING_SENSOR_CALIBRATION;
    uint32_t FUSE_DEBUG_AUTHENTICATION;
    uint32_t FUSE_SECURE_PROVISION_INDEX;
    uint32_t FUSE_SECURE_PROVISION_INFO;
    uint32_t FUSE_OPT_GPU_DISABLE_CP1;
    uint32_t FUSE_SPARE_ENDIS;
    uint32_t FUSE_ECO_RESERVE_0;
    uint32_t _0x2FC[0x2];
    uint32_t FUSE_RESERVED_CALIB0;
    uint32_t FUSE_RESERVED_CALIB1;
    uint32_t FUSE_OPT_GPU_TPC0_DISABLE;
    uint32_t FUSE_OPT_GPU_TPC0_DISABLE_CP1;
    uint32_t FUSE_OPT_CPU_DISABLE;
    uint32_t FUSE_OPT_CPU_DISABLE_CP1;
    uint32_t FUSE_TSENSOR10_CALIB;
    uint32_t FUSE_TSENSOR10_CALIB_AUX;
    uint32_t FUSE_OPT_RAM_SVOP_DP;          /* Erista only. */
    uint32_t FUSE_OPT_RAM_SVOP_PDP;         /* Erista only. */
    uint32_t FUSE_OPT_RAM_SVOP_REG;         /* Erista only. */
    uint32_t FUSE_OPT_RAM_SVOP_SP;          /* Erista only. */
    uint32_t FUSE_OPT_RAM_SVOP_SMPDP;       /* Erista only. */
    uint32_t FUSE_OPT_GPU_TPC0_DISABLE_CP2;
    uint32_t FUSE_OPT_GPU_TPC1_DISABLE;
    uint32_t FUSE_OPT_GPU_TPC1_DISABLE_CP1;
    uint32_t FUSE_OPT_GPU_TPC1_DISABLE_CP2;
    uint32_t FUSE_OPT_CPU_DISABLE_CP2;
    uint32_t FUSE_OPT_GPU_DISABLE_CP2;
    uint32_t FUSE_USB_CALIB_EXT;
    uint32_t FUSE_RESERVED_FIELD;
    uint32_t _0x358[0x9];
    uint32_t FUSE_SPARE_REALIGNMENT_REG;
    uint32_t FUSE_SPARE_BIT[0x20];
} tegra_fuse_chip_erista_t;

typedef struct {
    uint32_t FUSE_RESERVED_ODM8[0xE];       /* Mariko only. */
    uint32_t FUSE_KEK[0x4];                 /* Mariko only. */
    uint32_t FUSE_BEK[0x4];                 /* Mariko only. */
    uint32_t _0xF0;                         /* Mariko only. */
    uint32_t _0xF4;                         /* Mariko only. */
    uint32_t _0xF8;                         /* Mariko only. */
    uint32_t _0xFC;                         /* Mariko only. */
    uint32_t FUSE_PRODUCTION_MODE;
    uint32_t FUSE_JTAG_SECUREID_VALID;
    uint32_t FUSE_ODM_LOCK;
    uint32_t FUSE_OPT_OPENGL_EN;
    uint32_t FUSE_SKU_INFO;
    uint32_t FUSE_CPU_SPEEDO_0_CALIB;
    uint32_t FUSE_CPU_IDDQ_CALIB;
    uint32_t FUSE_RESERVED_ODM22[0x3];      /* Mariko only. */
    uint32_t FUSE_OPT_FT_REV;
    uint32_t FUSE_CPU_SPEEDO_1_CALIB;
    uint32_t FUSE_CPU_SPEEDO_2_CALIB;
    uint32_t FUSE_SOC_SPEEDO_0_CALIB;
    uint32_t FUSE_SOC_SPEEDO_1_CALIB;
    uint32_t FUSE_SOC_SPEEDO_2_CALIB;
    uint32_t FUSE_SOC_IDDQ_CALIB;
    uint32_t FUSE_RESERVED_ODM25;           /* Mariko only. */
    uint32_t FUSE_FA;
    uint32_t FUSE_RESERVED_PRODUCTION;
    uint32_t FUSE_HDMI_LANE0_CALIB;
    uint32_t FUSE_HDMI_LANE1_CALIB;
    uint32_t FUSE_HDMI_LANE2_CALIB;
    uint32_t FUSE_HDMI_LANE3_CALIB;
    uint32_t FUSE_ENCRYPTION_RATE;
    uint32_t FUSE_PUBLIC_KEY[0x8];
    uint32_t FUSE_TSENSOR1_CALIB;
    uint32_t FUSE_TSENSOR2_CALIB;
    uint32_t FUSE_OPT_SECURE_SCC_DIS;       /* Mariko only. */
    uint32_t FUSE_OPT_CP_REV;
    uint32_t FUSE_OPT_PFG;
    uint32_t FUSE_TSENSOR0_CALIB;
    uint32_t FUSE_FIRST_BOOTROM_PATCH_SIZE;
    uint32_t FUSE_SECURITY_MODE;
    uint32_t FUSE_PRIVATE_KEY[0x5];
    uint32_t FUSE_ARM_JTAG_DIS;
    uint32_t FUSE_BOOT_DEVICE_INFO;
    uint32_t FUSE_RESERVED_SW;
    uint32_t FUSE_OPT_VP9_DISABLE;
    uint32_t FUSE_RESERVED_ODM0[0x8];
    uint32_t FUSE_OBS_DIS;
    uint32_t _0x1EC;                        /* Mariko only. */
    uint32_t FUSE_USB_CALIB;
    uint32_t FUSE_SKU_DIRECT_CONFIG;
    uint32_t FUSE_KFUSE_PRIVKEY_CTRL;
    uint32_t FUSE_PACKAGE_INFO;
    uint32_t FUSE_OPT_VENDOR_CODE;
    uint32_t FUSE_OPT_FAB_CODE;
    uint32_t FUSE_OPT_LOT_CODE_0;
    uint32_t FUSE_OPT_LOT_CODE_1;
    uint32_t FUSE_OPT_WAFER_ID;
    uint32_t FUSE_OPT_X_COORDINATE;
    uint32_t FUSE_OPT_Y_COORDINATE;
    uint32_t FUSE_OPT_SEC_DEBUG_EN;
    uint32_t FUSE_OPT_OPS_RESERVED;
    uint32_t _0x224;                        /* Mariko only. */
    uint32_t FUSE_GPU_IDDQ_CALIB;
    uint32_t FUSE_TSENSOR3_CALIB;
    uint32_t FUSE_CLOCK_BOUNDOUT0;
    uint32_t FUSE_CLOCK_BOUNDOUT1;
    uint32_t FUSE_RESERVED_ODM26[0x3];      /* Mariko only. */
    uint32_t FUSE_OPT_SAMPLE_TYPE;
    uint32_t FUSE_OPT_SUBREVISION;
    uint32_t FUSE_OPT_SW_RESERVED_0;
    uint32_t FUSE_OPT_SW_RESERVED_1;
    uint32_t FUSE_TSENSOR4_CALIB;
    uint32_t FUSE_TSENSOR5_CALIB;
    uint32_t FUSE_TSENSOR6_CALIB;
    uint32_t FUSE_TSENSOR7_CALIB;
    uint32_t FUSE_OPT_PRIV_SEC_EN;
    uint32_t FUSE_BOOT_SECURITY_INFO;       /* Mariko only. */
    uint32_t _0x26C;                        /* Mariko only. */
    uint32_t _0x270;                        /* Mariko only. */
    uint32_t _0x274;                        /* Mariko only. */
    uint32_t _0x278;                        /* Mariko only. */
    uint32_t FUSE_FUSE2TSEC_DEBUG_DISABLE;
    uint32_t FUSE_TSENSOR_COMMON;
    uint32_t FUSE_OPT_CP_BIN;
    uint32_t FUSE_OPT_GPU_DISABLE;
    uint32_t FUSE_OPT_FT_BIN;
    uint32_t FUSE_OPT_DONE_MAP;
    uint32_t FUSE_RESERVED_ODM29;           /* Mariko only. */
    uint32_t FUSE_APB2JTAG_DISABLE;
    uint32_t FUSE_ODM_INFO;
    uint32_t _0x2A0[0x2];
    uint32_t FUSE_ARM_CRYPT_DE_FEATURE;
    uint32_t _0x2AC;
    uint32_t _0x2B0;                        /* Mariko only. */
    uint32_t _0x2B4;                        /* Mariko only. */
    uint32_t _0x2B8;                        /* Mariko only. */
    uint32_t _0x2BC;                        /* Mariko only. */
    uint32_t FUSE_WOA_SKU_FLAG;
    uint32_t FUSE_ECO_RESERVE_1;
    uint32_t FUSE_GCPLEX_CONFIG_FUSE;
    uint32_t FUSE_PRODUCTION_MONTH;
    uint32_t FUSE_RAM_REPAIR_INDICATOR;
    uint32_t FUSE_TSENSOR9_CALIB;
    uint32_t _0x2D8;
    uint32_t FUSE_VMIN_CALIBRATION;
    uint32_t FUSE_AGING_SENSOR_CALIBRATION;
    uint32_t FUSE_DEBUG_AUTHENTICATION;
    uint32_t FUSE_SECURE_PROVISION_INDEX;
    uint32_t FUSE_SECURE_PROVISION_INFO;
    uint32_t FUSE_OPT_GPU_DISABLE_CP1;
    uint32_t FUSE_SPARE_ENDIS;
    uint32_t FUSE_ECO_RESERVE_0;
    uint32_t _0x2FC[0x2];
    uint32_t FUSE_RESERVED_CALIB0;
    uint32_t FUSE_RESERVED_CALIB1;
    uint32_t FUSE_OPT_GPU_TPC0_DISABLE;
    uint32_t FUSE_OPT_GPU_TPC0_DISABLE_CP1;
    uint32_t FUSE_OPT_CPU_DISABLE;
    uint32_t FUSE_OPT_CPU_DISABLE_CP1;
    uint32_t FUSE_TSENSOR10_CALIB;
    uint32_t FUSE_TSENSOR10_CALIB_AUX;
    uint32_t _0x324;                        /* Mariko only. */
    uint32_t _0x328;                        /* Mariko only. */
    uint32_t _0x32C;                        /* Mariko only. */
    uint32_t _0x330;                        /* Mariko only. */
    uint32_t _0x334;                        /* Mariko only. */
    uint32_t FUSE_OPT_GPU_TPC0_DISABLE_CP2;
    uint32_t FUSE_OPT_GPU_TPC1_DISABLE;
    uint32_t FUSE_OPT_GPU_TPC1_DISABLE_CP1;
    uint32_t FUSE_OPT_GPU_TPC1_DISABLE_CP2;
    uint32_t FUSE_OPT_CPU_DISABLE_CP2;
    uint32_t FUSE_OPT_GPU_DISABLE_CP2;
    uint32_t FUSE_USB_CALIB_EXT;
    uint32_t FUSE_RESERVED_FIELD;
    uint32_t _0x358[0x9];
    uint32_t FUSE_SPARE_REALIGNMENT_REG;
    uint32_t FUSE_SPARE_BIT[0x1E];
} tegra_fuse_chip_mariko_t;

static inline volatile tegra_fuse_t *fuse_get_regs(void)
{
    return (volatile tegra_fuse_t *)FUSE_BASE;
}

static inline volatile tegra_fuse_chip_common_t *fuse_chip_common_get_regs(void)
{
    return (volatile tegra_fuse_chip_common_t *)FUSE_CHIP_BASE;
}

static inline volatile tegra_fuse_chip_erista_t *fuse_chip_erista_get_regs(void)
{
    return (volatile tegra_fuse_chip_erista_t *)FUSE_CHIP_BASE;
}

static inline volatile tegra_fuse_chip_mariko_t *fuse_chip_mariko_get_regs(void)
{
    return (volatile tegra_fuse_chip_mariko_t *)FUSE_CHIP_BASE;
}

void fuse_init(void);
void fuse_disable_programming(void);
void fuse_disable_private_key(void);
void fuse_enable_power(void);
void fuse_disable_power(void);

uint32_t fuse_get_sku_info(void);
uint32_t fuse_get_spare_bit(uint32_t index);
uint32_t fuse_get_reserved_odm(uint32_t index);
uint32_t fuse_get_bootrom_patch_version(void);
uint64_t fuse_get_device_id(void);
uint32_t fuse_get_dram_id(void);
uint32_t fuse_get_hardware_type_with_firmware_check(uint32_t target_firmware);
uint32_t fuse_get_hardware_type(void);
uint32_t fuse_get_hardware_state(void);
void fuse_get_hardware_info(void *dst);
bool fuse_is_new_format(void);
uint32_t fuse_get_device_unique_key_generation(void);
uint32_t fuse_get_soc_type(void);
uint32_t fuse_get_regulator(void);

uint32_t fuse_get_expected_fuse_count(uint32_t target_firmware);
uint32_t fuse_get_burnt_fuse_count(void);

uint32_t fuse_hw_read(uint32_t addr);
void fuse_hw_write(uint32_t value, uint32_t addr);
void fuse_hw_sense(void);

#endif
