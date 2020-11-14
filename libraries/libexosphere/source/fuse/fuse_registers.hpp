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

namespace ams::fuse {

    struct FuseRegisters {
        u32 FUSE_FUSECTRL;
        u32 FUSE_FUSEADDR;
        u32 FUSE_FUSERDATA;
        u32 FUSE_FUSEWDATA;
        u32 FUSE_FUSETIME_RD1;
        u32 FUSE_FUSETIME_RD2;
        u32 FUSE_FUSETIME_PGM1;
        u32 FUSE_FUSETIME_PGM2;
        u32 FUSE_PRIV2INTFC_START;
        u32 FUSE_FUSEBYPASS;
        u32 FUSE_PRIVATEKEYDISABLE;
        u32 FUSE_DISABLEREGPROGRAM;
        u32 FUSE_WRITE_ACCESS_SW;
        u32 FUSE_PWR_GOOD_SW;
        u32 _0x38;
        u32 FUSE_PRIV2RESHIFT;
        u32 _0x40[0x3];
        u32 FUSE_FUSETIME_RD3;
        u32 _0x50[0xC];
        u32 FUSE_PRIVATE_KEY0_NONZERO;
        u32 FUSE_PRIVATE_KEY1_NONZERO;
        u32 FUSE_PRIVATE_KEY2_NONZERO;
        u32 FUSE_PRIVATE_KEY3_NONZERO;
        u32 FUSE_PRIVATE_KEY4_NONZERO;
        u32 _0x94;
    };
    static_assert(util::is_pod<FuseRegisters>::value);
    static_assert(sizeof(FuseRegisters) == 0x98);

    struct FuseChipRegistersCommon {
        u32 _0x98[0x1A];
        u32 FUSE_PRODUCTION_MODE;
        u32 FUSE_JTAG_SECUREID_VALID;
        u32 FUSE_ODM_LOCK;
        u32 FUSE_OPT_OPENGL_EN;
        u32 FUSE_SKU_INFO;
        u32 FUSE_CPU_SPEEDO_0_CALIB;
        u32 FUSE_CPU_IDDQ_CALIB;
        u32 _0x11C;
        u32 _0x120;
        u32 _0x124;
        u32 FUSE_OPT_FT_REV;
        u32 FUSE_CPU_SPEEDO_1_CALIB;
        u32 FUSE_CPU_SPEEDO_2_CALIB;
        u32 FUSE_SOC_SPEEDO_0_CALIB;
        u32 FUSE_SOC_SPEEDO_1_CALIB;
        u32 FUSE_SOC_SPEEDO_2_CALIB;
        u32 FUSE_SOC_IDDQ_CALIB;
        u32 _0x144;
        u32 FUSE_FA;
        u32 FUSE_RESERVED_PRODUCTION;
        u32 FUSE_HDMI_LANE0_CALIB;
        u32 FUSE_HDMI_LANE1_CALIB;
        u32 FUSE_HDMI_LANE2_CALIB;
        u32 FUSE_HDMI_LANE3_CALIB;
        u32 FUSE_ENCRYPTION_RATE;
        u32 FUSE_PUBLIC_KEY[0x8];
        u32 FUSE_TSENSOR1_CALIB;
        u32 FUSE_TSENSOR2_CALIB;
        u32 _0x18C;
        u32 FUSE_OPT_CP_REV;
        u32 FUSE_OPT_PFG;
        u32 FUSE_TSENSOR0_CALIB;
        u32 FUSE_FIRST_BOOTROM_PATCH_SIZE;
        u32 FUSE_SECURITY_MODE;
        u32 FUSE_PRIVATE_KEY[0x5];
        u32 FUSE_ARM_JTAG_DIS;
        u32 FUSE_BOOT_DEVICE_INFO;
        u32 FUSE_RESERVED_SW;
        u32 FUSE_OPT_VP9_DISABLE;
        u32 FUSE_RESERVED_ODM_0[8 - 0];
        u32 FUSE_OBS_DIS;
        u32 _0x1EC;
        u32 FUSE_USB_CALIB;
        u32 FUSE_SKU_DIRECT_CONFIG;
        u32 FUSE_KFUSE_PRIVKEY_CTRL;
        u32 FUSE_PACKAGE_INFO;
        u32 FUSE_OPT_VENDOR_CODE;
        u32 FUSE_OPT_FAB_CODE;
        u32 FUSE_OPT_LOT_CODE_0;
        u32 FUSE_OPT_LOT_CODE_1;
        u32 FUSE_OPT_WAFER_ID;
        u32 FUSE_OPT_X_COORDINATE;
        u32 FUSE_OPT_Y_COORDINATE;
        u32 FUSE_OPT_SEC_DEBUG_EN;
        u32 FUSE_OPT_OPS_RESERVED;
        u32 _0x224;
        u32 FUSE_GPU_IDDQ_CALIB;
        u32 FUSE_TSENSOR3_CALIB;
        u32 _0x234;
        u32 _0x238;
        u32 _0x23C;
        u32 _0x240;
        u32 _0x244;
        u32 FUSE_OPT_SAMPLE_TYPE;
        u32 FUSE_OPT_SUBREVISION;
        u32 FUSE_OPT_SW_RESERVED_0;
        u32 FUSE_OPT_SW_RESERVED_1;
        u32 FUSE_TSENSOR4_CALIB;
        u32 FUSE_TSENSOR5_CALIB;
        u32 FUSE_TSENSOR6_CALIB;
        u32 FUSE_TSENSOR7_CALIB;
        u32 FUSE_OPT_PRIV_SEC_EN;
        u32 _0x268;
        u32 _0x26C;
        u32 _0x270;
        u32 _0x274;
        u32 _0x278;
        u32 FUSE_FUSE2TSEC_DEBUG_DISABLE;
        u32 FUSE_TSENSOR_COMMON;
        u32 FUSE_OPT_CP_BIN;
        u32 FUSE_OPT_GPU_DISABLE;
        u32 FUSE_OPT_FT_BIN;
        u32 FUSE_OPT_DONE_MAP;
        u32 _0x294;
        u32 FUSE_APB2JTAG_DISABLE;
        u32 FUSE_ODM_INFO;
        u32 _0x2A0;
        u32 _0x2A4;
        u32 FUSE_ARM_CRYPT_DE_FEATURE;
        u32 _0x2AC;
        u32 _0x2B0;
        u32 _0x2B4;
        u32 _0x2B8;
        u32 _0x2BC;
        u32 FUSE_WOA_SKU_FLAG;
        u32 FUSE_ECO_RESERVE_1;
        u32 FUSE_GCPLEX_CONFIG_FUSE;
        u32 FUSE_PRODUCTION_MONTH;
        u32 FUSE_RAM_REPAIR_INDICATOR;
        u32 FUSE_TSENSOR9_CALIB;
        u32 _0x2D8;
        u32 FUSE_VMIN_CALIBRATION;
        u32 FUSE_AGING_SENSOR_CALIBRATION;
        u32 FUSE_DEBUG_AUTHENTICATION;
        u32 FUSE_SECURE_PROVISION_INDEX;
        u32 FUSE_SECURE_PROVISION_INFO;
        u32 FUSE_OPT_GPU_DISABLE_CP1;
        u32 FUSE_SPARE_ENDIS;
        u32 FUSE_ECO_RESERVE_0;
        u32 _0x2FC;
        u32 _0x300;
        u32 FUSE_RESERVED_CALIB0;
        u32 FUSE_RESERVED_CALIB1;
        u32 FUSE_OPT_GPU_TPC0_DISABLE;
        u32 FUSE_OPT_GPU_TPC0_DISABLE_CP1;
        u32 FUSE_OPT_CPU_DISABLE;
        u32 FUSE_OPT_CPU_DISABLE_CP1;
        u32 FUSE_TSENSOR10_CALIB;
        u32 FUSE_TSENSOR10_CALIB_AUX;
        u32 _0x324;
        u32 _0x328;
        u32 _0x32C;
        u32 _0x330;
        u32 _0x334;
        u32 FUSE_OPT_GPU_TPC0_DISABLE_CP2;
        u32 FUSE_OPT_GPU_TPC1_DISABLE;
        u32 FUSE_OPT_GPU_TPC1_DISABLE_CP1;
        u32 FUSE_OPT_GPU_TPC1_DISABLE_CP2;
        u32 FUSE_OPT_CPU_DISABLE_CP2;
        u32 FUSE_OPT_GPU_DISABLE_CP2;
        u32 FUSE_USB_CALIB_EXT;
        u32 FUSE_RESERVED_FIELD;
        u32 _0x358;
        u32 _0x35C;
        u32 _0x360;
        u32 _0x364;
        u32 _0x368;
        u32 _0x36C;
        u32 _0x370;
        u32 _0x374;
        u32 _0x378;
        u32 FUSE_SPARE_REALIGNMENT_REG;
        u32 FUSE_SPARE_BIT[0x20];
    };
    static_assert(util::is_pod<FuseChipRegistersCommon>::value);
    static_assert(sizeof(FuseChipRegistersCommon) == 0x400 - 0x98);

    struct FuseChipRegistersErista {
        u32 _0x98[0x1A];
        u32 FUSE_PRODUCTION_MODE;
        u32 FUSE_JTAG_SECUREID_VALID;
        u32 FUSE_ODM_LOCK;
        u32 FUSE_OPT_OPENGL_EN;
        u32 FUSE_SKU_INFO;
        u32 FUSE_CPU_SPEEDO_0_CALIB;
        u32 FUSE_CPU_IDDQ_CALIB;
        u32 FUSE_DAC_CRT_CALIB;
        u32 FUSE_DAC_HDTV_CALIB;
        u32 FUSE_DAC_SDTV_CALIB;
        u32 FUSE_OPT_FT_REV;
        u32 FUSE_CPU_SPEEDO_1_CALIB;
        u32 FUSE_CPU_SPEEDO_2_CALIB;
        u32 FUSE_SOC_SPEEDO_0_CALIB;
        u32 FUSE_SOC_SPEEDO_1_CALIB;
        u32 FUSE_SOC_SPEEDO_2_CALIB;
        u32 FUSE_SOC_IDDQ_CALIB;
        u32 FUSE_RESERVED_PRODUCTION_WP;
        u32 FUSE_FA;
        u32 FUSE_RESERVED_PRODUCTION;
        u32 FUSE_HDMI_LANE0_CALIB;
        u32 FUSE_HDMI_LANE1_CALIB;
        u32 FUSE_HDMI_LANE2_CALIB;
        u32 FUSE_HDMI_LANE3_CALIB;
        u32 FUSE_ENCRYPTION_RATE;
        u32 FUSE_PUBLIC_KEY[0x8];
        u32 FUSE_TSENSOR1_CALIB;
        u32 FUSE_TSENSOR2_CALIB;
        u32 FUSE_VSENSOR_CALIB;
        u32 FUSE_OPT_CP_REV;
        u32 FUSE_OPT_PFG;
        u32 FUSE_TSENSOR0_CALIB;
        u32 FUSE_FIRST_BOOTROM_PATCH_SIZE;
        u32 FUSE_SECURITY_MODE;
        u32 FUSE_PRIVATE_KEY[0x5];
        u32 FUSE_ARM_JTAG_DIS;
        u32 FUSE_BOOT_DEVICE_INFO;
        u32 FUSE_RESERVED_SW;
        u32 FUSE_OPT_VP9_DISABLE;
        u32 FUSE_RESERVED_ODM_0[8 - 0];
        u32 FUSE_OBS_DIS;
        u32 FUSE_NOR_INFO;
        u32 FUSE_USB_CALIB;
        u32 FUSE_SKU_DIRECT_CONFIG;
        u32 FUSE_KFUSE_PRIVKEY_CTRL;
        u32 FUSE_PACKAGE_INFO;
        u32 FUSE_OPT_VENDOR_CODE;
        u32 FUSE_OPT_FAB_CODE;
        u32 FUSE_OPT_LOT_CODE_0;
        u32 FUSE_OPT_LOT_CODE_1;
        u32 FUSE_OPT_WAFER_ID;
        u32 FUSE_OPT_X_COORDINATE;
        u32 FUSE_OPT_Y_COORDINATE;
        u32 FUSE_OPT_SEC_DEBUG_EN;
        u32 FUSE_OPT_OPS_RESERVED;
        u32 FUSE_SATA_CALIB;
        u32 FUSE_GPU_IDDQ_CALIB;
        u32 FUSE_TSENSOR3_CALIB;
        u32 FUSE_SKU_BOND_OUT_L;
        u32 FUSE_SKU_BOND_OUT_H;
        u32 FUSE_SKU_BOND_OUT_U;
        u32 FUSE_SKU_BOND_OUT_V;
        u32 FUSE_SKU_BOND_OUT_W;
        u32 FUSE_OPT_SAMPLE_TYPE;
        u32 FUSE_OPT_SUBREVISION;
        u32 FUSE_OPT_SW_RESERVED_0;
        u32 FUSE_OPT_SW_RESERVED_1;
        u32 FUSE_TSENSOR4_CALIB;
        u32 FUSE_TSENSOR5_CALIB;
        u32 FUSE_TSENSOR6_CALIB;
        u32 FUSE_TSENSOR7_CALIB;
        u32 FUSE_OPT_PRIV_SEC_EN;
        u32 FUSE_PKC_DISABLE;
        u32 _0x26C;
        u32 _0x270;
        u32 _0x274;
        u32 _0x278;
        u32 FUSE_FUSE2TSEC_DEBUG_DISABLE;
        u32 FUSE_TSENSOR_COMMON;
        u32 FUSE_OPT_CP_BIN;
        u32 FUSE_OPT_GPU_DISABLE;
        u32 FUSE_OPT_FT_BIN;
        u32 FUSE_OPT_DONE_MAP;
        u32 _0x294;
        u32 FUSE_APB2JTAG_DISABLE;
        u32 FUSE_ODM_INFO;
        u32 _0x2A0;
        u32 _0x2A4;
        u32 FUSE_ARM_CRYPT_DE_FEATURE;
        u32 _0x2AC;
        u32 _0x2B0;
        u32 _0x2B4;
        u32 _0x2B8;
        u32 _0x2BC;
        u32 FUSE_WOA_SKU_FLAG;
        u32 FUSE_ECO_RESERVE_1;
        u32 FUSE_GCPLEX_CONFIG_FUSE;
        u32 FUSE_PRODUCTION_MONTH;
        u32 FUSE_RAM_REPAIR_INDICATOR;
        u32 FUSE_TSENSOR9_CALIB;
        u32 _0x2D8;
        u32 FUSE_VMIN_CALIBRATION;
        u32 FUSE_AGING_SENSOR_CALIBRATION;
        u32 FUSE_DEBUG_AUTHENTICATION;
        u32 FUSE_SECURE_PROVISION_INDEX;
        u32 FUSE_SECURE_PROVISION_INFO;
        u32 FUSE_OPT_GPU_DISABLE_CP1;
        u32 FUSE_SPARE_ENDIS;
        u32 FUSE_ECO_RESERVE_0;
        u32 _0x2FC;
        u32 _0x300;
        u32 FUSE_RESERVED_CALIB0;
        u32 FUSE_RESERVED_CALIB1;
        u32 FUSE_OPT_GPU_TPC0_DISABLE;
        u32 FUSE_OPT_GPU_TPC0_DISABLE_CP1;
        u32 FUSE_OPT_CPU_DISABLE;
        u32 FUSE_OPT_CPU_DISABLE_CP1;
        u32 FUSE_TSENSOR10_CALIB;
        u32 FUSE_TSENSOR10_CALIB_AUX;
        u32 FUSE_OPT_RAM_SVOP_DP;
        u32 FUSE_OPT_RAM_SVOP_PDP;
        u32 FUSE_OPT_RAM_SVOP_REG;
        u32 FUSE_OPT_RAM_SVOP_SP;
        u32 FUSE_OPT_RAM_SVOP_SMPDP;
        u32 FUSE_OPT_GPU_TPC0_DISABLE_CP2;
        u32 FUSE_OPT_GPU_TPC1_DISABLE;
        u32 FUSE_OPT_GPU_TPC1_DISABLE_CP1;
        u32 FUSE_OPT_GPU_TPC1_DISABLE_CP2;
        u32 FUSE_OPT_CPU_DISABLE_CP2;
        u32 FUSE_OPT_GPU_DISABLE_CP2;
        u32 FUSE_USB_CALIB_EXT;
        u32 FUSE_RESERVED_FIELD;
        u32 FUSE_OPT_ECC_EN;
        u32 _0x35C;
        u32 _0x360;
        u32 _0x364;
        u32 _0x368;
        u32 _0x36C;
        u32 _0x370;
        u32 _0x374;
        u32 _0x378;
        u32 FUSE_SPARE_REALIGNMENT_REG;
        u32 FUSE_SPARE_BIT[0x20];
    };
    static_assert(util::is_pod<FuseChipRegistersErista>::value);
    static_assert(sizeof(FuseChipRegistersErista) == 0x400 - 0x98);

    struct FuseChipRegistersMariko {
        u32 FUSE_RESERVED_ODM_8[22 - 8];
        u32 FUSE_KEK[4];
        u32 FUSE_BEK[4];
        u32 _0xF0[4];
        u32 FUSE_PRODUCTION_MODE;
        u32 FUSE_JTAG_SECUREID_VALID;
        u32 FUSE_ODM_LOCK;
        u32 FUSE_OPT_OPENGL_EN;
        u32 FUSE_SKU_INFO;
        u32 FUSE_CPU_SPEEDO_0_CALIB;
        u32 FUSE_CPU_IDDQ_CALIB;
        u32 FUSE_RESERVED_ODM_22[25 - 22];
        u32 FUSE_OPT_FT_REV;
        u32 FUSE_CPU_SPEEDO_1_CALIB;
        u32 FUSE_CPU_SPEEDO_2_CALIB;
        u32 FUSE_SOC_SPEEDO_0_CALIB;
        u32 FUSE_SOC_SPEEDO_1_CALIB;
        u32 FUSE_SOC_SPEEDO_2_CALIB;
        u32 FUSE_SOC_IDDQ_CALIB;
        u32 FUSE_RESERVED_ODM_25[26 - 25];
        u32 FUSE_FA;
        u32 FUSE_RESERVED_PRODUCTION;
        u32 FUSE_HDMI_LANE0_CALIB;
        u32 FUSE_HDMI_LANE1_CALIB;
        u32 FUSE_HDMI_LANE2_CALIB;
        u32 FUSE_HDMI_LANE3_CALIB;
        u32 FUSE_ENCRYPTION_RATE;
        u32 FUSE_PUBLIC_KEY[0x8];
        u32 FUSE_TSENSOR1_CALIB;
        u32 FUSE_TSENSOR2_CALIB;
        u32 FUSE_OPT_SECURE_SCC_DIS;
        u32 FUSE_OPT_CP_REV;
        u32 FUSE_OPT_PFG;
        u32 FUSE_TSENSOR0_CALIB;
        u32 FUSE_FIRST_BOOTROM_PATCH_SIZE;
        u32 FUSE_SECURITY_MODE;
        u32 FUSE_PRIVATE_KEY[0x5];
        u32 FUSE_ARM_JTAG_DIS;
        u32 FUSE_BOOT_DEVICE_INFO;
        u32 FUSE_RESERVED_SW;
        u32 FUSE_OPT_VP9_DISABLE;
        u32 FUSE_RESERVED_ODM_0[8 - 0];
        u32 FUSE_OBS_DIS;
        u32 _0x1EC;
        u32 FUSE_USB_CALIB;
        u32 FUSE_SKU_DIRECT_CONFIG;
        u32 FUSE_KFUSE_PRIVKEY_CTRL;
        u32 FUSE_PACKAGE_INFO;
        u32 FUSE_OPT_VENDOR_CODE;
        u32 FUSE_OPT_FAB_CODE;
        u32 FUSE_OPT_LOT_CODE_0;
        u32 FUSE_OPT_LOT_CODE_1;
        u32 FUSE_OPT_WAFER_ID;
        u32 FUSE_OPT_X_COORDINATE;
        u32 FUSE_OPT_Y_COORDINATE;
        u32 FUSE_OPT_SEC_DEBUG_EN;
        u32 FUSE_OPT_OPS_RESERVED;
        u32 _0x224;
        u32 FUSE_GPU_IDDQ_CALIB;
        u32 FUSE_TSENSOR3_CALIB;
        u32 FUSE_CLOCK_BONDOUT0;
        u32 FUSE_CLOCK_BONDOUT1;
        u32 FUSE_RESERVED_ODM_26[29 - 26];
        u32 FUSE_OPT_SAMPLE_TYPE;
        u32 FUSE_OPT_SUBREVISION;
        u32 FUSE_OPT_SW_RESERVED_0;
        u32 FUSE_OPT_SW_RESERVED_1;
        u32 FUSE_TSENSOR4_CALIB;
        u32 FUSE_TSENSOR5_CALIB;
        u32 FUSE_TSENSOR6_CALIB;
        u32 FUSE_TSENSOR7_CALIB;
        u32 FUSE_OPT_PRIV_SEC_EN;
        u32 FUSE_BOOT_SECURITY_INFO;
        u32 _0x26C;
        u32 _0x270;
        u32 _0x274;
        u32 _0x278;
        u32 FUSE_FUSE2TSEC_DEBUG_DISABLE;
        u32 FUSE_TSENSOR_COMMON;
        u32 FUSE_OPT_CP_BIN;
        u32 FUSE_OPT_GPU_DISABLE;
        u32 FUSE_OPT_FT_BIN;
        u32 FUSE_OPT_DONE_MAP;
        u32 FUSE_RESERVED_ODM_29[30 - 29];
        u32 FUSE_APB2JTAG_DISABLE;
        u32 FUSE_ODM_INFO;
        u32 _0x2A0;
        u32 _0x2A4;
        u32 FUSE_ARM_CRYPT_DE_FEATURE;
        u32 _0x2AC;
        u32 _0x2B0;
        u32 _0x2B4;
        u32 _0x2B8;
        u32 _0x2BC;
        u32 FUSE_WOA_SKU_FLAG;
        u32 FUSE_ECO_RESERVE_1;
        u32 FUSE_GCPLEX_CONFIG_FUSE;
        u32 FUSE_PRODUCTION_MONTH;
        u32 FUSE_RAM_REPAIR_INDICATOR;
        u32 FUSE_TSENSOR9_CALIB;
        u32 _0x2D8;
        u32 FUSE_VMIN_CALIBRATION;
        u32 FUSE_AGING_SENSOR_CALIBRATION;
        u32 FUSE_DEBUG_AUTHENTICATION;
        u32 FUSE_SECURE_PROVISION_INDEX;
        u32 FUSE_SECURE_PROVISION_INFO;
        u32 FUSE_OPT_GPU_DISABLE_CP1;
        u32 FUSE_SPARE_ENDIS;
        u32 FUSE_ECO_RESERVE_0;
        u32 _0x2FC;
        u32 _0x300;
        u32 FUSE_RESERVED_CALIB0;
        u32 FUSE_RESERVED_CALIB1;
        u32 FUSE_OPT_GPU_TPC0_DISABLE;
        u32 FUSE_OPT_GPU_TPC0_DISABLE_CP1;
        u32 FUSE_OPT_CPU_DISABLE;
        u32 FUSE_OPT_CPU_DISABLE_CP1;
        u32 FUSE_TSENSOR10_CALIB;
        u32 FUSE_TSENSOR10_CALIB_AUX;
        u32 _0x324;
        u32 _0x328;
        u32 _0x32C;
        u32 _0x330;
        u32 _0x334;
        u32 FUSE_OPT_GPU_TPC0_DISABLE_CP2;
        u32 FUSE_OPT_GPU_TPC1_DISABLE;
        u32 FUSE_OPT_GPU_TPC1_DISABLE_CP1;
        u32 FUSE_OPT_GPU_TPC1_DISABLE_CP2;
        u32 FUSE_OPT_CPU_DISABLE_CP2;
        u32 FUSE_OPT_GPU_DISABLE_CP2;
        u32 FUSE_USB_CALIB_EXT;
        u32 FUSE_RESERVED_FIELD;
        u32 _0x358;
        u32 _0x35C;
        u32 _0x360;
        u32 _0x364;
        u32 _0x368;
        u32 _0x36C;
        u32 _0x370;
        u32 _0x374;
        u32 _0x378;
        u32 FUSE_SPARE_REALIGNMENT_REG;
        u32 FUSE_SPARE_BIT[0x20];
    };
    static_assert(util::is_pod<FuseChipRegistersMariko>::value);
    static_assert(sizeof(FuseChipRegistersMariko) == 0x400 - 0x98);

    struct FuseRegisterRegion {
        FuseRegisters fuse;
        union {
            FuseChipRegistersCommon chip_common;
            FuseChipRegistersErista chip_erista;
            FuseChipRegistersMariko chip_mariko;
        };
    };
    static_assert(util::is_pod<FuseRegisterRegion>::value);
    static_assert(sizeof(FuseRegisterRegion) == secmon::MemoryRegionPhysicalDeviceFuses.GetSize());

    #define FUSE_REG_BITS_MASK(NAME)                                      REG_NAMED_BITS_MASK    (FUSE, NAME)
    #define FUSE_REG_BITS_VALUE(NAME, VALUE)                              REG_NAMED_BITS_VALUE   (FUSE, NAME, VALUE)
    #define FUSE_REG_BITS_ENUM(NAME, ENUM)                                REG_NAMED_BITS_ENUM    (FUSE, NAME, ENUM)
    #define FUSE_REG_BITS_ENUM_SEL(NAME, __COND__, TRUE_ENUM, FALSE_ENUM) REG_NAMED_BITS_ENUM_SEL(FUSE, NAME, __COND__, TRUE_ENUM, FALSE_ENUM)

    #define DEFINE_FUSE_REG(NAME, __OFFSET__, __WIDTH__)                                                                                                                  REG_DEFINE_NAMED_REG           (FUSE, NAME, __OFFSET__, __WIDTH__)
    #define DEFINE_FUSE_REG_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE)                                                                                                         REG_DEFINE_NAMED_BIT_ENUM      (FUSE, NAME, __OFFSET__, ZERO, ONE)
    #define DEFINE_FUSE_REG_TWO_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE)                                                                                         REG_DEFINE_NAMED_TWO_BIT_ENUM  (FUSE, NAME, __OFFSET__, ZERO, ONE, TWO, THREE)
    #define DEFINE_FUSE_REG_THREE_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)                                                               REG_DEFINE_NAMED_THREE_BIT_ENUM(FUSE, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN)
    #define DEFINE_FUSE_REG_FOUR_BIT_ENUM(NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN) REG_DEFINE_NAMED_FOUR_BIT_ENUM (FUSE, NAME, __OFFSET__, ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, ELEVEN, TWELVE, THIRTEEN, FOURTEEN, FIFTEEN)

    DEFINE_FUSE_REG_TWO_BIT_ENUM(FUSECTRL_CMD, 0, IDLE, READ, WRITE, SENSE_CTRL);

    DEFINE_FUSE_REG(FUSECTRL_STATE, 16, 5);

    enum FUSE_FUSECTRL_STATE {
        FUSE_FUSECTRL_STATE_RESET                   =  0,
        FUSE_FUSECTRL_STATE_POST_RESET              =  1,
        FUSE_FUSECTRL_STATE_LOAD_ROW0               =  2,
        FUSE_FUSECTRL_STATE_LOAD_ROW1               =  3,
        FUSE_FUSECTRL_STATE_IDLE                    =  4,
        FUSE_FUSECTRL_STATE_READ_SETUP              =  5,
        FUSE_FUSECTRL_STATE_READ_STROBE             =  6,
        FUSE_FUSECTRL_STATE_SAMPLE_FUSES            =  7,
        FUSE_FUSECTRL_STATE_READ_HOLD               =  8,
        FUSE_FUSECTRL_STATE_FUSE_SRC_SETUP          =  9,
        FUSE_FUSECTRL_STATE_WRITE_SETUP             = 10,
        FUSE_FUSECTRL_STATE_WRITE_ADDR_SETUP        = 11,
        FUSE_FUSECTRL_STATE_WRITE_PROGRAM           = 12,
        FUSE_FUSECTRL_STATE_WRITE_ADDR_HOLD         = 13,
        FUSE_FUSECTRL_STATE_FUSE_SRC_HOLD           = 14,
        FUSE_FUSECTRL_STATE_LOAD_RIR                = 15,
        FUSE_FUSECTRL_STATE_READ_BEFORE_WRITE_SETUP = 16,
        FUSE_FUSECTRL_STATE_READ_DEASSERT_PD        = 17,
    };

    DEFINE_FUSE_REG_BIT_ENUM(PRIVATEKEYDISABLE_TZ_STICKY_BIT_VAL, 4, KEY_VISIBLE, KEY_INVISIBLE);
    DEFINE_FUSE_REG_BIT_ENUM(PRIVATEKEYDISABLE_PRIVATEKEYDISABLE_VAL_KEY, 0, VISIBLE, INVISIBLE);

    DEFINE_FUSE_REG_BIT_ENUM(FUSEBYPASS_VAL, 0, DISABLE, ENABLE);

    DEFINE_FUSE_REG_BIT_ENUM(DISABLEREGPROGRAM_VAL, 0, DISABLE, ENABLE);

    DEFINE_FUSE_REG_BIT_ENUM(WRITE_ACCESS_SW_CTRL, 0, READWRITE, READONLY);
    DEFINE_FUSE_REG_BIT_ENUM(WRITE_ACCESS_SW_STATUS, 16, NOWRITE, WRITE);

    DEFINE_FUSE_REG_BIT_ENUM(SECURITY_MODE_SECURITY_MODE, 0, DISABLED, ENABLED);

}
