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

static constexpr u32 Module_Settings = 105;

static constexpr Result ResultSettingsItemNotFound                       = MAKERESULT(Module_Settings, 11);

static constexpr Result ResultSettingsItemKeyAllocationFailed            = MAKERESULT(Module_Settings, 101);
static constexpr Result ResultSettingsItemValueAllocationFailed          = MAKERESULT(Module_Settings, 102);

static constexpr Result ResultSettingsItemNameNull                       = MAKERESULT(Module_Settings, 201);
static constexpr Result ResultSettingsItemKeyNull                        = MAKERESULT(Module_Settings, 202);
static constexpr Result ResultSettingsItemValueNull                      = MAKERESULT(Module_Settings, 203);
static constexpr Result ResultSettingsItemKeyBufferNull                  = MAKERESULT(Module_Settings, 204);
static constexpr Result ResultSettingsItemValueBufferNull                = MAKERESULT(Module_Settings, 205);

static constexpr Result ResultSettingsItemNameEmpty                      = MAKERESULT(Module_Settings, 221);
static constexpr Result ResultSettingsItemKeyEmpty                       = MAKERESULT(Module_Settings, 222);

static constexpr Result ResultSettingsItemNameTooLong                    = MAKERESULT(Module_Settings, 241);
static constexpr Result ResultSettingsItemKeyTooLong                     = MAKERESULT(Module_Settings, 242);

static constexpr Result ResultSettingsItemNameInvalidFormat              = MAKERESULT(Module_Settings, 261);
static constexpr Result ResultSettingsItemKeyInvalidFormat               = MAKERESULT(Module_Settings, 262);
static constexpr Result ResultSettingsItemValueInvalidFormat             = MAKERESULT(Module_Settings, 263);
