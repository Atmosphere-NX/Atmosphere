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
#include <stratosphere.hpp>

#include "settings_task.hpp"

void GetSettingTask::OnStart(TmaPacket *packet) {
    size_t length;
    packet->ReadString(this->name, sizeof(this->name), &length);
    packet->ReadString(this->item_key, sizeof(this->item_key), &length);
    
    if (R_SUCCEEDED(setsysGetSettingsItemValueSize(this->name, this->item_key, &this->value_size))) {
        if (this->value_size <= sizeof(this->value)) {
            if (R_SUCCEEDED(setsysGetSettingsItemValue(this->name, this->item_key, this->value, this->value_size))) {
                this->succeeded = true;
            }
        }
    }
}

void GetSettingTask::OnReceivePacket(TmaPacket *packet) {
    std::abort();
}

void GetSettingTask::OnSendPacket(TmaPacket *packet) {
    packet->Write<u8>((u8)this->succeeded);
    if (this->succeeded) {
        packet->Write<u32>((u32)this->value_size);
        packet->Write(this->value, this->value_size);
    }
    
    this->Complete();
}