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

#define AMS_SINGLETON_TRAITS(_CLASSNAME_)                                 \
    private:                                                              \
        NON_COPYABLE(_CLASSNAME_);                                        \
        NON_MOVEABLE(_CLASSNAME_);                                        \
    private:                                                              \
        _CLASSNAME_ ();                                                   \
    public:                                                               \
        static _CLASSNAME_ &GetInstance() {                               \
            AMS_FUNCTION_LOCAL_STATIC(_CLASSNAME_, s_singleton_instance); \
                                                                          \
            return s_singleton_instance;                                  \
        }

#define AMS_CONSTINIT_SINGLETON_TRAITS(_CLASSNAME_)                                 \
    private:                                                                        \
        NON_COPYABLE(_CLASSNAME_);                                                  \
        NON_MOVEABLE(_CLASSNAME_);                                                  \
    private:                                                                        \
        constexpr _CLASSNAME_ () = default;                                         \
    public:                                                                         \
        static _CLASSNAME_ &GetInstance() {                                         \
            /* Declare singleton instance variables. */                             \
            AMS_FUNCTION_LOCAL_STATIC_CONSTINIT(_CLASSNAME_, s_singleton_instance); \
            return s_singleton_instance;                                            \
        }
