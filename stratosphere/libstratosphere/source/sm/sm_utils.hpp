/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/sm.hpp>

#include "sm_ams.h"

namespace sts::sm::impl {

    /* Utilities. */
    HosRecursiveMutex &GetUserSessionMutex();
    HosRecursiveMutex &GetMitmSessionMutex();

    template<typename F>
    Result DoWithUserSession(F f) {
        std::scoped_lock<HosRecursiveMutex &> lk(GetUserSessionMutex());
        {
            R_ASSERT(smInitialize());
            ON_SCOPE_EXIT { smExit(); };

            return f();
        }
    }

    template<typename F>
    Result DoWithMitmSession(F f) {
        std::scoped_lock<HosRecursiveMutex &> lk(GetMitmSessionMutex());
        {
            R_ASSERT(smAtmosphereMitmInitialize());
            ON_SCOPE_EXIT { smAtmosphereMitmExit(); };

            return f();
        }
    }

}
