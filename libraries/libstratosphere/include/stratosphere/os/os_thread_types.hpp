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
#include <stratosphere/os/os_thread_common.hpp>
#include <stratosphere/os/os_thread_local_storage_common.hpp>
#include <stratosphere/os/os_thread_local_storage_api.hpp>
#include <stratosphere/os/impl/os_internal_critical_section.hpp>
#include <stratosphere/os/impl/os_internal_condition_variable.hpp>
#include <stratosphere/os/os_sdk_thread_types.hpp>

namespace ams::os {

    namespace impl {

        class MultiWaitObjectList;

    }

    #if !defined(AMS_OS_IMPL_USE_PTHREADS)
    using ThreadId = u64;
    #else
        /* TODO: decide whether using pthread_id_np_t or not more thoroughly. */
        #if defined(ATMOSPHERE_OS_MACOS)
            #define AMS_OS_IMPL_USE_PTHREADID_NP_FOR_THREAD_ID
        #endif

        #if defined(AMS_OS_IMPL_USE_PTHREADID_NP_FOR_THREAD_ID)
            using ThreadId = u64;
        #else
            static_assert(sizeof(pthread_t) <= sizeof(u64));
            using ThreadId = pthread_t;
        #endif
    #endif

    struct ThreadType {
        static constexpr u16 Magic = 0xF5A5;

        enum State {
            State_NotInitialized         = 0,
            State_Initialized            = 1,
            State_DestroyedBeforeStarted = 2,
            State_Started                = 3,
            State_Terminated             = 4,
        };

        util::TypedStorage<util::IntrusiveListNode> all_threads_node;
        util::TypedStorage<impl::MultiWaitObjectList, sizeof(util::IntrusiveListNode), alignof(util::IntrusiveListNode)> waitlist;
        uintptr_t reserved[4];
        u8 state;
        bool stack_is_aliased;
        bool auto_registered;
        u8 suspend_count;
        u16 magic;
        s16 base_priority;
        u16 version;
        char name_buffer[ThreadNameLengthMax];
        const char *name_pointer;
        ThreadId thread_id;
        void *original_stack;
        void *stack;
        size_t stack_size;
        ThreadFunction function;
        void *initial_fiber;
        void *current_fiber;
        void *argument;

        mutable impl::InternalCriticalSectionStorage cs_thread;
        mutable impl::InternalConditionVariableStorage cv_thread;

        /* The following members are arch/os specific. */
        #if defined(AMS_OS_IMPL_USE_PTHREADS)

        mutable uintptr_t tls_value_array[TlsSlotCountMax + SdkTlsSlotCountMax];

        mutable impl::InternalCriticalSectionStorage cs_pthread_exit;
        mutable impl::InternalConditionVariableStorage cv_pthread_exit;
        bool exited_pthread;

        pthread_t pthread;
        u64 affinity_mask;
        int ideal_core;

        #elif defined(ATMOSPHERE_OS_HORIZON)
        /* NOTE: Here, Nintendo stores the TLS array. This is handled by libnx in our case. */
        /* However, we need to access certain values in other threads' TLS (Nintendo uses a hardcoded layout for SDK tls members...) */
        /* These members are tls slot holders in sdk code, but just normal thread type members under our scheme. */
        SdkInternalTlsType sdk_internal_tls;

        using ThreadImpl = ::Thread;

        ThreadImpl *thread_impl;
        ThreadImpl thread_impl_storage;
        #elif defined(ATMOSPHERE_OS_WINDOWS)

        mutable uintptr_t tls_value_array[TlsSlotCountMax + SdkTlsSlotCountMax];

        NativeHandle native_handle;
        int ideal_core;
        u64 affinity_mask;

        #endif
    };
    static_assert(std::is_trivial<ThreadType>::value);

    constexpr inline s32 IdealCoreDontCare   = -1;
    constexpr inline s32 IdealCoreUseDefault = -2;
    constexpr inline s32 IdealCoreNoUpdate   = -3;

}
