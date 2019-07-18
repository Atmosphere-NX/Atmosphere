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

static constexpr u32 Module_Ro = 22;

static constexpr Result ResultRoInsufficientAddressSpace = MAKERESULT(Module_Ro, 2);
static constexpr Result ResultRoAlreadyLoaded            = MAKERESULT(Module_Ro, 3);
static constexpr Result ResultRoInvalidNro               = MAKERESULT(Module_Ro, 4);

static constexpr Result ResultRoInvalidNrr               = MAKERESULT(Module_Ro, 6);
static constexpr Result ResultRoTooManyNro               = MAKERESULT(Module_Ro, 7);
static constexpr Result ResultRoTooManyNrr               = MAKERESULT(Module_Ro, 8);
static constexpr Result ResultRoNotAuthorized            = MAKERESULT(Module_Ro, 9);
static constexpr Result ResultRoInvalidNrrType           = MAKERESULT(Module_Ro, 10);

static constexpr Result ResultRoInternalError            = MAKERESULT(Module_Ro, 1023);

static constexpr Result ResultRoInvalidAddress           = MAKERESULT(Module_Ro, 1025);
static constexpr Result ResultRoInvalidSize              = MAKERESULT(Module_Ro, 1026);

static constexpr Result ResultRoNotLoaded                = MAKERESULT(Module_Ro, 1028);
static constexpr Result ResultRoNotRegistered            = MAKERESULT(Module_Ro, 1029);
static constexpr Result ResultRoInvalidSession           = MAKERESULT(Module_Ro, 1030);
static constexpr Result ResultRoInvalidProcess           = MAKERESULT(Module_Ro, 1031);
