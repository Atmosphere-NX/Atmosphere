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

    struct ThreadLocalRegionLp64 {
        u32 message_buffer[0x100 / sizeof(u32)];
        volatile u16 disable_counter;
        volatile u16 interrupt_flag;
        u32 reserved0;
        u64 reserved[15];
        u64 tls[10];
        u64 locale_ptr;
        s64 _errno_val;
        u64 thread_data;
        u64 eh_globals;
        u64 thread_pointer;
        u64 p_thread_type;
    };
    static_assert(sizeof(ThreadLocalRegionLp64) == sizeof(svc::ThreadLocalRegion));
    static_assert(AMS_OFFSETOF(ThreadLocalRegionLp64, tls) == 0x180);


    struct ThreadLocalRegionIlp32 {
        u32 message_buffer[0x100 / sizeof(u32)];
        volatile u16 disable_counter;
        volatile u16 interrupt_flag;
        u32 reserved[(0xC0 - 0x4) / sizeof(u32)];
        u32 tls[10];
        u32 locale_ptr;
        s32 _errno_val;
        u32 thread_data;
        u32 eh_globals;
        u32 thread_pointer;
        u32 p_thread_type;
    };
    static_assert(sizeof(ThreadLocalRegionIlp32) == sizeof(svc::ThreadLocalRegion));
    static_assert(AMS_OFFSETOF(ThreadLocalRegionIlp32, tls) == 0x1C0);

    struct LibnxThreadVars {
        static constexpr u32 Magic = util::ReverseFourCC<'!','T','V','$'>::Code;

        u32 magic;
        ::Handle handle;
        ::Thread *thread_ptr;
        void *reent;
        void *tls_tp;
    };
    static_assert(sizeof(LibnxThreadVars) == 0x20);

    struct ThreadLocalRegionLibnx {
        u32 message_buffer[0x100 / sizeof(u32)];
        volatile u16 disable_counter;
        volatile u16 interrupt_flag;
        u32 reserved0;
        u64 tls[(0x200 - sizeof(LibnxThreadVars) - 0x108) / sizeof(u64)];
        LibnxThreadVars thread_vars;
    };
    static_assert(sizeof(ThreadLocalRegionLibnx) == sizeof(svc::ThreadLocalRegion));
    static_assert(AMS_OFFSETOF(ThreadLocalRegionLibnx, thread_vars) == 0x200 - sizeof(LibnxThreadVars));

    struct LibnxThreadEntryArgs {
        u64 t;
        u64 entry;
        u64 arg;
        u64 reent;
        u64 tls;
        u64 padding;
    };

    union ThreadLocalRegionCommon {
        ThreadLocalRegionIlp32 ilp32;
        ThreadLocalRegionLp64 lp64;
        ThreadLocalRegionLibnx libnx;
    };
    static_assert(sizeof(ThreadLocalRegionCommon) == sizeof(svc::ThreadLocalRegion));

    inline ThreadLocalRegionCommon *GetTargetThreadLocalRegion(ThreadInfo *info) {
        return reinterpret_cast<ThreadLocalRegionCommon *>(info->_debug_info_create_thread.tls_address);
    }

    void GetTargetThreadType(ThreadInfo *info);

}
