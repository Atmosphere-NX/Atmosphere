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

#include "spl_fs_service.hpp"

Result FsService::ImportLotusKey(InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option) {
    return this->GetSecureMonitorWrapper()->ImportLotusKey(src.pointer, src.num_elements, access_key, key_source, option);
}

Result FsService::DecryptLotusMessage(Out<u32> out_size, OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> mod, InPointer<u8> label_digest) {
    return this->GetSecureMonitorWrapper()->DecryptLotusMessage(out_size.GetPointer(), out.pointer, out.num_elements, base.pointer, base.num_elements, mod.pointer, mod.num_elements, label_digest.pointer, label_digest.num_elements);
}

Result FsService::GenerateSpecificAesKey(Out<AesKey> out_key, KeySource key_source, u32 generation, u32 which) {
    return this->GetSecureMonitorWrapper()->GenerateSpecificAesKey(out_key.GetPointer(), key_source, generation, which);
}

Result FsService::LoadTitleKey(u32 keyslot, AccessKey access_key) {
    return this->GetSecureMonitorWrapper()->LoadTitleKey(keyslot, this, access_key);
}

Result FsService::GetPackage2Hash(OutPointerWithClientSize<u8> dst) {
    return this->GetSecureMonitorWrapper()->GetPackage2Hash(dst.pointer, dst.num_elements);
}
