/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include <switch.h>
#include <stratosphere.hpp>

#include "spl_deprecated_service.hpp"
#include "spl_api.hpp"

namespace sts::spl {

    Result DeprecatedService::GetConfig(Out<u64> out, u32 which) {
        return spl::GetConfig(out.GetPointer(), static_cast<SplConfigItem>(which));
    }

    Result DeprecatedService::ExpMod(OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> exp, InPointer<u8> mod) {
        return spl::ExpMod(out.pointer, out.num_elements, base.pointer, base.num_elements, exp.pointer, exp.num_elements, mod.pointer, mod.num_elements);
    }

    Result DeprecatedService::GenerateAesKek(Out<AccessKey> out_access_key, KeySource key_source, u32 generation, u32 option) {
        return spl::GenerateAesKek(out_access_key.GetPointer(), key_source, generation, option);
    }

    Result DeprecatedService::LoadAesKey(u32 keyslot, AccessKey access_key, KeySource key_source) {
        return spl::LoadAesKey(keyslot, this, access_key, key_source);
    }

    Result DeprecatedService::GenerateAesKey(Out<AesKey> out_key, AccessKey access_key, KeySource key_source) {
        return spl::GenerateAesKey(out_key.GetPointer(), access_key, key_source);
    }

    Result DeprecatedService::SetConfig(u32 which, u64 value) {
        return spl::SetConfig(static_cast<SplConfigItem>(which), value);
    }

    Result DeprecatedService::GenerateRandomBytes(OutPointerWithClientSize<u8> out) {
        return spl::GenerateRandomBytes(out.pointer, out.num_elements);
    }

    Result DeprecatedService::ImportLotusKey(InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option) {
        return spl::ImportLotusKey(src.pointer, src.num_elements, access_key, key_source, option);
    }

    Result DeprecatedService::DecryptLotusMessage(Out<u32> out_size, OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> mod, InPointer<u8> label_digest) {
        return spl::DecryptLotusMessage(out_size.GetPointer(), out.pointer, out.num_elements, base.pointer, base.num_elements, mod.pointer, mod.num_elements, label_digest.pointer, label_digest.num_elements);
    }

    Result DeprecatedService::IsDevelopment(Out<bool> is_dev) {
        return spl::IsDevelopment(is_dev.GetPointer());
    }

    Result DeprecatedService::GenerateSpecificAesKey(Out<AesKey> out_key, KeySource key_source, u32 generation, u32 which) {
        return spl::GenerateSpecificAesKey(out_key.GetPointer(), key_source, generation, which);
    }

    Result DeprecatedService::DecryptRsaPrivateKey(OutPointerWithClientSize<u8> dst, InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option) {
        return spl::DecryptRsaPrivateKey(dst.pointer, dst.num_elements, src.pointer, src.num_elements, access_key, key_source, option);
    }

    Result DeprecatedService::DecryptAesKey(Out<AesKey> out_key, KeySource key_source, u32 generation, u32 option) {
        return spl::DecryptAesKey(out_key.GetPointer(), key_source, generation, option);
    }

    Result DeprecatedService::CryptAesCtrDeprecated(OutBuffer<u8> out_buf, u32 keyslot, InBuffer<u8> in_buf, IvCtr iv_ctr) {
        return spl::CryptAesCtr(out_buf.buffer, out_buf.num_elements, keyslot, this, in_buf.buffer, in_buf.num_elements, iv_ctr);
    }

    Result DeprecatedService::CryptAesCtr(OutBuffer<u8, BufferType_Type1> out_buf, u32 keyslot, InBuffer<u8, BufferType_Type1> in_buf, IvCtr iv_ctr) {
        return spl::CryptAesCtr(out_buf.buffer, out_buf.num_elements, keyslot, this, in_buf.buffer, in_buf.num_elements, iv_ctr);
    }

    Result DeprecatedService::ComputeCmac(Out<Cmac> out_cmac, u32 keyslot, InPointer<u8> in_buf) {
        return spl::ComputeCmac(out_cmac.GetPointer(), keyslot, this, in_buf.pointer, in_buf.num_elements);
    }

    Result DeprecatedService::ImportEsKey(InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option) {
        return spl::ImportEsKey(src.pointer, src.num_elements, access_key, key_source, option);
    }

    Result DeprecatedService::UnwrapTitleKey(Out<AccessKey> out_access_key, InPointer<u8> base, InPointer<u8> mod, InPointer<u8> label_digest, u32 generation) {
        return spl::UnwrapTitleKey(out_access_key.GetPointer(), base.pointer, base.num_elements, mod.pointer, mod.num_elements, label_digest.pointer, label_digest.num_elements, generation);
    }

    Result DeprecatedService::LoadTitleKey(u32 keyslot, AccessKey access_key) {
        return spl::LoadTitleKey(keyslot, this, access_key);
    }

    Result DeprecatedService::UnwrapCommonTitleKey(Out<AccessKey> out_access_key, KeySource key_source, u32 generation) {
        return spl::UnwrapCommonTitleKey(out_access_key.GetPointer(), key_source, generation);
    }

    Result DeprecatedService::AllocateAesKeyslot(Out<u32> out_keyslot) {
        return spl::AllocateAesKeyslot(out_keyslot.GetPointer(), this);
    }

    Result DeprecatedService::FreeAesKeyslot(u32 keyslot) {
        return spl::FreeAesKeyslot(keyslot, this);
    }

    void DeprecatedService::GetAesKeyslotAvailableEvent(Out<CopiedHandle> out_hnd) {
        out_hnd.SetValue(spl::GetAesKeyslotAvailableEventHandle());
    }

    Result DeprecatedService::SetBootReason(BootReasonValue boot_reason) {
        return spl::SetBootReason(boot_reason);
    }

    Result DeprecatedService::GetBootReason(Out<BootReasonValue> out) {
        return spl::GetBootReason(out.GetPointer());
    }

}
