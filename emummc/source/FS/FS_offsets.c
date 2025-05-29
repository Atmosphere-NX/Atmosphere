/*
 * Copyright (c) 2019 m4xw <m4x@m4xw.net>
 * Copyright (c) 2019 Atmosphere-NX
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

#include "FS_offsets.h"
#include "offsets/100.h"
#include "offsets/200.h"
#include "offsets/200_exfat.h"
#include "offsets/210.h"
#include "offsets/210_exfat.h"
#include "offsets/300.h"
#include "offsets/300_exfat.h"
#include "offsets/301.h"
#include "offsets/301_exfat.h"
#include "offsets/400.h"
#include "offsets/400_exfat.h"
#include "offsets/410.h"
#include "offsets/410_exfat.h"
#include "offsets/500.h"
#include "offsets/500_exfat.h"
#include "offsets/510.h"
#include "offsets/510_exfat.h"
#include "offsets/600.h"
#include "offsets/600_exfat.h"
#include "offsets/700.h"
#include "offsets/700_exfat.h"
#include "offsets/800.h"
#include "offsets/800_exfat.h"
#include "offsets/810.h"
#include "offsets/810_exfat.h"
#include "offsets/900.h"
#include "offsets/900_exfat.h"
#include "offsets/910.h"
#include "offsets/910_exfat.h"
#include "offsets/1000.h"
#include "offsets/1000_exfat.h"
#include "offsets/1020.h"
#include "offsets/1020_exfat.h"
#include "offsets/1100.h"
#include "offsets/1100_exfat.h"
#include "offsets/1200.h"
#include "offsets/1200_exfat.h"
#include "offsets/1203.h"
#include "offsets/1203_exfat.h"
#include "offsets/1300.h"
#include "offsets/1300_exfat.h"
#include "offsets/1310.h"
#include "offsets/1310_exfat.h"
#include "offsets/1400.h"
#include "offsets/1400_exfat.h"
#include "offsets/1500.h"
#include "offsets/1500_exfat.h"
#include "offsets/1600.h"
#include "offsets/1600_exfat.h"
#include "offsets/1603.h"
#include "offsets/1603_exfat.h"
#include "offsets/1700.h"
#include "offsets/1700_exfat.h"
#include "offsets/1800.h"
#include "offsets/1800_exfat.h"
#include "offsets/1810.h"
#include "offsets/1810_exfat.h"
#include "offsets/1900.h"
#include "offsets/1900_exfat.h"
#include "offsets/2000.h"
#include "offsets/2000_exfat.h"
#include "offsets/2010.h"
#include "offsets/2010_exfat.h"
#include "../utils/fatal.h"

#define GET_OFFSET_STRUCT_NAME(vers) g_offsets##vers

#define DEFINE_OFFSET_STRUCT(vers) \
static const fs_offsets_t GET_OFFSET_STRUCT_NAME(vers) = { \
    .sdmmc_accessor_gc               = FS_OFFSET##vers##_SDMMC_ACCESSOR_GC, \
    .sdmmc_accessor_sd               = FS_OFFSET##vers##_SDMMC_ACCESSOR_SD, \
    .sdmmc_accessor_nand             = FS_OFFSET##vers##_SDMMC_ACCESSOR_NAND, \
    .sdmmc_wrapper_read              = FS_OFFSET##vers##_SDMMC_WRAPPER_READ, \
    .sdmmc_wrapper_write             = FS_OFFSET##vers##_SDMMC_WRAPPER_WRITE, \
    .clkrst_set_min_v_clock_rate     = FS_OFFSET##vers##_CLKRST_SET_MIN_V_CLK_RATE, \
    .rtld                            = FS_OFFSET##vers##_RTLD, \
    .rtld_destination                = FS_OFFSET##vers##_RTLD_DESTINATION, \
    .lock_mutex                      = FS_OFFSET##vers##_LOCK_MUTEX, \
    .unlock_mutex                    = FS_OFFSET##vers##_UNLOCK_MUTEX, \
    .sd_mutex                        = FS_OFFSET##vers##_SD_MUTEX, \
    .nand_mutex                      = FS_OFFSET##vers##_NAND_MUTEX, \
    .active_partition                = FS_OFFSET##vers##_ACTIVE_PARTITION, \
    .sdmmc_das_handle                = FS_OFFSET##vers##_SDMMC_DAS_HANDLE, \
    .sdmmc_accessor_controller_open  = FS_OFFSET##vers##_SDMMC_WRAPPER_CONTROLLER_OPEN, \
    .sdmmc_accessor_controller_close = FS_OFFSET##vers##_SDMMC_WRAPPER_CONTROLLER_CLOSE, \
    .sd_das_init                     = FS_OFFSET##vers##_SD_DAS_INIT, \
    .nintendo_paths                  = FS_OFFSET##vers##_NINTENDO_PATHS, \
}

// Actually define offset structs
DEFINE_OFFSET_STRUCT(_100);
DEFINE_OFFSET_STRUCT(_200);
DEFINE_OFFSET_STRUCT(_200_EXFAT);
DEFINE_OFFSET_STRUCT(_210);
DEFINE_OFFSET_STRUCT(_210_EXFAT);
DEFINE_OFFSET_STRUCT(_300);
DEFINE_OFFSET_STRUCT(_300_EXFAT);
DEFINE_OFFSET_STRUCT(_301);
DEFINE_OFFSET_STRUCT(_301_EXFAT);
DEFINE_OFFSET_STRUCT(_400);
DEFINE_OFFSET_STRUCT(_400_EXFAT);
DEFINE_OFFSET_STRUCT(_410);
DEFINE_OFFSET_STRUCT(_410_EXFAT);
DEFINE_OFFSET_STRUCT(_500);
DEFINE_OFFSET_STRUCT(_500_EXFAT);
DEFINE_OFFSET_STRUCT(_510);
DEFINE_OFFSET_STRUCT(_510_EXFAT);
DEFINE_OFFSET_STRUCT(_600);
DEFINE_OFFSET_STRUCT(_600_EXFAT);
DEFINE_OFFSET_STRUCT(_700);
DEFINE_OFFSET_STRUCT(_700_EXFAT);
DEFINE_OFFSET_STRUCT(_800);
DEFINE_OFFSET_STRUCT(_800_EXFAT);
DEFINE_OFFSET_STRUCT(_810);
DEFINE_OFFSET_STRUCT(_810_EXFAT);
DEFINE_OFFSET_STRUCT(_900);
DEFINE_OFFSET_STRUCT(_900_EXFAT);
DEFINE_OFFSET_STRUCT(_910);
DEFINE_OFFSET_STRUCT(_910_EXFAT);
DEFINE_OFFSET_STRUCT(_1000);
DEFINE_OFFSET_STRUCT(_1000_EXFAT);
DEFINE_OFFSET_STRUCT(_1020);
DEFINE_OFFSET_STRUCT(_1020_EXFAT);
DEFINE_OFFSET_STRUCT(_1100);
DEFINE_OFFSET_STRUCT(_1100_EXFAT);
DEFINE_OFFSET_STRUCT(_1200);
DEFINE_OFFSET_STRUCT(_1200_EXFAT);
DEFINE_OFFSET_STRUCT(_1203);
DEFINE_OFFSET_STRUCT(_1203_EXFAT);
DEFINE_OFFSET_STRUCT(_1300);
DEFINE_OFFSET_STRUCT(_1300_EXFAT);
DEFINE_OFFSET_STRUCT(_1310);
DEFINE_OFFSET_STRUCT(_1310_EXFAT);
DEFINE_OFFSET_STRUCT(_1400);
DEFINE_OFFSET_STRUCT(_1400_EXFAT);
DEFINE_OFFSET_STRUCT(_1500);
DEFINE_OFFSET_STRUCT(_1500_EXFAT);
DEFINE_OFFSET_STRUCT(_1600);
DEFINE_OFFSET_STRUCT(_1600_EXFAT);
DEFINE_OFFSET_STRUCT(_1603);
DEFINE_OFFSET_STRUCT(_1603_EXFAT);
DEFINE_OFFSET_STRUCT(_1700);
DEFINE_OFFSET_STRUCT(_1700_EXFAT);
DEFINE_OFFSET_STRUCT(_1800);
DEFINE_OFFSET_STRUCT(_1800_EXFAT);
DEFINE_OFFSET_STRUCT(_1810);
DEFINE_OFFSET_STRUCT(_1810_EXFAT);
DEFINE_OFFSET_STRUCT(_1900);
DEFINE_OFFSET_STRUCT(_1900_EXFAT);
DEFINE_OFFSET_STRUCT(_2000);
DEFINE_OFFSET_STRUCT(_2000_EXFAT);
DEFINE_OFFSET_STRUCT(_2010);
DEFINE_OFFSET_STRUCT(_2010_EXFAT);

const fs_offsets_t *get_fs_offsets(enum FS_VER version) {
    switch (version) {
        case FS_VER_1_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_100));
        case FS_VER_2_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_200));
        case FS_VER_2_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_200_EXFAT));
        case FS_VER_2_1_0:
            return &(GET_OFFSET_STRUCT_NAME(_210));
        case FS_VER_2_1_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_210_EXFAT));
        case FS_VER_3_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_300));
        case FS_VER_3_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_300_EXFAT));
        case FS_VER_3_0_1:
            return &(GET_OFFSET_STRUCT_NAME(_301));
        case FS_VER_3_0_1_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_301_EXFAT));
        case FS_VER_4_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_400));
        case FS_VER_4_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_400_EXFAT));
        case FS_VER_4_1_0:
            return &(GET_OFFSET_STRUCT_NAME(_410));
        case FS_VER_4_1_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_410_EXFAT));
        case FS_VER_5_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_500));
        case FS_VER_5_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_500_EXFAT));
        case FS_VER_5_1_0:
            return &(GET_OFFSET_STRUCT_NAME(_510));
        case FS_VER_5_1_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_510_EXFAT));
        case FS_VER_6_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_600));
        case FS_VER_6_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_600_EXFAT));
        case FS_VER_7_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_700));
        case FS_VER_7_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_700_EXFAT));
        case FS_VER_8_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_800));
        case FS_VER_8_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_800_EXFAT));
        case FS_VER_8_1_0:
            return &(GET_OFFSET_STRUCT_NAME(_810));
        case FS_VER_8_1_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_810_EXFAT));
        case FS_VER_9_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_900));
        case FS_VER_9_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_900_EXFAT));
        case FS_VER_9_1_0:
            return &(GET_OFFSET_STRUCT_NAME(_910));
        case FS_VER_9_1_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_910_EXFAT));
        case FS_VER_10_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_1000));
        case FS_VER_10_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_1000_EXFAT));
        case FS_VER_10_2_0:
            return &(GET_OFFSET_STRUCT_NAME(_1020));
        case FS_VER_10_2_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_1020_EXFAT));
        case FS_VER_11_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_1100));
        case FS_VER_11_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_1100_EXFAT));
        case FS_VER_12_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_1200));
        case FS_VER_12_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_1200_EXFAT));
        case FS_VER_12_0_3:
            return &(GET_OFFSET_STRUCT_NAME(_1203));
        case FS_VER_12_0_3_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_1203_EXFAT));
        case FS_VER_13_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_1300));
        case FS_VER_13_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_1300_EXFAT));
        case FS_VER_13_1_0:
            return &(GET_OFFSET_STRUCT_NAME(_1310));
        case FS_VER_13_1_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_1310_EXFAT));
        case FS_VER_14_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_1400));
        case FS_VER_14_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_1400_EXFAT));
        case FS_VER_15_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_1500));
        case FS_VER_15_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_1500_EXFAT));
        case FS_VER_16_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_1600));
        case FS_VER_16_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_1600_EXFAT));
        case FS_VER_16_0_3:
            return &(GET_OFFSET_STRUCT_NAME(_1603));
        case FS_VER_16_0_3_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_1603_EXFAT));
        case FS_VER_17_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_1700));
        case FS_VER_17_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_1700_EXFAT));
        case FS_VER_18_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_1800));
        case FS_VER_18_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_1800_EXFAT));
        case FS_VER_18_1_0:
            return &(GET_OFFSET_STRUCT_NAME(_1810));
        case FS_VER_18_1_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_1810_EXFAT));
        case FS_VER_19_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_1900));
        case FS_VER_19_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_1900_EXFAT));
        case FS_VER_20_0_0:
            return &(GET_OFFSET_STRUCT_NAME(_2000));
        case FS_VER_20_0_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_2000_EXFAT));
        case FS_VER_20_1_0:
            return &(GET_OFFSET_STRUCT_NAME(_2010));
        case FS_VER_20_1_0_EXFAT:
            return &(GET_OFFSET_STRUCT_NAME(_2010_EXFAT));
        default:
            fatal_abort(Fatal_UnknownVersion);
    }
}