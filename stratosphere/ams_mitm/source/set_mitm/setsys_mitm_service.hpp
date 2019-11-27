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
#pragma once
#include <stratosphere.hpp>

namespace ams::mitm::settings {

    class SetSysMitmService  : public sf::IMitmServiceObject {
        private:
            enum class CommandId {
                GetFirmwareVersion       = 3,
                GetFirmwareVersion2      = 4,

                GetSettingsItemValueSize = 37,
                GetSettingsItemValue     = 38,
            };
        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                /* We will mitm:
                 * - everything, because we want to intercept all settings requests.
                 */
                return true;
            }
        public:
            SF_MITM_SERVICE_OBJECT_CTOR(SetSysMitmService) { /* ... */ }
        protected:
            Result GetFirmwareVersion(sf::Out<ams::settings::FirmwareVersion> out);
            Result GetFirmwareVersion2(sf::Out<ams::settings::FirmwareVersion> out);
            Result GetSettingsItemValueSize(sf::Out<u64> out_size, const ams::settings::fwdbg::SettingsName &name, const ams::settings::fwdbg::SettingsItemKey &key);
            Result GetSettingsItemValue(sf::Out<u64> out_size, const sf::OutBuffer &out, const ams::settings::fwdbg::SettingsName &name, const ams::settings::fwdbg::SettingsItemKey &key);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetFirmwareVersion),
                MAKE_SERVICE_COMMAND_META(GetFirmwareVersion2),
                MAKE_SERVICE_COMMAND_META(GetSettingsItemValueSize),
                MAKE_SERVICE_COMMAND_META(GetSettingsItemValue),
            };
    };

}
