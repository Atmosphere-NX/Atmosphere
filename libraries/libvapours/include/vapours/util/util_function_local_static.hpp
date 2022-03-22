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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util/util_typed_storage.hpp>

namespace ams::util {

    #define AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(_TYPE_, _NAME_, ...) static constinit _TYPE_ _NAME_ { __VA_ARGS__ }

    /* NOTE: This must use placement new, to support private constructors. */
    #define AMS_FUNCTION_LOCAL_STATIC_IMPL(_LOCKTYPE_, _SCOPELOCKTYPE_, _TYPE_, _NAME_, ...)                         \
        static constinit ::ams::util::TypedStorage<_TYPE_> s_fls_storage_for_##_NAME_ {};                            \
        static constinit bool s_fls_initialized_##_NAME_ = false;                                                    \
        static constinit _LOCKTYPE_ s_fls_init_lock_##_NAME_ {};                                                     \
        if (AMS_UNLIKELY(!(s_fls_initialized_##_NAME_))) {                                                           \
            _SCOPELOCKTYPE_ sl_fls_for_##_NAME_ { s_fls_init_lock_##_NAME_ };                                        \
            if (AMS_LIKELY(!(s_fls_initialized_##_NAME_))) {                                                         \
                new (::ams::util::impl::GetPointerForConstructAt(s_fls_storage_for_##_NAME_)) _TYPE_( __VA_ARGS__ ); \
                s_fls_initialized_##_NAME_ = true;                                                                   \
            }                                                                                                        \
        }                                                                                                            \
                                                                                                                     \
        _TYPE_ & _NAME_ = util::GetReference(s_fls_storage_for_##_NAME_)

    #if defined(ATMOSPHERE_IS_MESOSPHERE)

        #define AMS_FUNCTION_LOCAL_STATIC(_TYPE_, _NAME_, ...) AMS_FUNCTION_LOCAL_STATIC_IMPL(KSpinLock, KScopedSpinLock, _TYPE_, _NAME_, ##__VA_ARGS__)

    #elif defined(ATMOSPHERE_IS_STRATOSPHERE)

        #define AMS_FUNCTION_LOCAL_STATIC(_TYPE_, _NAME_, ...) AMS_FUNCTION_LOCAL_STATIC_IMPL(os::SdkMutex, std::scoped_lock, _TYPE_, _NAME_, ##__VA_ARGS__)

    #endif

}