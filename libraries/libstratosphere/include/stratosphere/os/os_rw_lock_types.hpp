/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

    struct ReadWriteLockType {
        enum State {
            State_NotInitialized = 0,
            State_Initialized    = 1,
        };

        struct LockCount {
            union {
                s32 _arr[sizeof(impl::InternalCriticalSectionStorage) / sizeof(s32)];
                impl::InternalCriticalSectionStorage cs_storage;
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
        } cv_read_lock;
        union {
            s32 _arr[sizeof(impl::InternalConditionVariableStorage) / sizeof(s32)];
            impl::InternalConditionVariableStorage _storage;
        } cv_write_lock;
    };
    static_assert(std::is_trivial<ReadWriteLockType>::value);

}
