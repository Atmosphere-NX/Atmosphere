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
#include <stratosphere/powctl/powctl_types.hpp>

namespace ams::powctl {

    namespace impl {

        class SessionImpl : public ::ams::ddsf::ISession {
            NON_COPYABLE(SessionImpl);
            NON_MOVEABLE(SessionImpl);
            AMS_DDSF_CASTABLE_TRAITS(ams::powctl::impl::SessionImpl, ::ams::ddsf::ISession);
            public:
                SessionImpl() : ISession() { /* ... */ }

                ~SessionImpl() { ddsf::CloseSession(this); }
        };

    }

    struct Session {
        bool has_session;
        util::TypedStorage<impl::SessionImpl> impl_storage;

        struct ConstantInitializeTag{};

        constexpr Session(ConstantInitializeTag) : has_session(false), impl_storage() { /* ... */ }

        Session() : has_session(false) { /* ... */ }
    };

    Result OpenSession(Session *out, DeviceCode device_code, ddsf::AccessMode access_mode);
    void CloseSession(Session &session);

}
