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
 
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "smc_api.h"
#include "smc_ams.h"

uint32_t ams_iram_copy(smc_args_t *args) {
    /* TODO: Implement a DRAM <-> IRAM copy of up to one page here. */
    /* This operation is necessary to implement reboot-to-payload. */
    /* args->X[1] = DRAM address (translated by kernel). */
    /* args->X[2] = IRAM address. */
    /* args->X[3] = size (must be <= 0x1000). */
    /* args->X[4] = 0 for read, 1 for write. */
    return 2;
}
