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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        Result ManageNamedPort(ams::svc::Handle *out_server_handle, KUserPointer<const char *> user_name, s32 max_sessions) {
            /* Copy the provided name from user memory to kernel memory. */
            char name[KObjectName::NameLengthMax] = {};
            R_TRY(user_name.CopyStringTo(name, sizeof(name)));

            /* Validate that sessions and name are valid. */
            R_UNLESS(max_sessions >= 0,                svc::ResultOutOfRange());
            R_UNLESS(name[sizeof(name) - 1] == '\x00', svc::ResultOutOfRange());

            if (max_sessions > 0) {
                /* Get the current handle table. */
                auto &handle_table = GetCurrentProcess().GetHandleTable();

                /* Create a new port. */
                KPort *port = KPort::Create();
                R_UNLESS(port != nullptr, svc::ResultOutOfResource());

                /* Initialize the new port. */
                port->Initialize(max_sessions, false, 0);

                /* Register the port. */
                KPort::Register(port);

                /* Ensure that our only reference to the port is in the handle table when we're done. */
                ON_SCOPE_EXIT {
                    port->GetClientPort().Close();
                    port->GetServerPort().Close();
                };

                /* Register the handle in the table. */
                R_TRY(handle_table.Add(out_server_handle, std::addressof(port->GetServerPort())));
                ON_RESULT_FAILURE { handle_table.Remove(*out_server_handle); };

                /* Create a new object name. */
                R_TRY(KObjectName::NewFromName(std::addressof(port->GetClientPort()), name));
            } else /* if (max_sessions == 0) */ {
                /* Ensure that this else case is correct. */
                MESOSPHERE_AUDIT(max_sessions == 0);

                /* If we're closing, there's no server handle. */
                *out_server_handle = ams::svc::InvalidHandle;

                /* Delete the object. */
                R_TRY(KObjectName::Delete<KClientPort>(name));
            }

            R_SUCCEED();
        }

        Result CreatePort(ams::svc::Handle *out_server, ams::svc::Handle *out_client, int32_t max_sessions, bool is_light, uintptr_t name) {
            /* Ensure max sessions is valid. */
            R_UNLESS(max_sessions > 0, svc::ResultOutOfRange());

            /* Get the current handle table. */
            auto &handle_table = GetCurrentProcess().GetHandleTable();

            /* Create a new port. */
            KPort *port = KPort::Create();
            R_UNLESS(port != nullptr, svc::ResultOutOfResource());

            /* Initialize the port. */
            port->Initialize(max_sessions, is_light, name);

            /* Ensure that we clean up the port (and its only references are handle table) on function end. */
            ON_SCOPE_EXIT {
                port->GetServerPort().Close();
                port->GetClientPort().Close();
            };

            /* Register the port. */
            KPort::Register(port);

            /* Add the client to the handle table. */
            R_TRY(handle_table.Add(out_client, std::addressof(port->GetClientPort())));

            /* Ensure that we maintaing a clean handle state on exit. */
            ON_RESULT_FAILURE { handle_table.Remove(*out_client); };

            /* Add the server to the handle table. */
            R_RETURN(handle_table.Add(out_server, std::addressof(port->GetServerPort())));
        }

        Result ConnectToNamedPort(ams::svc::Handle *out, KUserPointer<const char *> user_name) {
            /* Copy the provided name from user memory to kernel memory. */
            char name[KObjectName::NameLengthMax] = {};
            R_TRY(user_name.CopyStringTo(name, sizeof(name)));

            /* Validate that name is valid. */
            R_UNLESS(name[sizeof(name) - 1] == '\x00', svc::ResultOutOfRange());

            /* Get the current handle table. */
            auto &handle_table = GetCurrentProcess().GetHandleTable();

            /* Find the client port. */
            auto port = KObjectName::Find<KClientPort>(name);
            R_UNLESS(port.IsNotNull(), svc::ResultNotFound());

            /* Reserve a handle for the port. */
            /* NOTE: Nintendo really does write directly to the output handle here. */
            R_TRY(handle_table.Reserve(out));
            ON_RESULT_FAILURE { handle_table.Unreserve(*out); };

            /* Create a session. */
            KClientSession *session;
            R_TRY(port->CreateSession(std::addressof(session)));

            /* Register the session in the table, close the extra reference. */
            handle_table.Register(*out, session);
            session->Close();

            /* We succeeded. */
            R_SUCCEED();
        }

        Result ConnectToPort(ams::svc::Handle *out, ams::svc::Handle port) {
            /* Get the current handle table. */
            auto &handle_table = GetCurrentProcess().GetHandleTable();

            /* Get the client port. */
            KScopedAutoObject client_port = handle_table.GetObject<KClientPort>(port);
            R_UNLESS(client_port.IsNotNull(), svc::ResultInvalidHandle());

            /* Reserve a handle for the port. */
            /* NOTE: Nintendo really does write directly to the output handle here. */
            R_TRY(handle_table.Reserve(out));
            ON_RESULT_FAILURE { handle_table.Unreserve(*out); };

            /* Create the session. */
            KAutoObject *session;
            if (client_port->IsLight()) {
                R_TRY(client_port->CreateLightSession(reinterpret_cast<KLightClientSession **>(std::addressof(session))));
            } else {
                R_TRY(client_port->CreateSession(reinterpret_cast<KClientSession **>(std::addressof(session))));
            }

            /* Register the session. */
            handle_table.Register(*out, session);
            session->Close();

            /* We succeeded. */
            R_SUCCEED();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result ConnectToNamedPort64(ams::svc::Handle *out_handle, KUserPointer<const char *> name) {
        R_RETURN(ConnectToNamedPort(out_handle, name));
    }

    Result CreatePort64(ams::svc::Handle *out_server_handle, ams::svc::Handle *out_client_handle, int32_t max_sessions, bool is_light, ams::svc::Address name) {
        R_RETURN(CreatePort(out_server_handle, out_client_handle, max_sessions, is_light, name));
    }

    Result ManageNamedPort64(ams::svc::Handle *out_server_handle, KUserPointer<const char *> name, int32_t max_sessions) {
        R_RETURN(ManageNamedPort(out_server_handle, name, max_sessions));
    }

    Result ConnectToPort64(ams::svc::Handle *out_handle, ams::svc::Handle port) {
        R_RETURN(ConnectToPort(out_handle, port));
    }

    /* ============================= 64From32 ABI ============================= */

    Result ConnectToNamedPort64From32(ams::svc::Handle *out_handle, KUserPointer<const char *> name) {
        R_RETURN(ConnectToNamedPort(out_handle, name));
    }

    Result CreatePort64From32(ams::svc::Handle *out_server_handle, ams::svc::Handle *out_client_handle, int32_t max_sessions, bool is_light, ams::svc::Address name) {
        R_RETURN(CreatePort(out_server_handle, out_client_handle, max_sessions, is_light, name));
    }

    Result ManageNamedPort64From32(ams::svc::Handle *out_server_handle, KUserPointer<const char *> name, int32_t max_sessions) {
        R_RETURN(ManageNamedPort(out_server_handle, name, max_sessions));
    }

    Result ConnectToPort64From32(ams::svc::Handle *out_handle, ams::svc::Handle port) {
        R_RETURN(ConnectToPort(out_handle, port));
    }

}
