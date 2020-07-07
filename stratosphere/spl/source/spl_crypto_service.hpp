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
#include "spl_general_service.hpp"

namespace ams::spl {

    class CryptoService : public GeneralService {
        public:
            virtual ~CryptoService();
        public:
            /* Actual commands. */
            Result GenerateAesKek(sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation, u32 option);
            Result LoadAesKey(s32 keyslot, AccessKey access_key, KeySource key_source);
            Result GenerateAesKey(sf::Out<AesKey> out_key, AccessKey access_key, KeySource key_source);
            Result DecryptAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 option);
            Result ComputeCtr(const sf::OutNonSecureBuffer &out_buf, s32 keyslot, const sf::InNonSecureBuffer &in_buf, IvCtr iv_ctr);
            Result ComputeCmac(sf::Out<Cmac> out_cmac, s32 keyslot, const sf::InPointerBuffer &in_buf);
            Result AllocateAesKeySlot(sf::Out<s32> out_keyslot);
            Result DeallocateAesKeySlot(s32 keyslot);
            Result GetAesKeySlotAvailableEvent(sf::OutCopyHandle out_hnd);
    };
    static_assert(spl::impl::IsICryptoInterface<CryptoService>);

}
