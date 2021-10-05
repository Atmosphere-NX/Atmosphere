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

#include <stratosphere/sm/sm_types.hpp>

namespace ams::sm::mitm {

    /* Mitm API. */
    Result InstallMitm(os::NativeHandle *out_port, os::NativeHandle *out_query, ServiceName name);
    Result UninstallMitm(ServiceName name);
    Result DeclareFutureMitm(ServiceName name);
    Result ClearFutureMitm(ServiceName name);
    Result AcknowledgeSession(Service *out_service, MitmProcessInfo *out_info, ServiceName name);
    Result HasMitm(bool *out, ServiceName name);
    Result WaitMitm(ServiceName name);

}
