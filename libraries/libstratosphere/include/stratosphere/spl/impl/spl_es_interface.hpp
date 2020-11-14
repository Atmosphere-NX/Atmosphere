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

    #define AMS_SPL_I_ES_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                                                                                                      \
        AMS_SPL_I_DEVICE_UNIQUE_DATA_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                                                                                          \
        AMS_SF_METHOD_INFO(C, H, 17, Result, LoadEsDeviceKeyDeprecated,               (const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option),                                                                      hos::Version_Min,   hos::Version_4_1_0) \
        AMS_SF_METHOD_INFO(C, H, 17, Result, LoadEsDeviceKey,                         (const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source),                                                                                  hos::Version_5_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 18, Result, PrepareEsTitleKey,                       (sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest, u32 generation))                                         \
        AMS_SF_METHOD_INFO(C, H, 20, Result, PrepareCommonEsTitleKey,                 (sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation),                                                                                     hos::Version_2_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 28, Result, DecryptAndStoreDrmDeviceCertKey,         (const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source),                                                                                  hos::Version_5_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 29, Result, ModularExponentiateWithDrmDeviceCertKey, (const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod),                                                            hos::Version_5_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 31, Result, PrepareEsArchiveKey,                     (sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest, u32 generation), hos::Version_6_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 32, Result, LoadPreparedAesKey,                      (s32 keyslot, AccessKey access_key),                                                                                                                           hos::Version_6_0_0)

    AMS_SF_DEFINE_INTERFACE(IEsInterface, AMS_SPL_I_ES_INTERFACE_INTERFACE_INFO)

}
