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
#include <stratosphere.hpp>
#include "sm_ams.h"

namespace ams::sm::impl {

    /* Utilities. */
    os::Mutex &GetUserSessionMutex();
    os::Mutex &GetMitmAcknowledgementSessionMutex();
    os::Mutex &GetPerThreadSessionMutex();

    template<typename F>
    Result DoWithUserSession(F f) {
        std::scoped_lock lk(GetUserSessionMutex());
        {
            R_ABORT_UNLESS(smInitialize());
            ON_SCOPE_EXIT {
                R_ABORT_UNLESS(smDetachClient());
                smExit();
            };

            return f();
        }
    }

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
        Service srv;
        {
            std::scoped_lock lk(GetPerThreadSessionMutex());
            R_ABORT_UNLESS(smAtmosphereOpenSession(&srv));
        }
        {
            ON_SCOPE_EXIT { smAtmosphereCloseSession(&srv); };
            return f(&srv);
        }
    }

    NX_CONSTEXPR SmServiceName ConvertName(sm::ServiceName name) {
        static_assert(sizeof(SmServiceName) == sizeof(sm::ServiceName));
        SmServiceName ret = {};
        __builtin_memcpy(&ret, &name, sizeof(sm::ServiceName));
        return ret;
    }

}
