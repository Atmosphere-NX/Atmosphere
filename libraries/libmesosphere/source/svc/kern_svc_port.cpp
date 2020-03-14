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

        Result ManageNamedPort(ams::svc::Handle *out_server_handle, KUserPointer<const char *> user_name, s32 max_sessions) {
            /* Copy the provided name from user memory to kernel memory. */
            char name[KObjectName::NameLengthMax] = {};
            R_TRY(user_name.CopyStringTo(name, sizeof(name)));

            /* Validate that sessions and name are valid. */
            R_UNLESS(max_sessions >= 0,                svc::ResultOutOfRange());
            R_UNLESS(name[sizeof(name) - 1] == '\x00', svc::ResultOutOfRange());

            if (max_sessions > 0) {
                MESOSPHERE_LOG("Creating Named Port %s (max sessions = %d)\n", name, max_sessions);
                /* Get the current handle table. */
                auto &handle_table = GetCurrentProcess().GetHandleTable();

                /* Create a new port. */
                KPort *port = KPort::Create();
                R_UNLESS(port != nullptr, svc::ResultOutOfResource());

                /* Reserve a handle for the server port. */
                R_TRY(handle_table.Reserve(out_server_handle));
                auto reserve_guard = SCOPE_GUARD { handle_table.Unreserve(*out_server_handle); };

                /* Initialize the new port. */
                port->Initialize(max_sessions, false, 0);

                /* Register the port. */
                KPort::Register(port);

                /* Register the handle in the table. */
                handle_table.Register(*out_server_handle, std::addressof(port->GetServerPort()));
                reserve_guard.Cancel();
                auto register_guard = SCOPE_GUARD { handle_table.Remove(*out_server_handle); };

                /* Create a new object name. */
                R_TRY(KObjectName::NewFromName(std::addressof(port->GetClientPort()), name));

                /* Perform resource cleanup. */
                port->GetServerPort().Close();
                port->GetClientPort().Close();
                register_guard.Cancel();
            } else /* if (max_sessions == 0) */ {
                MESOSPHERE_LOG("Deleting Named Port %s\n", name);

                /* Ensure that this else case is correct. */
                MESOSPHERE_AUDIT(max_sessions == 0);

                /* If we're closing, there's no server handle. */
                *out_server_handle = ams::svc::InvalidHandle;

                /* Delete the object. */
                R_TRY(KObjectName::Delete<KClientPort>(name));
            }

            return ResultSuccess();
        }

    }

    /* =============================    64 ABI    ============================= */

    Result ConnectToNamedPort64(ams::svc::Handle *out_handle, KUserPointer<const char *> name) {
        MESOSPHERE_PANIC("Stubbed SvcConnectToNamedPort64 was called.");
    }

    Result CreatePort64(ams::svc::Handle *out_server_handle, ams::svc::Handle *out_client_handle, int32_t max_sessions, bool is_light, ams::svc::Address name) {
        MESOSPHERE_PANIC("Stubbed SvcCreatePort64 was called.");
    }

    Result ManageNamedPort64(ams::svc::Handle *out_server_handle, KUserPointer<const char *> name, int32_t max_sessions) {
        return ManageNamedPort(out_server_handle, name, max_sessions);
    }

    Result ConnectToPort64(ams::svc::Handle *out_handle, ams::svc::Handle port) {
        MESOSPHERE_PANIC("Stubbed SvcConnectToPort64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    Result ConnectToNamedPort64From32(ams::svc::Handle *out_handle, KUserPointer<const char *> name) {
        MESOSPHERE_PANIC("Stubbed SvcConnectToNamedPort64From32 was called.");
    }

    Result CreatePort64From32(ams::svc::Handle *out_server_handle, ams::svc::Handle *out_client_handle, int32_t max_sessions, bool is_light, ams::svc::Address name) {
        MESOSPHERE_PANIC("Stubbed SvcCreatePort64From32 was called.");
    }

    Result ManageNamedPort64From32(ams::svc::Handle *out_server_handle, KUserPointer<const char *> name, int32_t max_sessions) {
        return ManageNamedPort(out_server_handle, name, max_sessions);
    }

    Result ConnectToPort64From32(ams::svc::Handle *out_handle, ams::svc::Handle port) {
        MESOSPHERE_PANIC("Stubbed SvcConnectToPort64From32 was called.");
    }

}
