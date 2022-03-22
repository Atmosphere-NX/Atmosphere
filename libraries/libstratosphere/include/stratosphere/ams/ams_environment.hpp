/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere/ams/ams_types.hpp>

namespace ams {

    /* Will be called by libstratosphere on crash. */
    void CrashHandler(ThreadExceptionDump *ctx);

    /* API for boot sysmodule. */
    void InitializeForBoot();
    void SetInitialRebootPayload(const void *src, size_t src_size);

    void *Malloc(size_t size);
    void Free(void *ptr);

    void *MallocForRapidJson(size_t size);
    void *ReallocForRapidJson(void *ptr, size_t size);
    void FreeForRapidJson(void *ptr);

}
