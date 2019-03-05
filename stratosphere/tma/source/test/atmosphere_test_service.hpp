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

#include "../tma_conn_service_ids.hpp"
#include "../tma_service.hpp"

class AtmosphereTestService : public TmaService {
    public:
        AtmosphereTestService(TmaServiceManager *m) : TmaService(m, "AtmosphereTestService") { }
        virtual ~AtmosphereTestService() { }
        
        virtual TmaTask *NewTask(TmaPacket *packet) override;
};