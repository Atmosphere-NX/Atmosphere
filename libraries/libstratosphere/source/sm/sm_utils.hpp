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
#include "sm_ams.h"

namespace ams::sm::impl {

    /* Utilities. */
    os::SdkRecursiveMutex &GetMitmAcknowledgementSessionMutex();
    os::SdkRecursiveMutex &GetPerThreadSessionMutex();

    template<typename F>
    Result DoWithMitmAcknowledgementSession(F f) {
        std::scoped_lock lk(GetMitmAcknowledgementSessionMutex());
        {
            R_ABORT_UNLESS(smAtmosphereMitmInitialize());
            ON_SCOPE_EXIT { smAtmosphereMitmExit(); };

            return f();
        }
    }

    template<typename F>
    Result DoWithPerThreadSession(F f) {
        TipcService srv;
        {
            std::scoped_lock lk(GetPerThreadSessionMutex());
            R_ABORT_UNLESS(smAtmosphereOpenSession(std::addressof(srv)));
        }
        {
            ON_SCOPE_EXIT { smAtmosphereCloseSession(std::addressof(srv)); };
            return f(std::addressof(srv));
        }
    }

    constexpr ALWAYS_INLINE SmServiceName ConvertName(sm::ServiceName name) {
        static_assert(sizeof(SmServiceName) == sizeof(sm::ServiceName));
        return std::bit_cast<SmServiceName>(name);
    }

}
