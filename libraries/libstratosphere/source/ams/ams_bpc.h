/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

#ifdef __cplusplus
extern "C" {
#endif

Result amsBpcInitialize(void);
void amsBpcExit(void);
Service *amsBpcGetServiceSession(void);

Result amsBpcRebootToFatalError(void *ctx);
Result amsBpcSetRebootPayload(const void *src, size_t src_size);

#ifdef __cplusplus
}
#endif