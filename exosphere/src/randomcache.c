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
#include <string.h>

#include "utils.h"
#include "randomcache.h"
#include "se.h"
#include "arm.h"

/* TrustZone maintains a cache of random for the kernel. */
/* So that requests can still be serviced even when a */
/* usermode SMC is in progress. */

static uint8_t g_random_cache[0x400];
static unsigned int g_random_cache_low = 0;
static unsigned int g_random_cache_high = 0x3FF;


void randomcache_refill_segment(unsigned int offset, unsigned int size) {
    if (offset + size >= 0x400) {
        size = 0x400 - offset;
    }

    flush_dcache_range(&g_random_cache[offset], &g_random_cache[offset + size]);
    se_generate_random(KEYSLOT_SWITCH_RNGKEY, &g_random_cache[offset], size);
    flush_dcache_range(&g_random_cache[offset], &g_random_cache[offset + size]);

}

void randomcache_init(void) {
    randomcache_refill_segment(0, 0x400);
    g_random_cache_low = 0;
    g_random_cache_high = 0x3FF;
}

void randomcache_refill(void) {
    unsigned int high_plus_one = (g_random_cache_high + 1) & 0x3FF;
    if (g_random_cache_low != high_plus_one) {
        /* Only refill if there's data to refill. */
        if (g_random_cache_low < high_plus_one) {
            /* NOTE: There is a bug in official code that causes this to not work properly. */
            /* In particular, official code checks whether high_plus_one == 0x400. */
            /* However, because high_plus_one is &= 0x3FF'd, this can never be the case. */
            /* We will implement according to Nintendo's intention, and not include their bug. */
            /* This should have no impact on actual observable results, anyway, since this data is random anyway... */
            
            if (g_random_cache_high != 0x3FF) { /* This is if (true) in Nintendo's code due to the above bug. */
                randomcache_refill_segment(high_plus_one, 0x400 - high_plus_one);
                g_random_cache_high = (g_random_cache_high + 0x400 - high_plus_one) & 0x3FF;
            }
            
            if (g_random_cache_low > 0) {
                randomcache_refill_segment(0, g_random_cache_low);
                g_random_cache_high = (g_random_cache_high + g_random_cache_low) & 0x3FF;
            }
        } else { /* g_random_cache_low > high_plus_one */
            randomcache_refill_segment(high_plus_one, g_random_cache_low - high_plus_one);
            g_random_cache_high = g_random_cache_low - 1;
        }
    }
}

void randomcache_getbytes(void *dst, size_t num_bytes) {
    unsigned int low = g_random_cache_low;
    if (num_bytes == 0) {
        return;
    }
    memcpy(dst, &g_random_cache[low], num_bytes);
    
    unsigned int new_low = low + num_bytes;
    if (new_low + 0x38 > 0x3FF) {
        new_low = 0;
    }

    g_random_cache_low = new_low;
}
