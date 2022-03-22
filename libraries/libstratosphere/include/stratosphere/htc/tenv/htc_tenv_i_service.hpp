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
#include <stratosphere/sf.hpp>
#include <stratosphere/htc/tenv/htc_tenv_types.hpp>

#define AMS_HTC_TENV_I_SERVICE_INTERFACE_INFO(C, H)                                                                                                                                              \
    AMS_SF_METHOD_INFO(C, H, 0, Result, GetVariable,                (sf::Out<s64> out_size, const sf::OutBuffer &out_buffer, const htc::tenv::VariableName &name), (out_size, out_buffer, name)) \
    AMS_SF_METHOD_INFO(C, H, 1, Result, GetVariableLength,          (sf::Out<s64> out_size,const htc::tenv::VariableName &name),                                   (out_size, name))             \
    AMS_SF_METHOD_INFO(C, H, 2, Result, WaitUntilVariableAvailable, (s64 timeout_ms),                                                                              (timeout_ms))

AMS_SF_DEFINE_INTERFACE(ams::htc::tenv, IService, AMS_HTC_TENV_I_SERVICE_INTERFACE_INFO)
