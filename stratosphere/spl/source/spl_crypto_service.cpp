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

#include "spl_crypto_service.hpp"

Result CryptoService::GenerateAesKek(Out<AccessKey> out_access_key, KeySource key_source, u32 generation, u32 option) {
    return this->GetSecureMonitorWrapper()->GenerateAesKek(out_access_key.GetPointer(), key_source, generation, option);
}

Result CryptoService::LoadAesKey(u32 keyslot, AccessKey access_key, KeySource key_source) {
    return this->GetSecureMonitorWrapper()->LoadAesKey(keyslot, this, access_key, key_source);
}

Result CryptoService::GenerateAesKey(Out<AesKey> out_key, AccessKey access_key, KeySource key_source) {
    return this->GetSecureMonitorWrapper()->GenerateAesKey(out_key.GetPointer(), access_key, key_source);
}

Result CryptoService::DecryptAesKey(Out<AesKey> out_key, KeySource key_source, u32 generation, u32 option) {
    return this->GetSecureMonitorWrapper()->DecryptAesKey(out_key.GetPointer(), key_source, generation, option);
}

Result CryptoService::CryptAesCtr(OutBuffer<u8, BufferType_Type1> out_buf, u32 keyslot, InBuffer<u8, BufferType_Type1> in_buf, IvCtr iv_ctr) {
    return this->GetSecureMonitorWrapper()->CryptAesCtr(out_buf.buffer, out_buf.num_elements, keyslot, this, in_buf.buffer, in_buf.num_elements, iv_ctr);
}

Result CryptoService::ComputeCmac(Out<Cmac> out_cmac, u32 keyslot, InPointer<u8> in_buf) {
    return this->GetSecureMonitorWrapper()->ComputeCmac(out_cmac.GetPointer(), keyslot, this, in_buf.pointer, in_buf.num_elements);
}

Result CryptoService::AllocateAesKeyslot(Out<u32> out_keyslot) {
    return this->GetSecureMonitorWrapper()->AllocateAesKeyslot(out_keyslot.GetPointer(), this);
}

Result CryptoService::FreeAesKeyslot(u32 keyslot) {
    return this->GetSecureMonitorWrapper()->FreeAesKeyslot(keyslot, this);
}

void CryptoService::GetAesKeyslotAvailableEvent(Out<CopiedHandle> out_hnd) {
    out_hnd.SetValue(this->GetSecureMonitorWrapper()->GetAesKeyslotAvailableEventHandle());
}
