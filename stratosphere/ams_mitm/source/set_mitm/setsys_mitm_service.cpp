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

#include <mutex>
#include <algorithm>
#include <switch.h>
#include "setsys_mitm_service.hpp"
#include "setsys_firmware_version.hpp"
#include "setsys_settings_items.hpp"

void SetSysMitmService::PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx) {
    /* No commands need postprocessing. */
}

Result SetSysMitmService::GetFirmwareVersion(OutPointerWithServerSize<SetSysFirmwareVersion, 0x1> out) {
    /* Get firmware version from manager. */
    R_TRY(VersionManager::GetFirmwareVersion(this->title_id, out.pointer));

    /* GetFirmwareVersion sanitizes these fields. */
    out.pointer->revision_major = 0;
    out.pointer->revision_minor = 0;
    return ResultSuccess;
}

Result SetSysMitmService::GetFirmwareVersion2(OutPointerWithServerSize<SetSysFirmwareVersion, 0x1> out) {
    return VersionManager::GetFirmwareVersion(this->title_id, out.pointer);
}

Result SetSysMitmService::GetSettingsItemValueSize(Out<u64> out_size, InPointer<char> in_name, InPointer<char> in_key) {
    char name[SET_MAX_NAME_SIZE] = {0};
    char key[SET_MAX_NAME_SIZE] = {0};

    /* Validate name and key. */
    R_TRY(SettingsItemManager::ValidateName(in_name.pointer));
    R_TRY(SettingsItemManager::ValidateKey(in_key.pointer));

    if (in_name.num_elements < SET_MAX_NAME_SIZE) {
        strncpy(name, in_name.pointer, in_name.num_elements);
    } else {
        strncpy(name, in_name.pointer, SET_MAX_NAME_SIZE-1);
    }

    if (in_key.num_elements < SET_MAX_NAME_SIZE) {
        strncpy(key, in_key.pointer, in_key.num_elements);
    } else {
        strncpy(key, in_key.pointer, SET_MAX_NAME_SIZE-1);
    }

    /* Try to get override setting, fall back to real setting. */
    if (R_FAILED(SettingsItemManager::GetValueSize(name, key, out_size.GetPointer()))) {
        R_TRY(setsysGetSettingsItemValueSize(name, key, out_size.GetPointer()));
    }

    return ResultSuccess;
}

Result SetSysMitmService::GetSettingsItemValue(Out<u64> out_size, OutBuffer<u8> out_value, InPointer<char> in_name, InPointer<char> in_key) {
    char name[SET_MAX_NAME_SIZE] = {0};
    char key[SET_MAX_NAME_SIZE] = {0};

    /* Validate name and key. */
    R_TRY(SettingsItemManager::ValidateName(in_name.pointer));
    R_TRY(SettingsItemManager::ValidateKey(in_key.pointer));

    if (out_value.buffer == nullptr) {
        return ResultSettingsItemValueBufferNull;
    }

    if (in_name.num_elements < SET_MAX_NAME_SIZE) {
        strncpy(name, in_name.pointer, in_name.num_elements);
    } else {
        strncpy(name, in_name.pointer, SET_MAX_NAME_SIZE-1);
    }

    if (in_key.num_elements < SET_MAX_NAME_SIZE) {
        strncpy(key, in_key.pointer, in_key.num_elements);
    } else {
        strncpy(key, in_key.pointer, SET_MAX_NAME_SIZE-1);
    }

    /* Try to get override setting, fall back to real setting. */
    if (R_FAILED(SettingsItemManager::GetValue(name, key, out_value.buffer, out_value.num_elements, out_size.GetPointer()))) {
        R_TRY(setsysGetSettingsItemValueFwd(this->forward_service.get(), name, key, out_value.buffer, out_value.num_elements, out_size.GetPointer()));
    }

    return ResultSuccess;
}

Result SetSysMitmService::GetEdid(OutPointerWithServerSize<SetSysEdid, 0x1> out) {
    return setsysGetEdidFwd(this->forward_service.get(), out.pointer);
}
