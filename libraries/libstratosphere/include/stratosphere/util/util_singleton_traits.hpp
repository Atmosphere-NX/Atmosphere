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
#include <stratosphere/os/os_sdk_mutex.hpp>

#define AMS_SINGLETON_TRAITS(_CLASSNAME_)                                                \
    private:                                                                             \
        NON_COPYABLE(_CLASSNAME_);                                                       \
        NON_MOVEABLE(_CLASSNAME_);                                                       \
    private:                                                                             \
        _CLASSNAME_ ();                                                                  \
    public:                                                                              \
        static _CLASSNAME_ &GetInstance() {                                              \
            /* Declare singleton instance variables. */                                  \
            static constinit ::ams::util::TypedStorage<_CLASSNAME_> s_singleton_storage; \
            static constinit ::ams::os::SdkMutex s_singleton_mutex;                      \
            static constinit bool s_initialized_singleton = false;                       \
                                                                                         \
            /* Ensure the instance is created. */                                        \
            if (AMS_UNLIKELY(!s_initialized_singleton)) {                                \
                std::scoped_lock lk(s_singleton_mutex);                                  \
                                                                                         \
                if (AMS_LIKELY(!s_initialized_singleton)) {                              \
                    new (::ams::util::GetPointer(s_singleton_storage)) _CLASSNAME_;      \
                    s_initialized_singleton = true;                                      \
                }                                                                        \
            }                                                                            \
                                                                                         \
            return ::ams::util::GetReference(s_singleton_storage);                       \
        }
