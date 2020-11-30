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

#ifndef FUSEE_CHAINLOADER_H
#define FUSEE_CHAINLOADER_H

#include <stddef.h>
#include <stdint.h>

#define CHAINLOADER_ARG_DATA_MAX_SIZE 0x5400
#define CHAINLOADER_MAX_ENTRIES       128

typedef struct chainloader_entry_t {
    uintptr_t load_address;
    uintptr_t src_address;
    size_t size;
    size_t num;
} chainloader_entry_t;

extern int g_chainloader_argc;
extern chainloader_entry_t g_chainloader_entries[CHAINLOADER_MAX_ENTRIES]; /* keep them sorted */
extern size_t g_chainloader_num_entries;
extern uintptr_t g_chainloader_entrypoint;

extern char g_chainloader_arg_data[CHAINLOADER_ARG_DATA_MAX_SIZE];

void relocate_and_chainload(void);

#endif
