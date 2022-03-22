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
#include <stratosphere/spl/impl/spl_general_interface.hpp>

#define AMS_SPL_I_CRYPTO_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                                               \
    AMS_SF_METHOD_INFO(C, H,  2, Result, GenerateAesKek,              (sf::Out<spl::AccessKey> out_access_key, spl::KeySource key_source, u32 generation, u32 option),              (out_access_key, key_source, generation, option)) \
    AMS_SF_METHOD_INFO(C, H,  3, Result, LoadAesKey,                  (s32 keyslot, spl::AccessKey access_key, spl::KeySource key_source),                                          (keyslot, access_key, key_source))                \
    AMS_SF_METHOD_INFO(C, H,  4, Result, GenerateAesKey,              (sf::Out<spl::AesKey> out_key, spl::AccessKey access_key, spl::KeySource key_source),                         (out_key, access_key, key_source))                \
    AMS_SF_METHOD_INFO(C, H, 14, Result, DecryptAesKey,               (sf::Out<spl::AesKey> out_key, spl::KeySource key_source, u32 generation, u32 option),                        (out_key, key_source, generation, option))        \
    AMS_SF_METHOD_INFO(C, H, 15, Result, ComputeCtr,                  (const sf::OutNonSecureBuffer &out_buf, s32 keyslot, const sf::InNonSecureBuffer &in_buf, spl::IvCtr iv_ctr), (out_buf, keyslot, in_buf, iv_ctr))               \
    AMS_SF_METHOD_INFO(C, H, 16, Result, ComputeCmac,                 (sf::Out<spl::Cmac> out_cmac, s32 keyslot, const sf::InPointerBuffer &in_buf),                                (out_cmac, keyslot, in_buf))                      \
    AMS_SF_METHOD_INFO(C, H, 21, Result, AllocateAesKeySlot,          (sf::Out<s32> out_keyslot),                                                                                   (out_keyslot))                                    \
    AMS_SF_METHOD_INFO(C, H, 22, Result, DeallocateAesKeySlot,        (s32 keyslot),                                                                                                (keyslot))                                        \
    AMS_SF_METHOD_INFO(C, H, 23, Result, GetAesKeySlotAvailableEvent, (sf::OutCopyHandle out_hnd),                                                                                  (out_hnd))

AMS_SF_DEFINE_INTERFACE_WITH_BASE(ams::spl::impl, ICryptoInterface, ::ams::spl::impl::IGeneralInterface, AMS_SPL_I_CRYPTO_INTERFACE_INTERFACE_INFO)
