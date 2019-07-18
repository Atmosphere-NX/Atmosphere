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

static constexpr u32 Module_Spl = 26;

/* Results 1-99 are converted smc results. */
static constexpr Result ResultSplSmcNotImplemented        = MAKERESULT(Module_Spl, 1);
static constexpr Result ResultSplSmcInvalidArgument       = MAKERESULT(Module_Spl, 2);
static constexpr Result ResultSplSmcInProgress            = MAKERESULT(Module_Spl, 3);
static constexpr Result ResultSplSmcNoAsyncOperation      = MAKERESULT(Module_Spl, 4);
static constexpr Result ResultSplSmcInvalidAsyncOperation = MAKERESULT(Module_Spl, 5);
static constexpr Result ResultSplSmcBlacklisted           = MAKERESULT(Module_Spl, 6);

/* Results 100+ are spl results. */
static constexpr Result ResultSplInvalidSize              = MAKERESULT(Module_Spl, 100);
static constexpr Result ResultSplUnknownSmcResult         = MAKERESULT(Module_Spl, 101);
static constexpr Result ResultSplDecryptionFailed         = MAKERESULT(Module_Spl, 102);

static constexpr Result ResultSplOutOfKeyslots            = MAKERESULT(Module_Spl, 104);
static constexpr Result ResultSplInvalidKeyslot           = MAKERESULT(Module_Spl, 105);
static constexpr Result ResultSplBootReasonAlreadySet     = MAKERESULT(Module_Spl, 106);
static constexpr Result ResultSplBootReasonNotSet         = MAKERESULT(Module_Spl, 107);
static constexpr Result ResultSplInvalidArgument          = MAKERESULT(Module_Spl, 108);
