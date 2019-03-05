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
 
#include <switch.h>

#include "ldr_random.hpp"

/* Official HOS uses TinyMT. This is high effort. Let's just use XorShift. */
/* https://en.wikipedia.org/wiki/Xorshift */

static u32 g_random_state[4] = {0};
static bool g_has_initialized = false;

static void EnsureRandomState() {
    if (g_has_initialized) {
        return;
    }
    
    /* Retrieve process entropy with svcGetInfo. */
    u64 val = 0;
    for (unsigned int i = 0; i < 4; i++) {
        if (R_FAILED(svcGetInfo(&val, 0xB, 0, i))) {
            /* TODO: Panic? */
        }
        g_random_state[i] = val & 0xFFFFFFFF;
    }
    
    g_has_initialized = true;
}

u32 RandomUtils::GetNext() {
    EnsureRandomState();
    u32 s, t = g_random_state[3];
    t ^= t << 11;
    t ^= t >> 8;
    g_random_state[3] = g_random_state[2]; g_random_state[2] = g_random_state[1]; g_random_state[1] = (s = g_random_state[0]);
    t ^= s;
    t ^= s >> 19;
    g_random_state[0] = t;
    return t;
}

/* These are slightly biased, but I think that's totally okay. */
u32 RandomUtils::GetRandomU32(u32 max) {
    return GetNext() % max;
}

u64 RandomUtils::GetRandomU64(u64 max) {
    u64 val = GetNext();
    val |= ((u64)GetNext()) << 32;
    return val % max;
}
