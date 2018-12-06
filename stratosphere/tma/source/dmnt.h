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
 
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u64 handle;
} DmntFile;

typedef enum {
    DmntTIOCreateOption_CreateNew = 1,
    DmntTIOCreateOption_CreateAlways = 2,
    DmntTIOCreateOption_OpenExisting = 3,
    DmntTIOCreateOption_OpenAlways = 4,
    DmntTIOCreateOption_ResetSize = 5,
} DmntTIOCreateOption;

Result dmntInitialize(void);
void dmntExit(void);

Result dmntTargetIOFileOpen(DmntFile *out, const char *path, int flags, DmntTIOCreateOption create_option);
Result dmntTargetIOFileClose(DmntFile *f);
Result dmntTargetIOFileRead(DmntFile *f, u64 off, void* buf, size_t len, size_t* out_read);
Result dmntTargetIOFileWrite(DmntFile *f, u64 off, const void* buf, size_t len, size_t* out_written);

#ifdef __cplusplus
}
#endif