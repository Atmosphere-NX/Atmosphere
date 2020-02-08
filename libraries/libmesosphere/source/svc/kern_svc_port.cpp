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



    }

    /* =============================    64 ABI    ============================= */

    Result ConnectToNamedPort64(ams::svc::Handle *out_handle, KUserPointer<const char *> name) {
        MESOSPHERE_PANIC("Stubbed SvcConnectToNamedPort64 was called.");
    }

    Result CreatePort64(ams::svc::Handle *out_server_handle, ams::svc::Handle *out_client_handle, int32_t max_sessions, bool is_light, ams::svc::Address name) {
        MESOSPHERE_PANIC("Stubbed SvcCreatePort64 was called.");
    }

    Result ManageNamedPort64(ams::svc::Handle *out_server_handle, KUserPointer<const char *> name, int32_t max_sessions) {
        MESOSPHERE_PANIC("Stubbed SvcManageNamedPort64 was called.");
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
        MESOSPHERE_PANIC("Stubbed SvcManageNamedPort64From32 was called.");
    }

    Result ConnectToPort64From32(ams::svc::Handle *out_handle, ams::svc::Handle port) {
        MESOSPHERE_PANIC("Stubbed SvcConnectToPort64From32 was called.");
    }

}