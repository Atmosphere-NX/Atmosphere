/*
 * Copyright (c) 2018 Atmosphère-NX
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

static FatalConfig g_fatal_config;

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
            return 0;
        }, true);
    }
    
    return g_fatal_settings_event;
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
}
