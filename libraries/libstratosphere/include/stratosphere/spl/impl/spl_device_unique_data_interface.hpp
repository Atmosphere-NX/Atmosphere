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
#include <stratosphere/spl/spl_types.hpp>
#include <stratosphere/spl/impl/spl_crypto_interface.hpp>

#define AMS_SPL_I_DEVICE_UNIQUE_DATA_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                                                                                                  \
    AMS_SF_METHOD_INFO(C, H, 13, Result, DecryptDeviceUniqueDataDeprecated, (const sf::OutPointerBuffer &dst, const sf::InPointerBuffer &src, spl::AccessKey access_key, spl::KeySource key_source, u32 option), (dst, src, access_key, key_source, option), hos::Version_Min,   hos::Version_4_1_0) \
    AMS_SF_METHOD_INFO(C, H, 13, Result, DecryptDeviceUniqueData,           (const sf::OutPointerBuffer &dst, const sf::InPointerBuffer &src, spl::AccessKey access_key, spl::KeySource key_source),             (dst, src, access_key, key_source),         hos::Version_5_0_0)

AMS_SF_DEFINE_INTERFACE_WITH_BASE(ams::spl::impl, IDeviceUniqueDataInterface, ::ams::spl::impl::ICryptoInterface, AMS_SPL_I_DEVICE_UNIQUE_DATA_INTERFACE_INTERFACE_INFO)
