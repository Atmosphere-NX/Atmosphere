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

static constexpr u32 Module_Ncm = 5;

static constexpr Result ResultNcmPlaceHolderAlreadyExists                = MAKERESULT(Module_Ncm, 2);
static constexpr Result ResultNcmPlaceHolderNotFound                     = MAKERESULT(Module_Ncm, 3);
static constexpr Result ResultNcmContentAlreadyExists                    = MAKERESULT(Module_Ncm, 4);
static constexpr Result ResultNcmContentNotFound                         = MAKERESULT(Module_Ncm, 5);
static constexpr Result ResultNcmContentMetaNotFound                     = MAKERESULT(Module_Ncm, 7);
static constexpr Result ResultNcmAllocationFailed                        = MAKERESULT(Module_Ncm, 8);
static constexpr Result ResultNcmUnknownStorage                          = MAKERESULT(Module_Ncm, 12);

static constexpr Result ResultNcmInvalidContentStorage                   = MAKERESULT(Module_Ncm, 100);
static constexpr Result ResultNcmInvalidContentMetaDatabase              = MAKERESULT(Module_Ncm, 110);

static constexpr Result ResultNcmBufferInsufficient                      = MAKERESULT(Module_Ncm, 180);
static constexpr Result ResultNcmInvalidContentMetaKey                   = MAKERESULT(Module_Ncm, 240);

static constexpr Result ResultNcmContentStorageNotActive                 = MAKERESULT(Module_Ncm, 250);
static constexpr Result ResultNcmGameCardContentStorageNotActive         = MAKERESULT(Module_Ncm, 251);
static constexpr Result ResultNcmNandSystemContentStorageNotActive       = MAKERESULT(Module_Ncm, 252);
static constexpr Result ResultNcmNandUserContentStorageNotActive         = MAKERESULT(Module_Ncm, 253);
static constexpr Result ResultNcmSdCardContentStorageNotActive           = MAKERESULT(Module_Ncm, 254);
static constexpr Result ResultNcmUnknownContentStorageNotActive          = MAKERESULT(Module_Ncm, 258);

static constexpr Result ResultNcmContentMetaDatabaseNotActive            = MAKERESULT(Module_Ncm, 260);
static constexpr Result ResultNcmGameCardContentMetaDatabaseNotActive    = MAKERESULT(Module_Ncm, 261);
static constexpr Result ResultNcmNandSystemContentMetaDatabaseNotActive  = MAKERESULT(Module_Ncm, 262);
static constexpr Result ResultNcmNandUserContentMetaDatabaseNotActive    = MAKERESULT(Module_Ncm, 263);
static constexpr Result ResultNcmSdCardContentMetaDatabaseNotActive      = MAKERESULT(Module_Ncm, 264);
static constexpr Result ResultNcmUnknownContentMetaDatabaseNotActive     = MAKERESULT(Module_Ncm, 268);

static constexpr Result ResultNcmInvalidArgument        = MAKERESULT(Module_Ncm, 8181);
static constexpr Result ResultNcmInvalidOffset          = MAKERESULT(Module_Ncm, 8182);
