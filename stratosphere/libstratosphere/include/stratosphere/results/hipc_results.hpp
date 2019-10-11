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

static constexpr u32 Module_Hipc = 11;

static constexpr Result ResultHipcSessionAllocationFailure = MAKERESULT(Module_Hipc, 102);

static constexpr Result ResultHipcOutOfSessions        = MAKERESULT(Module_Hipc, 131);
static constexpr Result ResultHipcPointerBufferTooSmall = MAKERESULT(Module_Hipc, 141);

static constexpr Result ResultHipcOutOfDomains         = MAKERESULT(Module_Hipc, 200);

static constexpr Result ResultHipcSessionClosed        = MAKERESULT(Module_Hipc, 301);

static constexpr Result ResultHipcInvalidRequestSize   = MAKERESULT(Module_Hipc, 402);
static constexpr Result ResultHipcUnknownCommandType   = MAKERESULT(Module_Hipc, 403);

static constexpr Result ResultHipcInvalidRequest       = MAKERESULT(Module_Hipc, 420);

static constexpr Result ResultHipcTargetNotDomain      = MAKERESULT(Module_Hipc, 491);
static constexpr Result ResultHipcDomainObjectNotFound = MAKERESULT(Module_Hipc, 492);
