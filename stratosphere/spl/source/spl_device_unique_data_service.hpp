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

    class DeviceUniqueDataService : public CryptoService {
        public:
            explicit DeviceUniqueDataService(SecureMonitorManager *manager) : CryptoService(manager) { /* ... */ }
        public:
            /* Actual commands. */
            Result DecryptDeviceUniqueDataDeprecated(const sf::OutPointerBuffer &dst, const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source, u32 option) {
                return m_manager.DecryptDeviceUniqueData(dst.GetPointer(), dst.GetSize(), src.GetPointer(), src.GetSize(), access_key, key_source, option);
            }

            Result DecryptDeviceUniqueData(const sf::OutPointerBuffer &dst, const sf::InPointerBuffer &src, AccessKey access_key, KeySource key_source) {
                return m_manager.DecryptDeviceUniqueData(dst.GetPointer(), dst.GetSize(), src.GetPointer(), src.GetSize(), access_key, key_source, static_cast<u32>(smc::DeviceUniqueDataMode::DecryptDeviceUniqueData));
            }
    };
    static_assert(spl::impl::IsIDeviceUniqueDataInterface<DeviceUniqueDataService>);

}
