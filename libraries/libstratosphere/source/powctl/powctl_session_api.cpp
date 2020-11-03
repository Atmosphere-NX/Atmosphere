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
#include "impl/powctl_device_management.hpp"

namespace ams::powctl {

    namespace {

        ddsf::AccessMode SanitizeAccessMode(ddsf::AccessMode access_mode) {
            switch (access_mode) {
                case ddsf::AccessMode_Read:
                case ddsf::AccessMode_Write:
                case ddsf::AccessMode_ReadWrite:
                case ddsf::AccessMode_WriteShared:
                case ddsf::AccessMode_ReadWriteShared:
                    return access_mode;
                default:
                    return ddsf::AccessMode_None;
            }
        }

        impl::SessionImpl &GetSessionImpl(Session &session) {
            return GetReference(session.impl_storage);
        }

        void DestroySession(Session &session) {
            GetSessionImpl(session).~SessionImpl();
            session.has_session = false;
        }

        void DestroySessionIfNecessary(Session &session) {
            if (session.has_session) {
                DestroySession(session);
            }
        }

        void CloseSessionIfOpen(Session &session) {
            if (session.has_session && GetSessionImpl(session).IsOpen()) {
                DestroySession(session);
            }
        }

    }

    Result OpenSession(Session *out, DeviceCode device_code, ddsf::AccessMode access_mode) {
        /* Validate input. */
        AMS_ASSERT(out != nullptr);
        access_mode = SanitizeAccessMode(access_mode);

        /* Find the target device. */
        impl::IDevice *device = nullptr;
        R_TRY(impl::FindDevice(std::addressof(device), device_code));

        /* Clean up the session if we have one. */
        DestroySessionIfNecessary(*out);

        /* Construct the session. */
        new (std::addressof(GetSessionImpl(*out))) impl::SessionImpl;
        auto guard = SCOPE_GUARD { DestroySessionIfNecessary(*out); };

        /* Try to open the session. */
        R_TRY(ddsf::OpenSession(device, std::addressof(GetSessionImpl(*out)), access_mode));

        /* We opened the session! */
        guard.Cancel();
        return ResultSuccess();
    }

    void CloseSession(Session &session) {
        /* This seems extremely unnecessary/duplicate, but it's what Nintendo does. */
        CloseSessionIfOpen(session);
        DestroySession(session);
    }

}