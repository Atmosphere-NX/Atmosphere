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

#include "spl_es_service.hpp"

Result EsService::ImportEsKey(InPointer<u8> src, AccessKey access_key, KeySource key_source, u32 option) {
    return this->GetSecureMonitorWrapper()->ImportEsKey(src.pointer, src.num_elements, access_key, key_source, option);
}

Result EsService::UnwrapTitleKey(Out<AccessKey> out_access_key, InPointer<u8> base, InPointer<u8> mod, InPointer<u8> label_digest, u32 generation) {
    return this->GetSecureMonitorWrapper()->UnwrapTitleKey(out_access_key.GetPointer(), base.pointer, base.num_elements, mod.pointer, mod.num_elements, label_digest.pointer, label_digest.num_elements, generation);
}

Result EsService::UnwrapCommonTitleKey(Out<AccessKey> out_access_key, KeySource key_source, u32 generation) {
    return this->GetSecureMonitorWrapper()->UnwrapCommonTitleKey(out_access_key.GetPointer(), key_source, generation);
}

Result EsService::ImportDrmKey(InPointer<u8> src, AccessKey access_key, KeySource key_source) {
    return this->GetSecureMonitorWrapper()->ImportDrmKey(src.pointer, src.num_elements, access_key, key_source);
}

Result EsService::DrmExpMod(OutPointerWithClientSize<u8> out, InPointer<u8> base, InPointer<u8> mod) {
    return this->GetSecureMonitorWrapper()->DrmExpMod(out.pointer, out.num_elements, base.pointer, base.num_elements, mod.pointer, mod.num_elements);
}

Result EsService::UnwrapElicenseKey(Out<AccessKey> out_access_key, InPointer<u8> base, InPointer<u8> mod, InPointer<u8> label_digest, u32 generation) {
    return this->GetSecureMonitorWrapper()->UnwrapElicenseKey(out_access_key.GetPointer(), base.pointer, base.num_elements, mod.pointer, mod.num_elements, label_digest.pointer, label_digest.num_elements, generation);
}

Result EsService::LoadElicenseKey(u32 keyslot, AccessKey access_key) {
    return this->GetSecureMonitorWrapper()->LoadElicenseKey(keyslot, this, access_key);
}
