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
#include <vapours.hpp>

namespace ams::os {

    #if defined(ATMOSPHERE_OS_HORIZON)

        using NativeHandle = svc::Handle;
        static_assert(std::unsigned_integral<NativeHandle>);

        constexpr inline NativeHandle InvalidNativeHandle = svc::InvalidHandle;

    #elif defined(ATMOSPHERE_OS_WINDOWS)

        using NativeHandle = void *;

        constexpr inline NativeHandle InvalidNativeHandle = nullptr;

    #elif defined(ATMOSPHERE_OS_LINUX) || defined(ATMOSPHERE_OS_MACOS)

        using NativeHandle = s32;

        constexpr inline NativeHandle InvalidNativeHandle = -1;

    #else
        #error "Unknown OS for os::NativeHandle"
    #endif

}
