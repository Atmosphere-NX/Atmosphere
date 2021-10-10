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
#include "spl_device_unique_data_service.hpp"

namespace ams::spl {

    class EsService : public DeviceUniqueDataService {
        public:
            explicit EsService(SecureMonitorManager *manager) : DeviceUniqueDataService(manager) { /* ... */ }
        public:
            /* Actual commands. */
            Result LoadEsDeviceKeyDeprecated(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option) {
                return m_manager.LoadEsDeviceKey(src.GetPointer(), src.GetSize(), access_key, key_source, option);
            }

            Result LoadEsDeviceKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source) {
                return m_manager.LoadEsDeviceKey(src.GetPointer(), src.GetSize(), access_key, key_source, static_cast<u32>(smc::DeviceUniqueDataMode::DecryptAndStoreEsDeviceKey));
            }

            Result PrepareEsTitleKey(sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest, u32 generation) {
                return m_manager.PrepareEsTitleKey(out_access_key.GetPointer(), base.GetPointer(), base.GetSize(), mod.GetPointer(), mod.GetSize(), label_digest.GetPointer(), label_digest.GetSize(), generation);
            }

            Result PrepareCommonEsTitleKey(sf::Out<AccessKey> out_access_key, KeySource key_source, u32 generation) {
                return m_manager.PrepareCommonEsTitleKey(out_access_key.GetPointer(), key_source, generation);
            }

            Result DecryptAndStoreDrmDeviceCertKey(const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source) {
                return m_manager.DecryptAndStoreDrmDeviceCertKey(src.GetPointer(), src.GetSize(), access_key, key_source);
            }

            Result ModularExponentiateWithDrmDeviceCertKey(const sf::OutPointerBuffer &out, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod) {
                return m_manager.ModularExponentiateWithDrmDeviceCertKey(out.GetPointer(), out.GetSize(), base.GetPointer(), base.GetSize(), mod.GetPointer(), mod.GetSize());
            }

            Result PrepareEsArchiveKey(sf::Out<AccessKey> out_access_key, const sf::InPointerBuffer &base, const sf::InPointerBuffer &mod, const sf::InPointerBuffer &label_digest, u32 generation) {
                return m_manager.PrepareEsArchiveKey(out_access_key.GetPointer(), base.GetPointer(), base.GetSize(), mod.GetPointer(), mod.GetSize(), label_digest.GetPointer(), label_digest.GetSize(), generation);
            }

            Result LoadPreparedAesKey(s32 keyslot, AccessKey access_key) {
                return m_manager.LoadPreparedAesKey(keyslot, this, access_key);
            }
    };
    static_assert(spl::impl::IsIEsInterface<EsService>);

}
