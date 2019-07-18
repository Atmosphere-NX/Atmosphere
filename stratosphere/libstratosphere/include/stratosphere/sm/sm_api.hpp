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

#pragma once

#include "sm_types.hpp"

namespace sts::sm {

    /* Ordinary SM API. */
    Result GetService(Service *out, ServiceName name);
    Result RegisterService(Handle *out, ServiceName name, size_t max_sessions, bool is_light);
    Result UnregisterService(ServiceName name);

    /* Atmosphere extensions. */
    Result HasService(bool *out, ServiceName name);
    Result WaitService(ServiceName name);

}
