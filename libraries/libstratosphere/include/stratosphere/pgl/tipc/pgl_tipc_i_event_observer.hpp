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
#include <stratosphere/os.hpp>
#include <stratosphere/pm.hpp>
#include <stratosphere/tipc.hpp>
#include <stratosphere/pgl/pgl_types.hpp>

#define AMS_PGL_TIPC_I_EVENT_OBSERVER_INTERFACE_INFO(C, H)                                                          \
    AMS_TIPC_METHOD_INFO(C, H, 0, Result, GetProcessEventHandle, (ams::tipc::OutCopyHandle out),             (out)) \
    AMS_TIPC_METHOD_INFO(C, H, 1, Result, GetProcessEventInfo,   (ams::tipc::Out<pm::ProcessEventInfo> out), (out))

AMS_TIPC_DEFINE_INTERFACE(ams::pgl::tipc, IEventObserver, AMS_PGL_TIPC_I_EVENT_OBSERVER_INTERFACE_INFO);

