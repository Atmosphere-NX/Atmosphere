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

static constexpr u32 Module_Loader = 9;

static constexpr Result ResultLoaderTooLongArgument       = MAKERESULT(Module_Loader, 1);
static constexpr Result ResultLoaderTooManyArguments      = MAKERESULT(Module_Loader, 2);
static constexpr Result ResultLoaderTooLargeMeta          = MAKERESULT(Module_Loader, 3);
static constexpr Result ResultLoaderInvalidMeta           = MAKERESULT(Module_Loader, 4);
static constexpr Result ResultLoaderInvalidNso            = MAKERESULT(Module_Loader, 5);
static constexpr Result ResultLoaderInvalidPath           = MAKERESULT(Module_Loader, 6);
static constexpr Result ResultLoaderTooManyProcesses      = MAKERESULT(Module_Loader, 7);
static constexpr Result ResultLoaderNotPinned             = MAKERESULT(Module_Loader, 8);
static constexpr Result ResultLoaderInvalidProgramId      = MAKERESULT(Module_Loader, 9);
static constexpr Result ResultLoaderInvalidVersion        = MAKERESULT(Module_Loader, 10);

static constexpr Result ResultLoaderInsufficientAddressSpace     = MAKERESULT(Module_Loader, 51);
static constexpr Result ResultLoaderInvalidNro                   = MAKERESULT(Module_Loader, 52);
static constexpr Result ResultLoaderInvalidNrr                   = MAKERESULT(Module_Loader, 53);
static constexpr Result ResultLoaderInvalidSignature             = MAKERESULT(Module_Loader, 54);
static constexpr Result ResultLoaderInsufficientNroRegistrations = MAKERESULT(Module_Loader, 55);
static constexpr Result ResultLoaderInsufficientNrrRegistrations = MAKERESULT(Module_Loader, 56);
static constexpr Result ResultLoaderNroAlreadyLoaded             = MAKERESULT(Module_Loader, 57);

static constexpr Result ResultLoaderInvalidAddress        = MAKERESULT(Module_Loader, 81);
static constexpr Result ResultLoaderInvalidSize           = MAKERESULT(Module_Loader, 82);
static constexpr Result ResultLoaderNotLoaded             = MAKERESULT(Module_Loader, 84);
static constexpr Result ResultLoaderNotRegistered         = MAKERESULT(Module_Loader, 85);
static constexpr Result ResultLoaderInvalidSession        = MAKERESULT(Module_Loader, 86);
static constexpr Result ResultLoaderInvalidProcess        = MAKERESULT(Module_Loader, 87);

static constexpr Result ResultLoaderUnknownCapability                = MAKERESULT(Module_Loader, 100);
static constexpr Result ResultLoaderInvalidCapabilityKernelFlags     = MAKERESULT(Module_Loader, 103);
static constexpr Result ResultLoaderInvalidCapabilitySyscallMask     = MAKERESULT(Module_Loader, 104);
static constexpr Result ResultLoaderInvalidCapabilityMapRange        = MAKERESULT(Module_Loader, 106);
static constexpr Result ResultLoaderInvalidCapabilityMapPage         = MAKERESULT(Module_Loader, 107);
static constexpr Result ResultLoaderInvalidCapabilityInterruptPair   = MAKERESULT(Module_Loader, 111);
static constexpr Result ResultLoaderInvalidCapabilityApplicationType = MAKERESULT(Module_Loader, 113);
static constexpr Result ResultLoaderInvalidCapabilityKernelVersion   = MAKERESULT(Module_Loader, 114);
static constexpr Result ResultLoaderInvalidCapabilityHandleTable     = MAKERESULT(Module_Loader, 115);
static constexpr Result ResultLoaderInvalidCapabilityDebugFlags      = MAKERESULT(Module_Loader, 116);

static constexpr Result ResultLoaderInternalError = MAKERESULT(Module_Loader, 200);
