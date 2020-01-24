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
#include "spl_api_impl.hpp"
#include "spl_deprecated_service.hpp"

namespace ams::spl {

    Result DeprecatedService::GetConfig(sf::Out<u64> out, u32 which) {
        return impl::GetConfig(out.GetPointer(), static_cast<SplConfigItem>(which));
    }

    Result DeprecatedService::ExpMod(const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &exp, const sf::InPointerBuffer &mod) {
        return impl::ExpMod(out.GetPointer(), out.GetSize(), base.GetPointer(), base.GetSize(), exp.GetPointer(), exp.GetSize(), mod.GetPointer(), mod.GetSize());
    }

    Result DeprecatedService::GenerateAesKek(sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation, u32 option) {
        return impl::GenerateAesKek(out_access_key.GetPointer(), key_source, generation, option);
    }

    Result DeprecatedService::LoadAesKey(u32 keyslot, AccessKey access_key, KeySource key_source) {
        return impl::LoadAesKey(keyslot, this, access_key, key_source);
    }

    Result DeprecatedService::GenerateAesKey(sf::Out<AesKey> out_key, AccessKey access_key, KeySource key_source) {
        return impl::GenerateAesKey(out_key.GetPointer(), access_key, key_source);
    }

    Result DeprecatedService::SetConfig(u32 which, u64 value) {
        return impl::SetConfig(static_cast<SplConfigItem>(which), value);
    }

    Result DeprecatedService::GenerateRandomBytes(const sf::OutPointerBuffer &out) {
        return impl::GenerateRandomBytes(out.GetPointer(), out.GetSize());
    }

    Result DeprecatedService::ImportLotusKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option) {
        return impl::ImportLotusKey(src.GetPointer(), src.GetSize(), access_key, key_source, option);
    }

    Result DeprecatedService::DecryptLotusMessage(sf::Out<u32> out_size, const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest) {
        return impl::DecryptLotusMessage(out_size.GetPointer(), out.GetPointer(), out.GetSize(), base.GetPointer(), base.GetSize(), mod.GetPointer(), mod.GetSize(), label_digest.GetPointer(), label_digest.GetSize());
    }

    Result DeprecatedService::IsDevelopment(sf::Out<bool> is_dev) {
        return impl::IsDevelopment(is_dev.GetPointer());
    }

    Result DeprecatedService::GenerateSpecificAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 which) {
        return impl::GenerateSpecificAesKey(out_key.GetPointer(), key_source, generation, which);
    }

    Result DeprecatedService::DecryptRsaPrivateKey(const sf::OutPointerBuffer &dst, const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option) {
        return impl::DecryptRsaPrivateKey(dst.GetPointer(), dst.GetSize(), src.GetPointer(), src.GetSize(), access_key, key_source, option);
    }

    Result DeprecatedService::DecryptAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 option) {
        return impl::DecryptAesKey(out_key.GetPointer(), key_source, generation, option);
    }

    Result DeprecatedService::CryptAesCtrDeprecated(const sf::OutBuffer &out_buf, u32 keyslot, const sf::InBuffer &in_buf, IvCtr iv_ctr) {
        return impl::CryptAesCtr(out_buf.GetPointer(), out_buf.GetSize(), keyslot, this, in_buf.GetPointer(), in_buf.GetSize(), iv_ctr);
    }

    Result DeprecatedService::CryptAesCtr(const sf::OutNonSecureBuffer &out_buf, u32 keyslot, const sf::InNonSecureBuffer &in_buf, IvCtr iv_ctr) {
        return impl::CryptAesCtr(out_buf.GetPointer(), out_buf.GetSize(), keyslot, this, in_buf.GetPointer(), in_buf.GetSize(), iv_ctr);
    }

    Result DeprecatedService::ComputeCmac(sf::Out<Cmac> out_cmac, u32 keyslot, const sf::InPointerBuffer &in_buf) {
        return impl::ComputeCmac(out_cmac.GetPointer(), keyslot, this, in_buf.GetPointer(), in_buf.GetSize());
    }

    Result DeprecatedService::ImportEsKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option) {
        return impl::ImportEsKey(src.GetPointer(), src.GetSize(), access_key, key_source, option);
    }

    Result DeprecatedService::UnwrapTitleKeyDeprecated(sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest) {
        return impl::UnwrapTitleKey(out_access_key.GetPointer(), base.GetPointer(), base.GetSize(), mod.GetPointer(), mod.GetSize(), label_digest.GetPointer(), label_digest.GetSize(), 0);
    }

    Result DeprecatedService::UnwrapTitleKey(sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest, u32 generation) {
        return impl::UnwrapTitleKey(out_access_key.GetPointer(), base.GetPointer(), base.GetSize(), mod.GetPointer(), mod.GetSize(), label_digest.GetPointer(), label_digest.GetSize(), generation);
    }

    Result DeprecatedService::LoadTitleKey(u32 keyslot, AccessKey access_key) {
        return impl::LoadTitleKey(keyslot, this, access_key);
    }

    Result DeprecatedService::UnwrapCommonTitleKeyDeprecated(sf::Out<AccessKey> out_access_key, KeySource key_source) {
        return impl::UnwrapCommonTitleKey(out_access_key.GetPointer(), key_source, 0);
    }

    Result DeprecatedService::UnwrapCommonTitleKey(sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation) {
        return impl::UnwrapCommonTitleKey(out_access_key.GetPointer(), key_source, generation);
    }

    Result DeprecatedService::AllocateAesKeyslot(sf::Out<u32> out_keyslot) {
        return impl::AllocateAesKeyslot(out_keyslot.GetPointer(), this);
    }

    Result DeprecatedService::FreeAesKeyslot(u32 keyslot) {
        return impl::FreeAesKeyslot(keyslot, this);
    }

    void DeprecatedService::GetAesKeyslotAvailableEvent(sf::OutCopyHandle out_hnd) {
        out_hnd.SetValue(impl::GetAesKeyslotAvailableEventHandle());
    }

    Result DeprecatedService::SetBootReason(BootReasonValue boot_reason) {
        return impl::SetBootReason(boot_reason);
    }

    Result DeprecatedService::GetBootReason(sf::Out<BootReasonValue> out) {
        return impl::GetBootReason(out.GetPointer());
    }

}
