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

static constexpr u32 Module_Fatal = 163;

static constexpr Result ResultFatalAllocationFailed                    = MAKERESULT(Module_Fatal, 1);
static constexpr Result ResultFatalNullGraphicsBuffer                  = MAKERESULT(Module_Fatal, 2);
static constexpr Result ResultFatalAlreadyThrown                       = MAKERESULT(Module_Fatal, 3);
static constexpr Result ResultFatalTooManyEvents                       = MAKERESULT(Module_Fatal, 4);
static constexpr Result ResultFatalInRepairWithoutVolHeld              = MAKERESULT(Module_Fatal, 5);
static constexpr Result ResultFatalInRepairWithoutTimeReviserCartridge = MAKERESULT(Module_Fatal, 6);
