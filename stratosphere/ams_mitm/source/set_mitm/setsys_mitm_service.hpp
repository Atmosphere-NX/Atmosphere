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
#pragma once
#include <stratosphere.hpp>

namespace ams::mitm::settings {

    namespace {

        #define AMS_SETTINGS_SYSTEM_MITM_INTERFACE_INFO(C, H)                                                                                                                                                                   \
            AMS_SF_METHOD_INFO(C, H,  3, Result, GetFirmwareVersion,       (sf::Out<ams::settings::FirmwareVersion> out))                                                                                                       \
            AMS_SF_METHOD_INFO(C, H,  4, Result, GetFirmwareVersion2,      (sf::Out<ams::settings::FirmwareVersion> out))                                                                                                       \
            AMS_SF_METHOD_INFO(C, H, 37, Result, GetSettingsItemValueSize, (sf::Out<u64> out_size, const ams::settings::fwdbg::SettingsName &name, const ams::settings::fwdbg::SettingsItemKey &key))                           \
            AMS_SF_METHOD_INFO(C, H, 38, Result, GetSettingsItemValue,     (sf::Out<u64> out_size, const sf::OutBuffer &out, const ams::settings::fwdbg::SettingsName &name, const ams::settings::fwdbg::SettingsItemKey &key)) \
            AMS_SF_METHOD_INFO(C, H, 62, Result, GetDebugModeFlag,         (sf::Out<bool> out))

        AMS_SF_DEFINE_MITM_INTERFACE(ISetSysMitmInterface, AMS_SETTINGS_SYSTEM_MITM_INTERFACE_INFO)

    }

    class SetSysMitmService  : public sf::MitmServiceImplBase {
        public:
            using MitmServiceImplBase::MitmServiceImplBase;
        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                /* We will mitm:
                 * - everything, because we want to intercept all settings requests.
                 */
                return true;
            }
        public:
            Result GetFirmwareVersion(sf::Out<ams::settings::FirmwareVersion> out);
            Result GetFirmwareVersion2(sf::Out<ams::settings::FirmwareVersion> out);
            Result GetSettingsItemValueSize(sf::Out<u64> out_size, const ams::settings::fwdbg::SettingsName &name, const ams::settings::fwdbg::SettingsItemKey &key);
            Result GetSettingsItemValue(sf::Out<u64> out_size, const sf::OutBuffer &out, const ams::settings::fwdbg::SettingsName &name, const ams::settings::fwdbg::SettingsItemKey &key);
            Result GetDebugModeFlag(sf::Out<bool> out);
    };
    static_assert(IsISetSysMitmInterface<SetSysMitmService>);

}
