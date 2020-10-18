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

#include "xusb_event_ring.h"

#include "xusb.h"
#include "xusb_control.h"
#include "xusb_endpoint.h"

#include "../reg/reg_xusb_dev.h"

extern "C" {
#include "../lib/log.h"
#include "../utils.h"
}

#include<array>
#include<string.h>

namespace ams {

    namespace xusb {

        void EventRing::Initialize() {
            memset((void*) &storage[0], 0, sizeof(storage));

            if (((uintptr_t) &storage[0] & 0b1111) != 0) {
                fatal_error("xusb initialization failed: event ring storage is misaligned\n");
            }

            this->consumer_dequeue_ptr = &this->storage[0];
            this->consumer_cycle_state = true;
        
            t210::T_XUSB_DEV_XHCI
                .ERST0BALO(0).ADDRLO.Set((uint32_t) &this->storage[0]).Commit()
                .ERST0BAHI(0).ADDRHI.Set(0).Commit()
                .ERST1BALO(0).ADDRLO.Set((uint32_t) &this->storage[0x10]).Commit()
                .ERST1BAHI(0).ADDRHI.Set(0).Commit()
                .ERSTSZ(0)
                .ERST0SZ.Set(0x10)
                .ERST1SZ.Set(0x10)
                .Commit()
                .EREPLO(0)
                .ADDRLO.Set((uint32_t) &this->storage[0])
                .ECS.Set(this->consumer_cycle_state)
                .Commit()
                .EREPHI(0).ADDRHI.Set(0).Commit()
                .ERDPLO(0).ADDRLO.Set((uint32_t) &this->storage[0]).Commit()
                .ERDPHI(0).ADDRHI.Set(0).Commit();
        }

        void EventRing::Process() {
            auto st = t210::T_XUSB_DEV_XHCI.ST();
            if (st.IP) {
                /* Clear interrupt pending. */
                st.IP.Clear().Commit();
            }

            while (this->consumer_dequeue_ptr->abstract.GetCycle() == this->consumer_cycle_state) {
                switch(this->consumer_dequeue_ptr->abstract.GetType()) {
                case TRBType::SetupPacketEvent:
                    control::ProcessSetupPacketEvent(&this->consumer_dequeue_ptr->setup_packet_event);
                    break;
                case TRBType::PortStatusChangeEvent:
                    control::ProcessPortStatusChangeEvent(&this->consumer_dequeue_ptr->abstract);
                    break;
                case TRBType::TransferEvent: {
                    TransferEventTRB *te = &this->consumer_dequeue_ptr->transfer_event;
                    TRBBorrow completed_transfer = xusb::endpoints[te->ep_id].AcceptCompletedTRB(te);

                    if (te->ep_id == 0) {
                        xusb::control::ProcessEP0TransferEvent(te, std::move(completed_transfer));
                    } else {
                        xusb::GetCurrentGadget()->ProcessTransferEvent(te, std::move(completed_transfer));
                    }

                    break; }
                default:
                    print(SCREEN_LOG_LEVEL_WARNING, "got unrecognized TRB type on event ring: %d\n", (int) this->consumer_dequeue_ptr->abstract.GetType());
                    break;
                }

                if ((size_t) (this->consumer_dequeue_ptr - &this->storage[0]) == std::size(this->storage) - 1) {
                    this->consumer_dequeue_ptr = &this->storage[0];
                    this->consumer_cycle_state = !this->consumer_cycle_state;
                } else {
                    this->consumer_dequeue_ptr++;
                }

                /* Some situations cause the event ring to continue filling up while
                   we're processing it, such as if we are submitting transfers and
                   they are completing before we fully yield out to the main
                   loop. */
                t210::T_XUSB_DEV_XHCI.ERDPLO()
                    .ADDRLO.Set((uint32_t) this->consumer_dequeue_ptr)
                    .Commit();
            }

            t210::T_XUSB_DEV_XHCI.ERDPLO()
                .EHB.Clear()
                .ADDRLO.Set((uint32_t) this->consumer_dequeue_ptr)
                .Commit();
        }
    
    } // namespace xusb

} // namespace ams
