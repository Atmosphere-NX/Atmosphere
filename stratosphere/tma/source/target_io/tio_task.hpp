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
#include "../tma_service_manager.hpp"
#include "../dmnt.h"

class TIOTask : public TmaTask {
    public:
        TIOTask(TmaServiceManager *m) : TmaTask(m) { }
        virtual ~TIOTask() { }
        
        virtual void SendResult(Result rc) {
            TmaPacket *packet = this->AllocateSendPacket();
            packet->Write<Result>(rc);
            this->manager->SendPacket(packet);
            this->Complete();
        }
        
        virtual void OnStart(TmaPacket *packet)  = 0;
        
        virtual void OnReceivePacket(TmaPacket *packet) override {
            this->Complete();
        }
        
        virtual void OnSendPacket(TmaPacket *packet) override {
            this->Complete();
        }
};

class TIOFileReadTask : public TIOTask {
    private:
        static constexpr size_t HeaderSize = sizeof(Result) + sizeof(u32);
        static constexpr size_t MaxDataSize = TmaPacket::MaxBodySize - HeaderSize;
    private:
        DmntFile handle = {0};
        u64 size_remaining = 0;
        u64 cur_offset = 0;
    public:
        TIOFileReadTask(TmaServiceManager *m) : TIOTask(m) { }
        virtual ~TIOFileReadTask() { }
        
        virtual void OnStart(TmaPacket *packet) override;
        virtual void OnSendPacket(TmaPacket *packet) override;
        virtual void SendResult(Result rc) override;
        
        Result ProcessPacket(TmaPacket *packet);
};

class TIOFileWriteTask : public TIOTask {
    private:
        static constexpr size_t HeaderSize = sizeof(u32);
        static constexpr size_t MaxDataSize = TmaPacket::MaxBodySize - HeaderSize;
    private:
        DmntFile handle = {0};
        u64 size_remaining = 0;
        u64 cur_offset = 0;
    public:
        TIOFileWriteTask(TmaServiceManager *m) : TIOTask(m) { }
        virtual ~TIOFileWriteTask() { }
        
        virtual void OnStart(TmaPacket *packet) override;
        virtual void OnReceivePacket(TmaPacket *packet) override;
        virtual void SendResult(Result rc) override;
        
        Result ProcessPacket(TmaPacket *packet);
};

