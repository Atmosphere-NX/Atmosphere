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

#include "tio_service.hpp"
#include "tio_task.hpp"

TmaTask *TIOService::NewTask(TmaPacket *packet) {
    TmaTask *new_task = nullptr;
    switch (packet->GetCommand()) {
        case TIOServiceCmd_FileRead:
            {
                new_task = new TIOFileReadTask(this->manager);
            }
            break;
        case TIOServiceCmd_FileWrite:
            {
                new_task = new TIOFileWriteTask(this->manager);
            }
            break;
        default:
            new_task = nullptr;
            break;
    }
    if (new_task != nullptr) {
        new_task->SetServiceId(this->GetServiceId());
        new_task->SetTaskId(packet->GetTaskId());
        new_task->OnStart(packet);
    }
    
    return new_task;
}
