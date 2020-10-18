/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

#include "xusb_trb.h"
#include "xusb_endpoint.h"

namespace ams {

    namespace xusb {

        namespace control {
        
            enum class DeviceState {
                Powered,
                Default,
                Address,
                Configured,
            };

            extern DeviceState state;

            Result SendData(void *data, size_t size);
            Result SendStatus(bool direction);
            
            void ProcessEP0TransferEvent(TransferEventTRB *event, TRBBorrow transfer);
            void ProcessSetupPacketEvent(SetupPacketEventTRB *trb);
            void ProcessPortStatusChangeEvent(AbstractTRB *trb);
        
        } // namespace control
    
    } // namespace xusb

} // namespace ams
