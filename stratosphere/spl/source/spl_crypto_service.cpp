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
#include <stratosphere.hpp>
#include "spl_api_impl.hpp"
#include "spl_crypto_service.hpp"

namespace ams::spl {

    CryptoService::~CryptoService() {
        /* Free any keyslots this service is using. */
        impl::DeallocateAllAesKeySlots(this);
    }

    Result CryptoService::GenerateAesKek(sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation, u32 option) {
        return impl::GenerateAesKek(out_access_key.GetPointer(), key_source, generation, option);
    }

    Result CryptoService::LoadAesKey(s32 keyslot, AccessKey access_key, KeySource key_source) {
        return impl::LoadAesKey(keyslot, this, access_key, key_source);
    }

    Result CryptoService::GenerateAesKey(sf::Out<AesKey> out_key, AccessKey access_key, KeySource key_source) {
        return impl::GenerateAesKey(out_key.GetPointer(), access_key, key_source);
    }

    Result CryptoService::DecryptAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 option) {
        return impl::DecryptAesKey(out_key.GetPointer(), key_source, generation, option);
    }

    Result CryptoService::ComputeCtr(const sf::OutNonSecureBuffer &out_buf, s32 keyslot, const sf::InNonSecureBuffer &in_buf, IvCtr iv_ctr) {
        return impl::ComputeCtr(out_buf.GetPointer(), out_buf.GetSize(), keyslot, this, in_buf.GetPointer(), in_buf.GetSize(), iv_ctr);
    }

    Result CryptoService::ComputeCmac(sf::Out<Cmac> out_cmac, s32 keyslot, const sf::InPointerBuffer &in_buf) {
        return impl::ComputeCmac(out_cmac.GetPointer(), keyslot, this, in_buf.GetPointer(), in_buf.GetSize());
    }

    Result CryptoService::AllocateAesKeySlot(sf::Out<s32> out_keyslot) {
        return impl::AllocateAesKeySlot(out_keyslot.GetPointer(), this);
    }

    Result CryptoService::DeallocateAesKeySlot(s32 keyslot) {
        return impl::DeallocateAesKeySlot(keyslot, this);
    }

    Result CryptoService::GetAesKeySlotAvailableEvent(sf::OutCopyHandle out_hnd) {
        out_hnd.SetValue(impl::GetAesKeySlotAvailableEventHandle());
        return ResultSuccess();
    }

}
