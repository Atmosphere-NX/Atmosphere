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
#include "spl_es_service.hpp"

namespace ams::spl {

    Result EsService::LoadEsDeviceKeyDeprecated(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option) {
        return impl::LoadEsDeviceKey(src.GetPointer(), src.GetSize(), access_key, key_source, option);
    }

    Result EsService::LoadEsDeviceKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source) {
        return impl::LoadEsDeviceKey(src.GetPointer(), src.GetSize(), access_key, key_source, static_cast<u32>(smc::DeviceUniqueDataMode::DecryptAndStoreEsDeviceKey));
    }

    Result EsService::PrepareEsTitleKey(sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest, u32 generation) {
        return impl::PrepareEsTitleKey(out_access_key.GetPointer(), base.GetPointer(), base.GetSize(), mod.GetPointer(), mod.GetSize(), label_digest.GetPointer(), label_digest.GetSize(), generation);
    }

    Result EsService::PrepareCommonEsTitleKey(sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation) {
        return impl::PrepareCommonEsTitleKey(out_access_key.GetPointer(), key_source, generation);
    }

    Result EsService::DecryptAndStoreDrmDeviceCertKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source) {
        return impl::DecryptAndStoreDrmDeviceCertKey(src.GetPointer(), src.GetSize(), access_key, key_source);
    }

    Result EsService::ModularExponentiateWithDrmDeviceCertKey(const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod) {
        return impl::ModularExponentiateWithDrmDeviceCertKey(out.GetPointer(), out.GetSize(), base.GetPointer(), base.GetSize(), mod.GetPointer(), mod.GetSize());
    }

    Result EsService::PrepareEsArchiveKey(sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest, u32 generation) {
        return impl::PrepareEsArchiveKey(out_access_key.GetPointer(), base.GetPointer(), base.GetSize(), mod.GetPointer(), mod.GetSize(), label_digest.GetPointer(), label_digest.GetSize(), generation);
    }

    Result EsService::LoadPreparedAesKey(s32 keyslot, AccessKey access_key) {
        return impl::LoadPreparedAesKey(keyslot, this, access_key);
    }

}
