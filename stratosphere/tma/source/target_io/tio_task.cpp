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

#include "tio_task.hpp"


void TIOFileReadTask::OnStart(TmaPacket *packet) {
    char path[FS_MAX_PATH];
    
    packet->ReadString(path, sizeof(path), nullptr);
    packet->Read<u64>(this->size_remaining);
    packet->Read<u64>(this->cur_offset);
    
    Result rc = 0;    
    if (strlen(path) == 0) {
        rc = 0x202;
    }
    
    if (R_SUCCEEDED(rc)) {
        u64 file_size;
        rc = dmntTargetIOFileGetSize(path, &file_size);
        if (R_SUCCEEDED(rc)) {
            if (file_size < this->cur_offset + this->size_remaining) {
                this->size_remaining = file_size - this->cur_offset;
            }
        }
    }
    
    rc = dmntTargetIOFileOpen(&this->handle, path, FS_OPEN_READ, DmntTIOCreateOption_OpenExisting);
    if (R_FAILED(rc)) {
        this->SendResult(rc);
        return;
    } else {
        auto packet = this->AllocateSendPacket();
        rc = this->ProcessPacket(packet);
        if (R_SUCCEEDED(rc)) {
            this->manager->SendPacket(packet);
            if (this->size_remaining) {
                this->SetNeedsPackets(true);
            } else {
                this->SendResult(rc);
            }
        } else {
            this->manager->FreePacket(packet);
            this->SendResult(rc);
        }
    }
}

void TIOFileReadTask::OnSendPacket(TmaPacket *packet) {
    Result rc = this->ProcessPacket(packet);
    
    if (this->size_remaining == 0 || R_FAILED(rc)) {
        this->SendResult(rc);
    }
}

void TIOFileReadTask::SendResult(Result rc) {
    dmntTargetIOFileClose(&this->handle);
    this->SetNeedsPackets(false);
    
    auto packet = this->AllocateSendPacket();
    packet->Write<Result>(rc);
    this->manager->SendPacket(packet);
    Complete();
}

Result TIOFileReadTask::ProcessPacket(TmaPacket *packet) {
    Result rc = 0x196002;
    
    size_t cur_read = static_cast<u32>((this->size_remaining > MaxDataSize) ? MaxDataSize : this->size_remaining);
    
    u8 *buf = new u8[cur_read];
    if (buf != nullptr) {
        size_t actual_read = 0;
        rc = dmntTargetIOFileRead(&this->handle, this->cur_offset, buf, cur_read, &actual_read);
        if (R_SUCCEEDED(rc)) {
            packet->Write<Result>(rc);
            packet->Write<u32>(actual_read);
            packet->Write(buf, actual_read);
            this->cur_offset += actual_read;
            this->size_remaining -= actual_read;
        }
        
        delete buf;
    }
    
    return rc;
}

void TIOFileWriteTask::OnStart(TmaPacket *packet) {
    char path[FS_MAX_PATH];
    
    packet->ReadString(path, sizeof(path), nullptr);
    packet->Read<u64>(this->size_remaining);
    packet->Read<u64>(this->cur_offset);
    
    Result rc = 0;
    if (strlen(path) == 0) {
        rc = 0x202;
    }
    
    if (R_SUCCEEDED(rc)) {
        u64 file_size;
        rc = dmntTargetIOFileGetSize(path, &file_size);
        if (R_SUCCEEDED(rc)) {
            if (file_size < this->cur_offset + this->size_remaining) {
                this->size_remaining = file_size - this->cur_offset;
            }
        }
    }
    
    rc = dmntTargetIOFileOpen(&this->handle, path, FS_OPEN_READ, DmntTIOCreateOption_OpenExisting);
    
    if (R_FAILED(rc)) {
        this->SendResult(rc);
    }
}

void TIOFileWriteTask::OnReceivePacket(TmaPacket *packet) {
    Result rc = this->ProcessPacket(packet);
    
    if (this->size_remaining == 0 || R_FAILED(rc)) {
        this->SendResult(rc);
    }
}

void TIOFileWriteTask::SendResult(Result rc) {
    dmntTargetIOFileClose(&this->handle);
    
    auto packet = this->AllocateSendPacket();
    packet->Write<Result>(rc);
    this->manager->SendPacket(packet);
    Complete();
}

Result TIOFileWriteTask::ProcessPacket(TmaPacket *packet) {
    Result rc = 0x196002;
    
    /* Note: N does not bounds check this. We do. */
    u32 cur_write = 0;
    packet->Read<u32>(cur_write);
    
    size_t actual_written = 0;
    if (cur_write < MaxDataSize) {
        if (cur_write > this->size_remaining) {
            cur_write = this->size_remaining;
        }
        rc = dmntTargetIOFileWrite(&this->handle, this->cur_offset, packet->GetCurrentBodyPtr(), cur_write, &actual_written);  
        if (R_SUCCEEDED(rc)) {
            this->size_remaining -= actual_written;
            this->cur_offset += actual_written;
        }
    }
        
    return rc;
}