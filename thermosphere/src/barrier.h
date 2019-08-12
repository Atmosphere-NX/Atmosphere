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

#include <stdatomic.h>
#include "types.h"

typedef struct Barrier {
    atomic_uint val;
} Barrier;

void barrierInit(Barrier *barrier, u32 coreList);
void barrierInitAllButSelf(Barrier *barrier);
void barrierInitAll(Barrier *barrier);

void barrierWait(Barrier *barrier);