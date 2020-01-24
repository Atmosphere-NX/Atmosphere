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
 
#include <stdint.h>

#include "mc.h"
#include "utils.h"

void disable_bpmp_access_to_dram(void) {
    /* Modify carveout 4 to prevent BPMP access to dram (TZ will fix it). */
    volatile security_carveout_t *carveout = (volatile security_carveout_t *)(MC_BASE + 0xC08 + 0x50 * (4 - CARVEOUT_ID_MIN));
    carveout->paddr_low = 0;
    carveout->paddr_high = 0;
    carveout->size_big_pages = 1; /* 128 KiB */
    carveout->client_access_0 = 0;
    carveout->client_access_1 = 0;
    carveout->client_access_2 = 0;
    carveout->client_access_3 = 0;
    carveout->client_access_4 = 0;
    carveout->client_force_internal_access_0 = BIT(CSR_AVPCARM7R);
    carveout->client_force_internal_access_1 = BIT(CSW_AVPCARM7W);
    carveout->client_force_internal_access_2 = 0;
    carveout->client_force_internal_access_3 = 0;
    carveout->client_force_internal_access_4 = 0;
    /* Set config to LOCKED, TZ-SECURE, untranslated addresses only. */
    carveout->config = 0x8F;
}
