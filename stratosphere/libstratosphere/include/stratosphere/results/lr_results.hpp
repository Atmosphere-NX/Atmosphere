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

static constexpr u32 Module_Lr = 8;

static constexpr Result ResultLrProgramNotFound                          = MAKERESULT(Module_Lr, 2);
static constexpr Result ResultLrDataNotFound                             = MAKERESULT(Module_Lr, 3);
static constexpr Result ResultLrUnknownStorageId                         = MAKERESULT(Module_Lr, 4);
static constexpr Result ResultLrHtmlDocumentNotFound                     = MAKERESULT(Module_Lr, 6);
static constexpr Result ResultLrAddOnContentNotFound                     = MAKERESULT(Module_Lr, 7);
static constexpr Result ResultLrControlNotFound                          = MAKERESULT(Module_Lr, 8);
static constexpr Result ResultLrLegalInformationNotFound                 = MAKERESULT(Module_Lr, 9);

static constexpr Result ResultLrTooManyRegisteredPaths = MAKERESULT(Module_Lr, 90);
