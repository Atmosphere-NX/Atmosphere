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

namespace ams::osdbg {

    namespace impl {

        union ThreadTypeCommon;

    }

    enum ThreadTypeType : u8 {
        ThreadTypeType_Unknown = 0,
        ThreadTypeType_Nintendo,
        ThreadTypeType_Stratosphere,
        ThreadTypeType_Libnx,
    };

    struct ThreadInfo {
        s32 _base_priority;
        s32 _current_priority;
        size_t _stack_size;
        uintptr_t _stack;
        uintptr_t _argument;
        uintptr_t _function;
        uintptr_t _name_pointer;
        impl::ThreadTypeCommon *_thread_type;
        os::NativeHandle _debug_handle;
        ThreadTypeType _thread_type_type;
        svc::DebugInfoCreateProcess _debug_info_create_process;
        svc::DebugInfoCreateThread _debug_info_create_thread;
    };

}
