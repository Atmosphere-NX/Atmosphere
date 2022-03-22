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

namespace ams::os::impl {

    class InterProcessEventImpl {
        public:
            static Result Create(NativeHandle *out_write, NativeHandle *out_read);
            static void Close(NativeHandle handle);
            static void Signal(NativeHandle handle);
            static void Clear(NativeHandle handle);
            static void Wait(NativeHandle handle, bool auto_clear);
            static bool TryWait(NativeHandle handle, bool auto_clear);
            static bool TimedWait(NativeHandle handle, bool auto_clear, TimeSpan timeout);
    };

}