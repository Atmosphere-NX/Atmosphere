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

#include "tma_conn_result.hpp"
#include "tma_conn_packet.hpp"

class TmaUsbComms {
    private:
        static void UsbStateChangeThreadFunc(void *arg);
        static Result UsbXfer(UsbDsEndpoint *ep, size_t *out_xferd, void *buf, size_t size);
    public:
        static TmaConnResult Initialize();
        static TmaConnResult Finalize();
        static void CancelComms();
        static TmaConnResult ReceivePacket(TmaPacket *packet);
        static TmaConnResult SendPacket(TmaPacket *packet);
        
        static void SetStateChangeCallback(void (*callback)(void *, u32), void *arg);
};