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
 
#ifndef FUSEE_BCT0_H
#define FUSEE_BCT0_H

#include <stdint.h>
#include <stdbool.h>

#include "lib/log.h"

#define BCTO_MAX_SIZE 0x5800

typedef struct {
	/* [config] */
	ScreenLogLevel log_level;

	/* [stage1] */
	char stage2_path[0x100];
	char stage2_mtc_path[0x100];
	uintptr_t stage2_load_address;
	uintptr_t stage2_entrypoint;
} bct0_t;

int bct0_parse(const char *ini, bct0_t *out);

#endif
