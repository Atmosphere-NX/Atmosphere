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
 
#include <switch.h>
#include "fatal_types.hpp"
#include "fatal_config.hpp"

static FatalConfig g_fatal_config = {};

static IEvent *g_fatal_settings_event = nullptr;

FatalConfig *GetFatalConfig() {
    return &g_fatal_config;
}

static void UpdateLanguageCode() {
    setGetLanguageCode(&GetFatalConfig()->language_code);
}

IEvent *GetFatalSettingsEvent() {
    if (g_fatal_settings_event == nullptr) {
        Event evt;
        if (R_FAILED(setsysBindFatalDirtyFlagEvent(&evt))) {
            std::abort();
        }
        g_fatal_settings_event = LoadReadOnlySystemEvent(evt.revent, [](u64 timeout) {
            u64 flags_0, flags_1;
            if (R_SUCCEEDED(setsysGetFatalDirtyFlags(&flags_0, &flags_1)) && (flags_0 & 1)) { 
                UpdateLanguageCode();
            }
            return ResultSuccess;
        }, true);
    }
    
    return g_fatal_settings_event;
}

static void SetupConfigLanguages() {
    FatalConfig *config = GetFatalConfig();
    
    /* Defaults. */
    config->error_msg   = u8"Error Code: 2%03d-%04d (0x%x)\n";
    
    if (config->quest_flag) {
        config->error_desc = u8"Please call 1-800-875-1852 for service.\n";
    } else {
        config->error_desc = u8"An error has occured.\n\n"
                             u8"Please press the POWER Button to restart the console normally, or a VOL button\n"
                             u8"to reboot to a payload (or RCM, if none is present). If you are unable to\n"
                             u8"restart the console, hold the POWER Button for 12 seconds to turn the console off.\n\n"
                             u8"If the problem persists, refer to the Nintendo Support Website.\n"
                             u8"support.nintendo.com/switch/error\n";
    }
    
    /* TODO: Try to load dynamically. */
    /* FsStorage message_storage; */
    /* TODO: if (R_SUCCEEDED(fsOpenDataStorageByDataId(0x010000000000081D, "fatal_msg"))) { ... } */
}

void InitializeFatalConfig() {
    FatalConfig *config = GetFatalConfig();
    
    memset(config, 0, sizeof(*config));
    setsysGetSerialNumber(config->serial_number);
    setsysGetFirmwareVersion(&config->firmware_version);
    UpdateLanguageCode();
    
    setsysGetSettingsItemValue("fatal", "transition_to_fatal", &config->transition_to_fatal, sizeof(config->transition_to_fatal));
    setsysGetSettingsItemValue("fatal", "show_extra_info", &config->show_extra_info, sizeof(config->show_extra_info));
    setsysGetSettingsItemValue("fatal", "quest_reboot_interval_second", &config->quest_reboot_interval_second, sizeof(config->quest_reboot_interval_second));
    
    setsysGetFlag(SetSysFlag_Quest, &config->quest_flag);
    
    config->is_auto_reboot_enabled = R_SUCCEEDED(setsysGetSettingsItemValue("atmosphere", "fatal_auto_reboot_interval", &config->fatal_auto_reboot_interval, sizeof(config->fatal_auto_reboot_interval)));
    config->is_auto_reboot_enabled &= (config->fatal_auto_reboot_interval != 0);

    SetupConfigLanguages();
}
