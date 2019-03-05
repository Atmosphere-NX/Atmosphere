/*
 * Copyright (c) 2018 naehrwert
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
 
#ifndef EXOSPHERE_WARMBOOT_BIN_CLUSTER_H
#define EXOSPHERE_WARMBOOT_BIN_CLUSTER_H

#include <stdint.h>

#include "utils.h"

#define MSELECT_CONFIG_0 MAKE_REG32(0x50060000)

void cluster_initialize_cpu(void);
void cluster_power_on_cpu(void);

#endif
