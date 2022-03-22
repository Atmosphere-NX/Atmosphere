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

#define AMS_SPL_I_FS_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                                                                                                                                            \
    AMS_SF_METHOD_INFO(C, H,  9, Result, DecryptAndStoreGcKeyDeprecated, (const sf::InPointerBuffer &src, spl::AccessKey access_key, spl::KeySource key_source, u32 option),                                                                 (src, access_key, key_source, option),    hos::Version_Min,   hos::Version_4_1_0) \
    AMS_SF_METHOD_INFO(C, H,  9, Result, DecryptAndStoreGcKey,           (const sf::InPointerBuffer &src, spl::AccessKey access_key, spl::KeySource key_source),                                                                             (src, access_key, key_source),            hos::Version_5_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 10, Result, DecryptGcMessage,               (sf::Out<u32> out_size, const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest), (out_size, out, base, mod, label_digest))                                         \
    AMS_SF_METHOD_INFO(C, H, 12, Result, GenerateSpecificAesKey,         (sf::Out<spl::AesKey> out_key, spl::KeySource key_source, u32 generation, u32 which),                                                                               (out_key, key_source, generation, which))                                         \
    AMS_SF_METHOD_INFO(C, H, 19, Result, LoadPreparedAesKey,             (s32 keyslot, spl::AccessKey access_key),                                                                                                                           (keyslot, access_key))                                                            \
    AMS_SF_METHOD_INFO(C, H, 31, Result, GetPackage2Hash,                (const sf::OutPointerBuffer &dst),                                                                                                                                  (dst), hos::Version_5_0_0)

AMS_SF_DEFINE_INTERFACE_WITH_BASE(ams::spl::impl, IFsInterface, ::ams::spl::impl::ICryptoInterface, AMS_SPL_I_FS_INTERFACE_INTERFACE_INFO)
