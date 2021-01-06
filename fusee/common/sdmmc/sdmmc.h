/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 CTCaer
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
 
#ifndef FUSEE_SDMMC_H
#define FUSEE_SDMMC_H

#include "sdmmc_core.h"

/* Structure for storing the MMC CID (adapted from Linux headers) */
typedef struct {
    uint32_t    manfid;
    uint8_t     prod_name[8];
    uint8_t     prv;
    uint32_t    serial;
    uint16_t    oemid;
    uint16_t    year;
    uint8_t     hwrev;
    uint8_t     fwrev;
    uint8_t     month;
} mmc_cid_t;

/* Structure for storing the MMC CSD (adapted from Linux headers) */
typedef struct {
    uint8_t     structure;
    uint8_t     mmca_vsn;
    uint16_t    cmdclass;
    uint16_t    taac_clks;
    uint32_t    taac_ns;
    uint32_t    c_size;
    uint32_t    r2w_factor;
    uint32_t    max_dtr;
    uint32_t    erase_size;     /* In sectors */
    uint32_t    read_blkbits;
    uint32_t    write_blkbits;
    uint32_t    capacity;
    uint32_t    read_partial:1,
                read_misalign:1,
                write_partial:1,
                write_misalign:1,
                dsr_imp:1;
} mmc_csd_t;

/* Structure for storing the MMC extended CSD (adapted from Linux headers) */
typedef struct {
    uint8_t     rev;
    uint8_t     erase_group_def;
    uint8_t     sec_feature_support;
    uint8_t     rel_sectors;
    uint8_t     rel_param;
    uint8_t     part_config;
    uint8_t     cache_ctrl;
    uint8_t     rst_n_function;
    uint8_t     max_packed_writes;
    uint8_t     max_packed_reads;
    uint8_t     packed_event_en;
    uint32_t    part_time;                      /* Units: ms */
    uint32_t    sa_timeout;                     /* Units: 100ns */
    uint32_t    generic_cmd6_time;              /* Units: 10ms */
    uint32_t    power_off_longtime;             /* Units: ms */
    uint8_t     power_off_notification;         /* state */
    uint32_t    hs_max_dtr;
    uint32_t    hs200_max_dtr;
    uint32_t    sectors;
    uint32_t    hc_erase_size;                  /* In sectors */
    uint32_t    hc_erase_timeout;               /* In milliseconds */
    uint32_t    sec_trim_mult;                  /* Secure trim multiplier  */
    uint32_t    sec_erase_mult;                 /* Secure erase multiplier */
    uint32_t    trim_timeout;                   /* In milliseconds */
    uint32_t    partition_setting_completed;    /* enable bit */
    uint64_t    enhanced_area_offset;           /* Units: Byte */
    uint32_t    enhanced_area_size;             /* Units: KB */
    uint32_t    cache_size;                     /* Units: KB */
    uint32_t    hpi_en;                         /* HPI enablebit */
    uint32_t    hpi;                            /* HPI support bit */
    uint32_t    hpi_cmd;                        /* cmd used as HPI */
    uint32_t    bkops;                          /* background support bit */
    uint32_t    man_bkops_en;                   /* manual bkops enable bit */
    uint32_t    auto_bkops_en;                  /* auto bkops enable bit */
    uint32_t    data_sector_size;               /* 512 bytes or 4KB */
    uint32_t    data_tag_unit_size;             /* DATA TAG UNIT size */
    uint32_t    boot_ro_lock;                   /* ro lock support */
    uint32_t    boot_ro_lockable;
    uint32_t    ffu_capable;                    /* Firmware upgrade support */
    uint32_t    cmdq_en;                        /* Command Queue enabled */
    uint32_t    cmdq_support;                   /* Command Queue supported */
    uint32_t    cmdq_depth;                     /* Command Queue depth */
    uint8_t     fwrev[8];                       /* FW version */
    uint8_t     raw_exception_status;           /* 54 */
    uint8_t     raw_partition_support;          /* 160 */
    uint8_t     raw_rpmb_size_mult;             /* 168 */
    uint8_t     raw_erased_mem_count;           /* 181 */
    uint8_t     strobe_support;                 /* 184 */
    uint8_t     raw_ext_csd_structure;          /* 194 */
    uint8_t     raw_card_type;                  /* 196 */
    uint8_t     raw_driver_strength;            /* 197 */
    uint8_t     out_of_int_time;                /* 198 */
    uint8_t     raw_pwr_cl_52_195;              /* 200 */
    uint8_t     raw_pwr_cl_26_195;              /* 201 */
    uint8_t     raw_pwr_cl_52_360;              /* 202 */
    uint8_t     raw_pwr_cl_26_360;              /* 203 */
    uint8_t     raw_s_a_timeout;                /* 217 */
    uint8_t     raw_hc_erase_gap_size;          /* 221 */
    uint8_t     raw_erase_timeout_mult;         /* 223 */
    uint8_t     raw_hc_erase_grp_size;          /* 224 */
    uint8_t     raw_sec_trim_mult;              /* 229 */
    uint8_t     raw_sec_erase_mult;             /* 230 */
    uint8_t     raw_sec_feature_support;        /* 231 */
    uint8_t     raw_trim_mult;                  /* 232 */
    uint8_t     raw_pwr_cl_200_195;             /* 236 */
    uint8_t     raw_pwr_cl_200_360;             /* 237 */
    uint8_t     raw_pwr_cl_ddr_52_195;          /* 238 */
    uint8_t     raw_pwr_cl_ddr_52_360;          /* 239 */
    uint8_t     raw_pwr_cl_ddr_200_360;         /* 253 */
    uint8_t     raw_bkops_status;               /* 246 */
    uint8_t     raw_sectors[4];                 /* 212 - 4 bytes */
    uint8_t     pre_eol_info;                   /* 267 */
    uint8_t     device_life_time_est_typ_a;     /* 268 */
    uint8_t     device_life_time_est_typ_b;     /* 269 */
    uint32_t    feature_support;
} mmc_ext_csd_t;

/* Structure for storing the SD SCR (adapted from Linux headers) */
typedef struct  {
    uint8_t     sda_vsn;
    uint8_t     sda_spec3;
    uint8_t     bus_widths;
    uint8_t     cmds;
} sd_scr_t;

/* Structure for storing the SD SSR (adapted from Linux headers) */
typedef struct  {
    uint8_t     dat_bus_width;
    uint8_t     secured_mode;
    uint16_t    sd_card_type;
    uint8_t     speed_class;
    uint8_t     uhs_speed_grade;
    uint8_t     uhs_au_size;
    uint8_t     video_speed_class;
    uint8_t     app_perf_class;
} sd_ssr_t;

/* Structure describing a SDMMC device's context. */
typedef struct {
    /* Underlying driver context. */
    sdmmc_t *sdmmc;
    
    bool is_180v;
    bool is_block_sdhc;
    uint32_t rca;
    mmc_cid_t cid;
    mmc_csd_t csd;
    mmc_ext_csd_t ext_csd;
    sd_scr_t scr;
    sd_ssr_t ssr;
} sdmmc_device_t;

int sdmmc_device_sd_init(sdmmc_device_t *device, sdmmc_t *sdmmc, SdmmcBusWidth bus_width, SdmmcBusSpeed bus_speed);
int sdmmc_device_mmc_init(sdmmc_device_t *device, sdmmc_t *sdmmc, SdmmcBusWidth bus_width, SdmmcBusSpeed bus_speed);
int sdmmc_device_read(sdmmc_device_t *device, uint32_t sector, uint32_t num_sectors, void *data);
int sdmmc_device_write(sdmmc_device_t *device, uint32_t sector, uint32_t num_sectors, void *data);
int sdmmc_device_finish(sdmmc_device_t *device);
int sdmmc_mmc_select_partition(sdmmc_device_t *device, SdmmcPartitionNum partition);

#endif