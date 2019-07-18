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

static constexpr u32 Module_Dmnt = 13;

static constexpr Result ResultDmntUnknown           = MAKERESULT(Module_Dmnt, 1);
static constexpr Result ResultDmntDebuggingDisabled = MAKERESULT(Module_Dmnt, 2);

static constexpr Result ResultDmntCheatNotAttached               = MAKERESULT(Module_Dmnt, 6500);
static constexpr Result ResultDmntCheatNullBuffer                = MAKERESULT(Module_Dmnt, 6501);
static constexpr Result ResultDmntCheatInvalidBuffer             = MAKERESULT(Module_Dmnt, 6502);
static constexpr Result ResultDmntCheatUnknownChtId              = MAKERESULT(Module_Dmnt, 6503);
static constexpr Result ResultDmntCheatOutOfCheats               = MAKERESULT(Module_Dmnt, 6504);
static constexpr Result ResultDmntCheatInvalidCheat              = MAKERESULT(Module_Dmnt, 6505);
static constexpr Result ResultDmntCheatCannotDisableMasterCheat  = MAKERESULT(Module_Dmnt, 6505);

static constexpr Result ResultDmntCheatInvalidFreezeWidth     = MAKERESULT(Module_Dmnt, 6600);
static constexpr Result ResultDmntCheatAddressAlreadyFrozen   = MAKERESULT(Module_Dmnt, 6601);
static constexpr Result ResultDmntCheatAddressNotFrozen       = MAKERESULT(Module_Dmnt, 6602);
static constexpr Result ResultDmntCheatTooManyFrozenAddresses = MAKERESULT(Module_Dmnt, 6603);

static constexpr Result ResultDmntCheatVmInvalidCondDepth = MAKERESULT(Module_Dmnt, 6700);