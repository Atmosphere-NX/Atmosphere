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
#include <stratosphere/spl/impl/spl_device_unique_data_interface.hpp>

namespace ams::spl::impl {

    #define AMS_SPL_I_SSL_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                      \
        AMS_SPL_I_DEVICE_UNIQUE_DATA_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                           \
        AMS_SF_METHOD_INFO(C, H, 26, Result, DecryptAndStoreSslClientCertKey,         (const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source),                       hos::Version_5_0_0) \
        AMS_SF_METHOD_INFO(C, H, 27, Result, ModularExponentiateWithSslClientCertKey, (const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod), hos::Version_5_0_0)

    AMS_SF_DEFINE_INTERFACE(ISslInterface, AMS_SPL_I_SSL_INTERFACE_INTERFACE_INFO)

}
