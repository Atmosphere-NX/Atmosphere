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

#include "fatal_config.hpp"

namespace sts::fatal::srv {

    namespace {

        /* Global config. */
        FatalConfig g_config;

        /* Event creator. */
        IEvent *CreateFatalDirtyEvent() {
            Event evt;
            R_ASSERT(setsysBindFatalDirtyFlagEvent(&evt));
            return LoadReadOnlySystemEvent(evt.revent, [](u64 timeout) {
                u64 flags_0, flags_1;
                if (R_SUCCEEDED(setsysGetFatalDirtyFlags(&flags_0, &flags_1)) && (flags_0 & 1)) {
                    g_config.UpdateLanguageCode();
                }
                return ResultSuccess;
            }, true);
        }

        /* Global event. */
        IEvent *g_fatal_dirty_event = CreateFatalDirtyEvent();
    }

    FatalConfig::FatalConfig() {
        /* Clear this. */
        std::memset(this, 0, sizeof(*this));

        /* Get information from set. */
        setsysGetSerialNumber(this->serial_number);
        setsysGetFirmwareVersion(&this->firmware_version);
        setsysGetFlag(SetSysFlag_Quest, &this->quest_flag);
        this->UpdateLanguageCode();

        /* Read information from settings. */
        setsysGetSettingsItemValue("fatal", "transition_to_fatal", &this->transition_to_fatal, sizeof(this->transition_to_fatal));
        setsysGetSettingsItemValue("fatal", "show_extra_info", &this->show_extra_info, sizeof(this->show_extra_info));
        setsysGetSettingsItemValue("fatal", "quest_reboot_interval_second", &this->quest_reboot_interval_second, sizeof(this->quest_reboot_interval_second));

        /* Atmosphere extension for automatic reboot. */
        if (R_SUCCEEDED(setsysGetSettingsItemValue("atmosphere", "fatal_auto_reboot_interval", &this->fatal_auto_reboot_interval, sizeof(this->fatal_auto_reboot_interval)))) {
            this->fatal_auto_reboot_enabled = this->fatal_auto_reboot_interval != 0;
        }

        /* Setup messages. */
        {
            this->error_msg = u8"Error Code: 2%03d-%04d (0x%x)\n";

            this->error_desc = u8"An error has occured.\n\n"
                                 u8"Please press the POWER Button to restart the console normally, or a VOL button\n"
                                 u8"to reboot to a payload (or RCM, if none is present). If you are unable to\n"
                                 u8"restart the console, hold the POWER Button for 12 seconds to turn the console off.\n\n"
                                 u8"If the problem persists, refer to the Nintendo Support Website.\n"
                                 u8"support.nintendo.com/switch/error\n";

            /* If you're running Atmosphere on a quest unit for some reason, talk to me on discord. */
            this->quest_desc = u8"Please call 1-800-875-1852 for service.\n\n"
                                 u8"Also, please be aware that running Atmosphere on a Quest device is not fully\n"
                                 u8"supported. Perhaps try booting your device without Atmosphere before calling\n"
                                 u8"an official Nintendo service hotline. If you encounter further issues, please\n"
                                 u8"contact SciresM#0524 on Discord, or via some other means.\n";

            /* TODO: Try to load dynamically? */
            /* FsStorage message_storage; */
            /* TODO: if (R_SUCCEEDED(fsOpenDataStorageByDataId(0x010000000000081D, "fatal_msg"))) { ... } */
        }
    }

    IEvent *GetFatalDirtyEvent() {
        return g_fatal_dirty_event;
    }

    const FatalConfig &GetFatalConfig() {
        return g_config;
    }

}
