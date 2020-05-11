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
#include <stratosphere.hpp>
#include "sm_utils.hpp"

namespace ams::sm::impl {

    namespace {

        /* Globals. */
        os::Mutex g_user_session_mutex(true);
        os::Mutex g_mitm_ack_session_mutex(true);
        os::Mutex g_per_thread_session_mutex(true);

    }

    /* Utilities. */
    os::Mutex &GetUserSessionMutex() {
        return g_user_session_mutex;
    }

    os::Mutex &GetMitmAcknowledgementSessionMutex() {
        return g_mitm_ack_session_mutex;
    }

    os::Mutex &GetPerThreadSessionMutex() {
        return g_per_thread_session_mutex;
    }

}
