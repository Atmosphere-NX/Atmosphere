/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere.hpp>
#include "powctl_interrupt_event_handler.hpp"

namespace ams::powctl::impl::board::nintendo::nx {

    void ChargerInterruptEventHandler::SignalEvent(IDevice *device) {
        /* TODO */
        AMS_UNUSED(device);
        AMS_ABORT();
    }

    void BatteryInterruptEventHandler::SignalEvent(IDevice *device) {
        /* TODO */
        AMS_UNUSED(device);
        AMS_ABORT();
    }

}