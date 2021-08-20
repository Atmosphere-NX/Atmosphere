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

#ifndef FUSEE_KEYDERIVATION_H
#define FUSEE_KEYDERIVATION_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum BisPartition {
    BisPartition_Calibration = 0,
    BisPartition_Safe = 1,
    BisPartition_UserSystem = 2,
} BisPartition;

int derive_nx_keydata_erista(uint32_t target_firmware);
int derive_nx_keydata_mariko(uint32_t target_firmware);
void derive_bis_key(void *dst, BisPartition partition_id, uint32_t target_firmware);

#endif
