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
#include "osdbg_types.hpp"

namespace ams::osdbg::impl {

    /* Check that our values are the same as Nintendo's. */
    static_assert(os::TlsSlotCountMax == 16);
    static_assert(os::SdkTlsSlotCountMax == 16);
    static_assert(os::ThreadNameLengthMax == 32);

    struct ThreadTypeIlp32 {
        AlignedStorageIlp32<0, 2, alignof(u32)> _all_threads_node;
        AlignedStorageIlp32<0, 2, alignof(u32)> _multi_wait_object_list;
        u32 _padding[4];
        u8 _state;
        bool _stack_is_aliased;
        bool _auto_registered;
        u8 _suspend_count;
        s16 _base_priority;
        u16 _version;
        u32 _original_stack;
        u32 _stack;
        u32 _stack_size;
        u32 _argument;
        u32 _thread_function;
        u32 _current_fiber;
        u32 _initial_fiber;
        u32 _tls_value_array[os::TlsSlotCountMax + os::SdkTlsSlotCountMax];
        char _name_buffer[os::ThreadNameLengthMax];
        u32 _name_pointer;
        AlignedStorageIlp32<4, 0, alignof(u32)> _cs_thread;
        AlignedStorageIlp32<4, 0, alignof(u32)> _cv_thread;
        AlignedStorageIlp32<4, 0, alignof(u32)> _handle;
        u32 _lock_history;
        u32 _thread_id_low;
        u32 _thread_id_high;
    };
    static_assert(sizeof(ThreadTypeIlp32) == 0x100);

    struct ThreadTypeIlp32Version0 {
        AlignedStorageIlp32<0, 2, alignof(u32)> _all_threads_node;
        AlignedStorageIlp32<0, 2, alignof(u32)> _multi_wait_object_list;
        u32 _padding[4];
        u8 _state;
        bool _stack_is_aliased;
        bool _auto_registered;
        u8 _padding1;
        s32 _base_priority;
        u32 _original_stack;
        u32 _stack;
        u32 _stack_size;
        u32 _argument;
        u32 _thread_function;
        u32 _current_fiber;
        u32 _initial_fiber;
        u32 _lock_history;
        u32 _tls_value_array[os::TlsSlotCountMax + os::SdkTlsSlotCountMax];
        char _name_buffer[os::ThreadNameLengthMax];
        u32 _name_pointer;
        AlignedStorageIlp32<4, 0, alignof(u32)> _cs_thread;
        AlignedStorageIlp32<4, 0, alignof(u32)> _cv_thread;
        AlignedStorageIlp32<4, 0, alignof(u32)> _handle;
        char _padding2[8];
    };
    static_assert(sizeof(ThreadTypeIlp32Version0) == 0x100);

    struct ThreadTypeLp64 {
        AlignedStorageLp64<0, 2, alignof(u64)> _all_threads_node;
        AlignedStorageLp64<0, 2, alignof(u64)> _multi_wait_object_list;
        u64 _padding[4];
        u8 _state;
        bool _stack_is_aliased;
        bool _auto_registered;
        u8 _suspend_count;
        s16 _base_priority;
        u16 _version;
        u64 _original_stack;
        u64 _stack;
        u64 _stack_size;
        u64 _argument;
        u64 _thread_function;
        u64 _current_fiber;
        u64 _initial_fiber;
        u64 _tls_value_array[os::TlsSlotCountMax + os::SdkTlsSlotCountMax];
        char _name_buffer[os::ThreadNameLengthMax];
        u64 _name_pointer;
        AlignedStorageLp64<4, 0, alignof(u32)> _cs_thread;
        AlignedStorageLp64<4, 0, alignof(u32)> _cv_thread;
        AlignedStorageLp64<4, 0, alignof(u32)> _handle;
        u32 _lock_history;
        u64 thread_id;
    };
    static_assert(sizeof(ThreadTypeLp64) == 0x1C0);

    struct ThreadTypeLp64Version0 {
        AlignedStorageLp64<0, 2, alignof(u64)> _all_threads_node;
        AlignedStorageLp64<0, 2, alignof(u64)> _multi_wait_object_list;
        u64 _padding[4];
        u8 _state;
        bool _stack_is_aliased;
        bool _auto_registered;
        u8 _suspend_count;
        s16 _base_priority;
        u16 _version;
        u64 _original_stack;
        u64 _stack;
        u64 _stack_size;
        u64 _argument;
        u64 _thread_function;
        u64 _current_fiber;
        u64 _initial_fiber;
        u32 _lock_history;
        u32 _padding2;
        u64 _tls_value_array[os::TlsSlotCountMax + os::SdkTlsSlotCountMax];
        char _name_buffer[os::ThreadNameLengthMax];
        u64 _name_pointer;
        AlignedStorageLp64<4, 0, alignof(u32)> _cs_thread;
        AlignedStorageLp64<4, 0, alignof(u32)> _cv_thread;
        AlignedStorageLp64<4, 0, alignof(u32)> _handle;
        u32 _padding3;
    };
    static_assert(sizeof(ThreadTypeLp64Version0) == 0x1C0);

    union ThreadTypeCommon {
        ThreadTypeIlp32 ilp32;
        ThreadTypeLp64 lp64;
        ThreadTypeIlp32Version0 ilp32_v0;
        ThreadTypeLp64Version0 lp64_v0;
        os::ThreadType stratosphere;
        ::Thread libnx;
    };

}
