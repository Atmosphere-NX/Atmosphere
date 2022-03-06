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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

/* Collides with StandardUserSystemClock. */
#ifdef GetCurrentTime

ALWAYS_INLINE auto WindowsGetCurrentTime() {
    return GetCurrentTime();
}

#undef GetCurrentTime

#endif

/* Collides with several things. */
#ifdef FillMemory

ALWAYS_INLINE auto WindowsFillMemory(PVOID p, SIZE_T l, BYTE v) {
    return FillMemory(p, l, v);
}

#undef FillMemory

/* Should be provided by winerror.h, but isn't. */
#if !defined(ERROR_SPACES_NOT_ENOUGH_DRIVES)
#define ERROR_SPACES_NOT_ENOUGH_DRIVES ((DWORD)0x80E7000Bu)
#endif

#endif
