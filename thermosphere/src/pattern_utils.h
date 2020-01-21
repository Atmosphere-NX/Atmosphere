/*
 * Copyright (c) 2019 Atmosphère-NX
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

#include "utils.h"

// u32 size to limit stack usage
// Not sure if we need this function because we can only map one guest page at a time...
u8 *memsearch(u8 *startPos, const void *pattern, u32 size, u32 patternSize);
unsigned int xstrtoui(const char *nptr, char **endptr, int base, bool allowPrefix, bool *ok);
unsigned long int xstrtoul(const char *nptr, char **endptr, int base, bool allowPrefix, bool *ok);
unsigned long long int xstrtoull(const char *nptr, char **endptr, int base, bool allowPrefix, bool *ok);
