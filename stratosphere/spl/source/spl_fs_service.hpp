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
#include <stratosphere.hpp>
#include "spl_crypto_service.hpp"

namespace ams::spl {

    class FsService : public CryptoService {
        public:
            explicit FsService(SecureMonitorManager *manager) : CryptoService(manager) { /* ... */ }
        public:
            /* Actual commands. */
            Result DecryptAndStoreGcKeyDeprecated(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option) {
                return m_manager.DecryptAndStoreGcKey(src.GetPointer(), src.GetSize(), access_key, key_source, option);
            }

            Result DecryptAndStoreGcKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source) {
                return m_manager.DecryptAndStoreGcKey(src.GetPointer(), src.GetSize(), access_key, key_source, static_cast<u32>(smc::DeviceUniqueDataMode::DecryptAndStoreGcKey));
            }

            Result DecryptGcMessage(sf::Out<u32> out_size, const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest) {
                return m_manager.DecryptGcMessage(out_size.GetPointer(), out.GetPointer(), out.GetSize(), base.GetPointer(), base.GetSize(), mod.GetPointer(), mod.GetSize(), label_digest.GetPointer(), label_digest.GetSize());
            }

            Result GenerateSpecificAesKey(sf::Out<AesKey> out_key, KeySource key_source, u32 generation, u32 which) {
                return m_manager.GenerateSpecificAesKey(out_key.GetPointer(), key_source, generation, which);
            }

            Result LoadPreparedAesKey(s32 keyslot, AccessKey access_key) {
                return m_manager.LoadPreparedAesKey(keyslot, this, access_key);
            }

            Result GetPackage2Hash(const sf::OutPointerBuffer &dst) {
                return m_manager.GetPackage2Hash(dst.GetPointer(), dst.GetSize());
            }
    };
    static_assert(spl::impl::IsIFsInterface<FsService>);

}
