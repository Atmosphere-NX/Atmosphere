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
#include <stratosphere.hpp>
#include "htclow_ctrl_settings_holder.hpp"

namespace ams::htclow::ctrl {

    void SettingsHolder::LoadSettings() {
        /* Load configuration id. */
        {
            settings::factory::ConfigurationId1 cfg_id;
            settings::factory::GetConfigurationId1(std::addressof(cfg_id));

            if (cfg_id.str[0]) {
                util::Strlcpy(m_hardware_type, cfg_id.str, sizeof(m_hardware_type));
            } else {
                util::Strlcpy(m_hardware_type, "Unknown", sizeof(m_hardware_type));
            }
        }

        /* Load device name. */
        {
            char device_name[0x40];
            settings::fwdbg::GetSettingsItemValue(device_name, sizeof(device_name), "target_manager", "device_name");
            util::Strlcpy(m_target_name, device_name, sizeof(m_target_name));
        }

        /* Load serial number. */
        {
            settings::factory::SerialNumber sn;
            if (R_SUCCEEDED(settings::factory::GetSerialNumber(std::addressof(sn)))) {
                util::Strlcpy(m_serial_number, sn.str, sizeof(m_serial_number));
            } else {
                m_serial_number[0] = '\x00';
            }
        }

        /* Load firmware version. */
        {
            settings::system::FirmwareVersion fw_ver;
            settings::system::GetFirmwareVersion(std::addressof(fw_ver));
            util::Strlcpy(m_firmware_version, fw_ver.display_name, sizeof(m_firmware_version));
        }
    }

}
