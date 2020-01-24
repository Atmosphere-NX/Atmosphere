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
 
#ifndef EXOSPHERE_USERPAGE_H
#define EXOSPHERE_USERPAGE_H

#include "utils.h"
#include "memory_map.h"

static inline uintptr_t get_user_page_secure_monitor_addr(void) {
    return TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGMENT_ID_USERPAGE);
}

#define USER_PAGE_SECURE_MONITOR_ADDR (get_user_page_secure_monitor_addr())

typedef struct {
    uintptr_t user_address;
    uintptr_t secure_monitor_address;
} upage_ref_t;

bool upage_init(upage_ref_t *user_page, void *user_address);

bool user_copy_to_secure(upage_ref_t *user_page, void *secure_dst, const void *user_src, size_t size);
bool secure_copy_to_user(upage_ref_t *user_page, void *user_dst, const void *secure_src, size_t size);

#endif
