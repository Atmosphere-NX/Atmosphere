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
#include <stratosphere/ncm/ncm_program_id.hpp>
#include <stratosphere/cfg/cfg_types.hpp>
#include <stratosphere/sf.hpp>

#define AMS_MITM_PM_IMPL_I_PM_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 65000, Result, PrepareLaunchProgram, (sf::Out<u64> out_boost_size, ncm::ProgramId program_id, const cfg::OverrideStatus &override_status, bool is_application), (out_boost_size, program_id, override_status, is_application))

AMS_SF_DEFINE_INTERFACE(ams::mitm::pm::impl, IPmInterface, AMS_MITM_PM_IMPL_I_PM_INTERFACE_INFO, 0xEA88789C)

