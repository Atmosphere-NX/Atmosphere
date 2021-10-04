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
#include <stratosphere.hpp>
#include "sm_utils.hpp"

namespace ams::sm::impl {

    namespace {

        /* Globals. */
        constinit os::SdkRecursiveMutex g_mitm_ack_session_mutex;
        constinit os::SdkRecursiveMutex g_per_thread_session_mutex;

    }

    /* Utilities. */
    os::SdkRecursiveMutex &GetMitmAcknowledgementSessionMutex() {
        return g_mitm_ack_session_mutex;
    }

    os::SdkRecursiveMutex &GetPerThreadSessionMutex() {
        return g_per_thread_session_mutex;
    }

}
