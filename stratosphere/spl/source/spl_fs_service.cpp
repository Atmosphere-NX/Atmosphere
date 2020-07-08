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
#include "spl_fs_service.hpp"

namespace ams::spl {

    Result FsService::DecryptAndStoreGcKeyDeprecated(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option) {
        return impl::DecryptAndStoreGcKey(src.GetPointer(), src.GetSize(), access_key, key_source, option);
    }

    Result FsService::DecryptAndStoreGcKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source) {
        return impl::DecryptAndStoreGcKey(src.GetPointer(), src.GetSize(), access_key, key_source, static_cast<u32>(smc::DeviceUniqueDataMode::DecryptAndStoreGcKey));
    }

    Result FsService::DecryptGcMessage(sf::Out<u32> out_size, const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest) {
        return impl::DecryptGcMessage(out_size.GetPointer(), out.GetPointer(), out.GetSize(), base.GetPointer(), base.GetSize(), mod.GetPointer(), mod.GetSize(), label_digest.GetPointer(), label_digest.GetSize());
    }

    Result FsService::GenerateSpecificAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 which) {
        return impl::GenerateSpecificAesKey(out_key.GetPointer(), key_source, generation, which);
    }

    Result FsService::LoadPreparedAesKey(s32 keyslot, AccessKey access_key) {
        return impl::LoadPreparedAesKey(keyslot, this, access_key);
    }

    Result FsService::GetPackage2Hash(const sf::OutPointerBuffer &dst) {
        return impl::GetPackage2Hash(dst.GetPointer(), dst.GetSize());
    }

}
