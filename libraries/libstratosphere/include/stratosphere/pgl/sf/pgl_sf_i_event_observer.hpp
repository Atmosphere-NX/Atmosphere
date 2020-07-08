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

#pragma once
#include <vapours.hpp>
#include <stratosphere/os.hpp>
#include <stratosphere/pm.hpp>
#include <stratosphere/pgl/pgl_types.hpp>

namespace ams::pgl::sf {

    #define AMS_PGL_I_EVENT_OBSERVER_INTERFACE_INFO(C, H)                                                    \
        AMS_SF_METHOD_INFO(C, H, 0, Result, GetProcessEventHandle, (ams::sf::OutCopyHandle out))             \
        AMS_SF_METHOD_INFO(C, H, 1, Result, GetProcessEventInfo,   (ams::sf::Out<pm::ProcessEventInfo> out))

    AMS_SF_DEFINE_INTERFACE(IEventObserver, AMS_PGL_I_EVENT_OBSERVER_INTERFACE_INFO);

}