/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

static constexpr u32 Module_Kvdb = 20;

static constexpr Result ResultKvdbKeyCapacityInsufficient = MAKERESULT(Module_Kvdb, 1);
static constexpr Result ResultKvdbKeyNotFound             = MAKERESULT(Module_Kvdb, 2);
static constexpr Result ResultKvdbAllocationFailed        = MAKERESULT(Module_Kvdb, 4);
static constexpr Result ResultKvdbInvalidKeyValue         = MAKERESULT(Module_Kvdb, 5);
static constexpr Result ResultKvdbBufferInsufficient      = MAKERESULT(Module_Kvdb, 6);

static constexpr Result ResultKvdbInvalidFilesystemState  = MAKERESULT(Module_Kvdb, 8);
static constexpr Result ResultKvdbNotCreated              = MAKERESULT(Module_Kvdb, 9);