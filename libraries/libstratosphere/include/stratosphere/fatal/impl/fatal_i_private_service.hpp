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
#include <stratosphere/fatal/fatal_types.hpp>
#include <stratosphere/sf.hpp>

#define AMS_FATAL_I_PRIVATE_SERVICE_INTERFACE_INFO(C, H)                                                                                                                                                                                                     \
    AMS_SF_METHOD_INFO(C, H,  0, Result, GetFatalEvent,   (sf::OutCopyHandle out_h),                                                                                                                       (out_h))                                          \
    AMS_SF_METHOD_INFO(C, H, 10, Result, GetFatalContext, (sf::Out<Result> out_error, sf::Out<ncm::ProgramId> out_program_id, sf::Out<fatal::FatalPolicy> out_policy, sf::Out<fatal::CpuContext> out_ctx), (out_error, out_program_id, out_policy, out_ctx))

AMS_SF_DEFINE_INTERFACE(ams::fatal::impl, IPrivateService, AMS_FATAL_I_PRIVATE_SERVICE_INTERFACE_INFO, 0x6C3C9791)
