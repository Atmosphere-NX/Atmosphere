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
 
#pragma once
#include <switch.h>
#include <stratosphere.hpp>

struct FatalConfig {
    char serial_number[0x18];
    SetSysFirmwareVersion firmware_version;
    u64 language_code;
    u64 quest_reboot_interval_second;
    bool transition_to_fatal;
    bool show_extra_info;
    bool quest_flag;
    const char *error_msg;
    const char *error_desc;
    const char *quest_desc;
};

IEvent *GetFatalSettingsEvent();
FatalConfig *GetFatalConfig();

void InitializeFatalConfig();