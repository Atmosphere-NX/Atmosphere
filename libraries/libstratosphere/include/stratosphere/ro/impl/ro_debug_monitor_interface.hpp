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
#include <stratosphere/ro/ro_types.hpp>
#include <stratosphere/sf.hpp>

namespace ams::ro::impl {

    #define AMS_RO_I_DEBUG_MONITOR_INTERFACE_INTERFACE_INFO(C, H)                                                                                                       \
        AMS_SF_METHOD_INFO(C, H, 0, Result, GetProcessModuleInfo, (sf::Out<u32> out_count, const sf::OutArray<LoaderModuleInfo> &out_infos, os::ProcessId process_id))

    AMS_SF_DEFINE_INTERFACE(IDebugMonitorInterface, AMS_RO_I_DEBUG_MONITOR_INTERFACE_INTERFACE_INFO)

}
