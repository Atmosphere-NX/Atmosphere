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
#include <stratosphere/os/impl/os_internal_critical_section.hpp>
#include <stratosphere/os/impl/os_internal_condition_variable.hpp>

namespace ams::os {

    struct ThreadType;

    struct ReaderWriterLockType {
        enum State {
            State_NotInitialized = 0,
            State_Initialized    = 1,
        };

        struct LockCount {
            union {
                s32 _arr[sizeof(impl::InternalCriticalSectionStorage) / sizeof(s32)];
                impl::InternalCriticalSectionStorage cs_storage;
                impl::InternalCriticalSectionStorageTypeForConstantInitialize _storage_for_constant_initialize;
            };
            util::BitPack32 counter;
        };
        static_assert(util::is_pod<LockCount>::value);
        static_assert(std::is_trivial<LockCount>::value);

        union {
            struct {
                LockCount c;
                u32 write_lock_count;
            } aligned;
            struct {
                u32 write_lock_count;
                LockCount c;
            } not_aligned;
        } lock_count;

        u32 reserved_1;

        u8 state;
        ThreadType *owner_thread;
        u32 reserved_2;

        union {
            s32 _arr[sizeof(impl::InternalConditionVariableStorage) / sizeof(s32)];
            impl::InternalConditionVariableStorage _storage;
            impl::InternalConditionVariableStorageTypeForConstantInitialize _storage_for_constant_initialize;
        } cv_read_lock;
        union {
            s32 _arr[sizeof(impl::InternalConditionVariableStorage) / sizeof(s32)];
            impl::InternalConditionVariableStorage _storage;
            impl::InternalConditionVariableStorageTypeForConstantInitialize _storage_for_constant_initialize;
        } cv_write_lock;
    };
    static_assert(std::is_trivial<ReaderWriterLockType>::value);

    #if defined(ATMOSPHERE_OS_HORIZON)
    consteval ReaderWriterLockType::LockCount MakeConstantInitializedLockCount() { return {}; }
    #elif defined(ATMOSPHERE_OS_WINDOWS) || defined(ATMOSPHERE_OS_LINUX) || defined(ATMOSPHERE_OS_MACOS)
    /* If windows/linux, require that the lock counter have guaranteed alignment, so that we may constant-initialize. */
    static_assert(alignof(ReaderWriterLockType) == sizeof(u64));
    consteval ReaderWriterLockType::LockCount MakeConstantInitializedLockCount() {
        return ReaderWriterLockType::LockCount {
            { AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CONSTANT_INITIALIZER },
            {},
        };
    }
    #else
        #error "Unknown OS for constant initialized RW-lock LockCount"
    #endif

}
