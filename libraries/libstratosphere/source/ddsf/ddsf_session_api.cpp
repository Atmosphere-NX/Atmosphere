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
#include <stratosphere.hpp>

namespace ams::ddsf {

    Result OpenSession(IDevice *device, ISession *session, AccessMode access_mode) {
        /* Check pre-conditions. */
        AMS_ASSERT(device != nullptr);
        AMS_ASSERT(session != nullptr);
        AMS_ASSERT(!session->IsOpen());

        /* Attack the session to the device. */
        session->AttachDevice(device, access_mode);
        auto session_guard = SCOPE_GUARD { session->DetachDevice(); };

        /* Attach the device to the session. */
        R_TRY(device->AttachSession(session));

        /* We succeeded. */
        session_guard.Cancel();
        return ResultSuccess();
    }

    void CloseSession(ISession *session) {
        /* Check pre-conditions. */
        AMS_ASSERT(session != nullptr);

        /* Detach the device from the session. */
        session->GetDevice().DetachSession(session);

        /* Detach the session from the device. */
        session->DetachDevice();
    }

}
