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

#include "../tma_task.hpp"

class GetSettingTask : public TmaTask {
    private:
        char name[0x40] = {0};
        char item_key[0x40] = {0};
        u8 value[0x40] = {0};
        u64 value_size = 0;
        bool succeeded = false;
        
    public:
        GetSettingTask(TmaServiceManager *m) : TmaTask(m) { }
        virtual ~GetSettingTask() { }
        
        virtual void OnStart(TmaPacket *packet) override;
        virtual void OnReceivePacket(TmaPacket *packet) override;
        virtual void OnSendPacket(TmaPacket *packet) override;
};
