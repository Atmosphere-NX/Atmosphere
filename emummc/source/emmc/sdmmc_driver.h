/*
 * Copyright (c) 2018 naehrwert
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

#ifndef _SDMMC_DRIVER_H_
#define _SDMMC_DRIVER_H_

#include "../utils/types.h"
#include "sdmmc_t210.h"

/*! SDMMC controller IDs. */
#define SDMMC_1 0
#define SDMMC_2 1
#define SDMMC_3 2
#define SDMMC_4 3

/*! SDMMC power types. */
#define SDMMC_POWER_OFF 0
#define SDMMC_POWER_1_8 1
#define SDMMC_POWER_3_3 2

/*! SDMMC bus widths. */
#define SDMMC_BUS_WIDTH_1 0
#define SDMMC_BUS_WIDTH_4 1
#define SDMMC_BUS_WIDTH_8 2

/*! SDMMC response types. */
#define SDMMC_RSP_TYPE_0 0
#define SDMMC_RSP_TYPE_1 1
#define SDMMC_RSP_TYPE_2 2
#define SDMMC_RSP_TYPE_3 3
#define SDMMC_RSP_TYPE_4 4
#define SDMMC_RSP_TYPE_5 5

/*! SDMMC mask interrupt status. */
#define SDMMC_MASKINT_MASKED   0
#define SDMMC_MASKINT_NOERROR -1
#define SDMMC_MASKINT_ERROR   -2

/*! SDMMC host control 2 */
#define SDHCI_CTRL_UHS_MASK			0xFFF8
#define SDHCI_CTRL_VDD_330			0xFFF7
#define SDHCI_CTRL_VDD_180			8
#define SDHCI_CTRL_EXEC_TUNING		0x40
#define SDHCI_CTRL_TUNED_CLK		0x80
#define SDHCI_HOST_VERSION_4_EN		0x1000
#define SDHCI_ADDRESSING_64BIT_EN	0x2000
#define SDHCI_CTRL_PRESET_VAL_EN	0x8000

/*! SD bus speeds. */
#define UHS_SDR12_BUS_SPEED		0
#define HIGH_SPEED_BUS_SPEED	1
#define UHS_SDR25_BUS_SPEED		1
#define UHS_SDR50_BUS_SPEED		2
#define UHS_SDR104_BUS_SPEED	3
#define UHS_DDR50_BUS_SPEED		4
#define HS400_BUS_SPEED 		5

/*! Helper for SWITCH command argument. */
#define SDMMC_SWITCH(mode, index, value) (((mode) << 24) | ((index) << 16) | ((value) << 8))

/*! SDMMC controller context. */
typedef struct _sdmmc_t
{
	t210_sdmmc_t *regs;
	u32 id;
	u32 divisor;
	u32 clock_stopped;
	int no_sd;
	int sd_clock_enabled;
	int venclkctl_set;
	u32 venclkctl_tap;
	u32 expected_rsp_type;
	u64 last_dma_idx;
	u64 dma_addr_next;
	u32 rsp[4];
	u32 rsp3;
} sdmmc_t;

/*! SDMMC command. */
typedef struct _sdmmc_cmd_t
{
	u16 cmd;
	u32 arg;
	u32 rsp_type;
	u32 check_busy;
} sdmmc_cmd_t;

/*! SDMMC request. */
typedef struct _sdmmc_req_t
{
	void *buf;
	u32 blksize;
	u32 num_sectors;
	int is_write;
	int is_multi_block;
	int is_auto_cmd12;
} sdmmc_req_t;

int sdmmc_get_voltage(sdmmc_t *sdmmc);
u32 sdmmc_get_bus_width(sdmmc_t *sdmmc);
void sdmmc_set_bus_width(sdmmc_t *sdmmc, u32 bus_width);
void sdmmc_get_venclkctl(sdmmc_t *sdmmc);
int sdmmc_setup_clock(sdmmc_t *sdmmc, u32 type);
void sdmmc_sd_clock_ctrl(sdmmc_t *sdmmc, int no_sd);
int sdmmc_get_rsp(sdmmc_t *sdmmc, u32 *rsp, u32 size, u32 type);
int sdmmc_config_tuning(sdmmc_t *sdmmc, u32 type, u32 cmd);
int sdmmc_stop_transmission(sdmmc_t *sdmmc, u32 *rsp);
int sdmmc_init(sdmmc_t *sdmmc, u32 id, u32 power, u32 bus_width, u32 type, int no_sd);
void sdmmc_end(sdmmc_t *sdmmc);
void sdmmc_init_cmd(sdmmc_cmd_t *cmdbuf, u16 cmd, u32 arg, u32 rsp_type, u32 check_busy);
int sdmmc_execute_cmd(sdmmc_t *sdmmc, sdmmc_cmd_t *cmd, sdmmc_req_t *req, u32 *blkcnt_out);
int sdmmc_enable_low_voltage(sdmmc_t *sdmmc);

#endif
