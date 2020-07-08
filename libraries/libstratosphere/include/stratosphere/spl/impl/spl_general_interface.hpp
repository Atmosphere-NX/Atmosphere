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
#include <stratosphere/sf.hpp>
#include <stratosphere/spl/spl_types.hpp>

namespace ams::spl::impl {

    #define AMS_SPL_I_GENERAL_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                              \
        AMS_SF_METHOD_INFO(C, H,  0, Result, GetConfig,           (sf::Out<u64> out, u32 which))                                                                                                                          \
        AMS_SF_METHOD_INFO(C, H,  1, Result, ModularExponentiate, (const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &exp, const sf::InPointerBuffer &mod))                     \
        AMS_SF_METHOD_INFO(C, H,  5, Result, SetConfig,           (u32 which, u64 value))                                                                                                                                 \
        AMS_SF_METHOD_INFO(C, H,  7, Result, GenerateRandomBytes, (const sf::OutPointerBuffer &out))                                                                                                                      \
        AMS_SF_METHOD_INFO(C, H, 11, Result, IsDevelopment,       (sf::Out<bool> is_dev))                                                                                                                                 \
        AMS_SF_METHOD_INFO(C, H, 24, Result, SetBootReason,       (spl::BootReasonValue boot_reason),                                                                                                 hos::Version_3_0_0) \
        AMS_SF_METHOD_INFO(C, H, 25, Result, GetBootReason,       (sf::Out<spl::BootReasonValue> out),                                                                                                hos::Version_3_0_0)

    AMS_SF_DEFINE_INTERFACE(IGeneralInterface, AMS_SPL_I_GENERAL_INTERFACE_INTERFACE_INFO)

}
