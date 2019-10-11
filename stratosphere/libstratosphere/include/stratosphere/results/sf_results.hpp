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

static constexpr u32 Module_ServiceFramework = 10;

static constexpr Result ResultServiceFrameworkNotSupported          = MAKERESULT(Module_ServiceFramework, 1);
static constexpr Result ResultServiceFrameworkPreconditionViolation = MAKERESULT(Module_ServiceFramework, 3);

static constexpr Result ResultServiceFrameworkInvalidCmifHeaderSize = MAKERESULT(Module_ServiceFramework, 202);
static constexpr Result ResultServiceFrameworkInvalidCmifInHeader   = MAKERESULT(Module_ServiceFramework, 211);
static constexpr Result ResultServiceFrameworkUnknownCmifCommandId  = MAKERESULT(Module_ServiceFramework, 221);
static constexpr Result ResultServiceFrameworkInvalidCmifOutRawSize = MAKERESULT(Module_ServiceFramework, 232);

static constexpr Result ResultServiceFrameworkTargetNotFound        = MAKERESULT(Module_ServiceFramework, 261);

static constexpr Result ResultServiceFrameworkOutOfDomainEntries    = MAKERESULT(Module_ServiceFramework, 301);


static constexpr Result ResultServiceFrameworkRequestDeferred       = MAKERESULT(Module_ServiceFramework, 811);
static constexpr Result ResultServiceFrameworkRequestDeferredByUser = MAKERESULT(Module_ServiceFramework, 812);
