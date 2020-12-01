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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        template<typename T>
        Result CreateSession(ams::svc::Handle *out_server, ams::svc::Handle *out_client, uintptr_t name) {
            /* Get the current process and handle table. */
            auto &process      = GetCurrentProcess();
            auto &handle_table = process.GetHandleTable();

            /* Reserve a new session from the process resource limit. */
            KScopedResourceReservation session_reservation(std::addressof(process), ams::svc::LimitableResource_SessionCountMax);
            R_UNLESS(session_reservation.Succeeded(), svc::ResultLimitReached());

            /* Create a new session. */
            T *session = T::Create();
            R_UNLESS(session != nullptr, svc::ResultOutOfResource());

            /* Initialize the session. */
            session->Initialize(nullptr, name);

            /* Commit the session reservation. */
            session_reservation.Commit();

            /* Ensure that we clean up the session (and its only references are handle table) on function end. */
            ON_SCOPE_EXIT {
                session->GetServerSession().Close();
                session->GetClientSession().Close();
            };

            /* Register the session. */
            T::Register(session);

            /* Add the server session to the handle table. */
            R_TRY(handle_table.Add(out_server, std::addressof(session->GetServerSession())));

            /* Ensure that we maintaing a clean handle state on exit. */
            auto handle_guard = SCOPE_GUARD { handle_table.Remove(*out_server); };

            /* Add the client session to the handle table. */
            R_TRY(handle_table.Add(out_client, std::addressof(session->GetClientSession())));

            /* We succeeded! */
            handle_guard.Cancel();
            return ResultSuccess();
        }

        Result CreateSession(ams::svc::Handle *out_server, ams::svc::Handle *out_client, bool is_light, uintptr_t name) {
            if (is_light) {
                return CreateSession<KLightSession>(out_server, out_client, name);
            } else {
                return CreateSession<KSession>(out_server, out_client, name);
            }
        }

        Result AcceptSession(ams::svc::Handle *out, ams::svc::Handle port_handle) {
            /* Get the current handle table. */
            auto &handle_table = GetCurrentProcess().GetHandleTable();

            /* Get the server port. */
            KScopedAutoObject port = handle_table.GetObject<KServerPort>(port_handle);
            R_UNLESS(port.IsNotNull(), svc::ResultInvalidHandle());

            /* Reserve an entry for the new session. */
            R_TRY(handle_table.Reserve(out));
            auto handle_guard = SCOPE_GUARD { handle_table.Unreserve(*out); };

            /* Accept the session. */
            KAutoObject *session;
            if (port->IsLight()) {
                session = port->AcceptLightSession();
            } else {
                session = port->AcceptSession();
            }

            /* Ensure we accepted successfully. */
            R_UNLESS(session != nullptr, svc::ResultNotFound());

            /* Register the session. */
            handle_table.Register(*out, session);
            handle_guard.Cancel();
            session->Close();

            return ResultSuccess();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result CreateSession64(ams::svc::Handle *out_server_session_handle, ams::svc::Handle *out_client_session_handle, bool is_light, ams::svc::Address name) {
        return CreateSession(out_server_session_handle, out_client_session_handle, is_light, name);
    }

    Result AcceptSession64(ams::svc::Handle *out_handle, ams::svc::Handle port) {
        return AcceptSession(out_handle, port);
    }

    /* ============================= 64From32 ABI ============================= */

    Result CreateSession64From32(ams::svc::Handle *out_server_session_handle, ams::svc::Handle *out_client_session_handle, bool is_light, ams::svc::Address name) {
        return CreateSession(out_server_session_handle, out_client_session_handle, is_light, name);
    }

    Result AcceptSession64From32(ams::svc::Handle *out_handle, ams::svc::Handle port) {
        return AcceptSession(out_handle, port);
    }

}
