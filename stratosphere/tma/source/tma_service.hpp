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

#include "tma_conn_service_ids.hpp"
#include "tma_conn_packet.hpp"
#include "tma_task.hpp"

class TmaServiceManager;

class TmaService {
    protected:
        TmaServiceManager *manager;
        const char *service_name;
        const TmaServiceId id;
    protected:
        u32 GetNextTaskId();
    public:
        TmaService(TmaServiceManager *m, const char *n) : manager(m), service_name(n), id(static_cast<TmaServiceId>(HashServiceName(this->service_name))) { }
        virtual ~TmaService() { }
        
        TmaServiceId GetServiceId() const { return this->id; }
        
        virtual TmaTask *NewTask(TmaPacket *packet) = 0;
        virtual void OnSleep();
        virtual void OnWake();
};
