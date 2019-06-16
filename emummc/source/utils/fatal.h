/*
 * Copyright (c) 2019 m4xw <m4x@m4xw.net>
 * Copyright (c) 2019 Atmosphere-NX
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
#include "../nx/smc.h"

enum FatalReason {
    Fatal_InitMMC = 0,
    Fatal_InitSD,
    Fatal_InvalidAccessor,
    Fatal_ReadNoAccessor,
    Fatal_WriteNoAccessor,
    Fatal_IoMapping,
    Fatal_UnknownVersion,
    Fatal_BadResult,
    Fatal_GetConfig,
    Fatal_Max
};

void __attribute__((noreturn)) fatal_abort(enum FatalReason abortReason);
