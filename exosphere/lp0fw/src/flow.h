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
 
#ifndef EXOSPHERE_WARMBOOT_BIN_FLOW_CTLR_H
#define EXOSPHERE_WARMBOOT_BIN_FLOW_CTLR_H

#include <stdint.h>
#include <stdbool.h>

#include "utils.h"

#define FLOW_BASE (0x60007000)

#define MAKE_FLOW_REG(ofs) MAKE_REG32(FLOW_BASE + ofs)

#define FLOW_CTLR_HALT_COP_EVENTS_0      MAKE_FLOW_REG(0x004)
#define FLOW_CTLR_RAM_REPAIR_0           MAKE_FLOW_REG(0x040)
#define FLOW_CTLR_BPMP_CLUSTER_CONTROL_0 MAKE_FLOW_REG(0x098)

void flow_perform_ram_repair(void);

#endif
