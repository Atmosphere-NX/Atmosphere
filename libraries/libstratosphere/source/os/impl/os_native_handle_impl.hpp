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
#include <stratosphere.hpp>

#if defined(ATMOSPHERE_OS_HORIZON)
    #include "os_native_handle_impl.os.horizon.hpp"
#elif defined(ATMOSPHERE_OS_WINDOWS)
    #include "os_native_handle_impl.os.windows.hpp"
#elif defined(ATMOSPHERE_OS_LINUX)
    #include "os_native_handle_impl.os.linux.hpp"
#elif defined(ATMOSPHERE_OS_MACOS)
    #include "os_native_handle_impl.os.macos.hpp"
#else
    #error "Unknown OS for ams::os::NativeHandleImpl"
#endif