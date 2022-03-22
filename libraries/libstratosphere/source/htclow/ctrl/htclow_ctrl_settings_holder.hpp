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

namespace ams::htclow::ctrl {

    class SettingsHolder {
        private:
            char m_hardware_type[0x40]{};
            char m_target_name[0x40]{};
            char m_serial_number[0x40]{};
            char m_firmware_version[0x40]{};
        public:
            constexpr SettingsHolder() = default;

            void LoadSettings();

            const char *GetSpec() { return "NX"; }
            const char *GetHardwareType() { return m_hardware_type; }
            const char *GetTargetName() { return m_target_name; }
            const char *GetSerialNumber() { return m_serial_number; }
            const char *GetFirmwareVersion() { return m_firmware_version; }
    };

}
