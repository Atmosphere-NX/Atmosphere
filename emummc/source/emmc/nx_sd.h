/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2019 CTCaer
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

#ifndef NX_SD_H
#define NX_SD_H

#include "../utils/types.h"

enum
{
	SD_INIT_FAIL  = 0,
	SD_1BIT_HS25  = 1,
	SD_4BIT_HS25  = 2,
	SD_UHS_SDR82  = 3,
	SD_UHS_SDR104 = 4
};

u32  nx_sd_get_mode();
int  nx_sd_init_retry(bool power_cycle);
bool nx_sd_initialize(bool power_cycle);

#endif