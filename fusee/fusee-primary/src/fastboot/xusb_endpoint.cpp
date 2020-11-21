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

#include "xusb_endpoint.h"

#include "xusb.h"
#include "xusb_control.h"

extern "C" {
#include "../lib/log.h"
}

#include<string.h>

namespace ams {

    namespace xusb {

        void EndpointContext::InitializeForControl() {
            memset((void*) this, 0, sizeof(*this));

            this->ep_state = 1;
            this->cerr = 3;
            this->ep_type = 4;
            this->max_packet_size = 64;
            this->dcs = 1;
            this->average_trb_length = 8;
            this->cec = 3;
            this->seqnum = 0;
        }

        void EndpointContext::Initialize(const usb::EndpointDescriptor &desc) {
            memset((void*) this, 0, sizeof(*this));

            this->ep_state = 1;

            if (desc.GetEndpointType() != usb::EndpointType::Control && (desc.bEndpointAddress & 0x80)) {
                /* In endpoint types are offset by 4. */
                this->ep_type = (uint8_t) desc.GetEndpointType() + 4;
            } else {
                this->ep_type = (uint8_t) desc.GetEndpointType();
            }
        
            this->cerr = 3;
            this->max_packet_size = desc.wMaxPacketSize;
            this->max_burst_size = 0;

            this->cec = 3;
        }

        void EndpointAccessor::RingDoorbell() const {
            t210::T_XUSB_DEV_XHCI.DB()
                .TARGET.Set(this->no)
                .Commit();
        }

        void EndpointAccessor::RingDoorbell(uint16_t stream_id) const {
            t210::T_XUSB_DEV_XHCI.DB()
                .STREAMID.Set(stream_id)
                .TARGET.Set(this->no)
                .Commit();
        }

        void EndpointAccessor::InitializeForControl(TRB *ring, size_t ring_size) const {
            /* Initialize hardware endpoint context for control endpont. */
            this->GetContext().InitializeForControl();

            /* Initialize transfer ring. */
            this->GetRingManager().Initialize(this->GetContext(), ring, ring_size);
        }

        void EndpointAccessor::Initialize(const usb::EndpointDescriptor &desc, TRB *ring, size_t ring_size) const {
            /* Initialize hardawre endpoint context for regular endpoint. */
            this->GetContext().Initialize(desc);

            /* Initialize transfer ring. */
            this->GetRingManager().Initialize(this->GetContext(), ring, ring_size);

            this->Reload();
        
            t210::T_XUSB_DEV_XHCI.EP_PAUSE()[this->no].Set(0).Commit();
            t210::T_XUSB_DEV_XHCI.EP_HALT()[this->no].Set(0).Commit();
        }

        void EndpointAccessor::Disable() const {
            this->GetContext().ep_state = 0;
            this->Reload();
            this->ClearPause();
            this->ClearHalt();

            auto stopped = t210::T_XUSB_DEV_XHCI.EP_STOPPED();
            if (stopped[this->no]) {
                stopped[this->no].Clear().Commit();
            }
        }
    
        Result EndpointAccessor::TransferNormal(void *data, size_t size, TRB **trb_out) const {
            TRBBorrow trb;
            
            R_TRY(this->EnqueueTRB(&trb));
            
            trb->transfer.InitializeNormal(data, size);
            trb->transfer.interrupt_on_completion = true;
            trb->transfer.interrupt_on_short_packet = true;
            trb.Release();

            this->RingDoorbell();

            *trb_out = &*trb;

            return ResultSuccess();
        }
    
        void EndpointAccessor::Halt() const {
            auto ep_halt = t210::T_XUSB_DEV_XHCI.EP_HALT();
            if (ep_halt[this->no]) {
                return;
            }

            (ep_halt[this->no] = 1).Commit();

            /* BootROM polls EP_HALT register, but TRM says to poll STCHG instead. */
            while (!t210::T_XUSB_DEV_XHCI.EP_STCHG()[this->no]) {
            }
                
            t210::T_XUSB_DEV_XHCI.EP_STCHG()[this->no].Clear().Commit();
        }

        void EndpointAccessor::ClearHalt() const {
            auto ep_halt = t210::T_XUSB_DEV_XHCI.EP_HALT();
            if (!ep_halt[this->no]) {
                return;
            }
                
            (ep_halt[this->no] = 0).Commit();

            /* Bootrom polls EP_HALT register, but TRM says to poll STCHG instead. */
            while (!t210::T_XUSB_DEV_XHCI.EP_STCHG()[this->no]) {
            }
                
            t210::T_XUSB_DEV_XHCI.EP_STCHG()[this->no].Clear().Commit();
        }

        void EndpointAccessor::Pause() const {
            auto ep_pause = t210::T_XUSB_DEV_XHCI.EP_PAUSE();
            if (ep_pause[this->no]) {
                return;
            }

            (ep_pause[this->no] = 1).Commit();

            /* Bootrom polls EP_PAUSE register, but TRM says to poll STCHG instead. */
            while (!t210::T_XUSB_DEV_XHCI.EP_STCHG()[this->no]) {
            }
                
            t210::T_XUSB_DEV_XHCI.EP_STCHG()[this->no].Clear().Commit();
        }

        void EndpointAccessor::ClearPause() const {
            auto ep_pause = t210::T_XUSB_DEV_XHCI.EP_PAUSE();
            if (!ep_pause[this->no]) {
                return;
            }
                
            (ep_pause[this->no] = 0).Commit();

            /* Bootrom polls EP_PAUSE register, but TRM says to poll STCHG instead. */
            while (!t210::T_XUSB_DEV_XHCI.EP_STCHG()[this->no]) {
            }
                
            t210::T_XUSB_DEV_XHCI.EP_STCHG()[this->no].Clear().Commit();
        }

        void EndpointAccessor::Reload() const {
            auto ep_reload = t210::T_XUSB_DEV_XHCI.EP_RELOAD();
            ep_reload[this->no] = 1;
            ep_reload.Commit();

            while (t210::T_XUSB_DEV_XHCI.EP_RELOAD()[this->no]) {
            }
        }
    
        TRBBorrow EndpointAccessor::AcceptCompletedTRB(TransferEventTRB *trb) const {
            return TRBBorrow(&this->GetRingManager(), trb->GetSourceTRB());
        }

        void EndpointAccessor::ResetAndReloadTransferRing() const {
            this->GetRingManager().Reset(this->GetContext());
            this->Reload();
        }
    
        void EndpointAccessor::DumpTransferRingForDebug() const {
            this->GetRingManager().DumpTransferRingForDebug();
        }
    
        TRBBorrow::TRBBorrow(TRBBorrow &&other) :
            ring(other.ring),
            trb(other.trb),
            state(other.state),
            cycle_state(other.cycle_state) {
            other.state = State::Empty;
        }

        TRBBorrow &TRBBorrow::operator=(TRBBorrow &&other) {
            this->Release();

            this->ring = other.ring;
            this->trb = other.trb;
            this->state = other.state;
            this->cycle_state = other.cycle_state;

            other.state = State::Empty;

            return *this;
        }

        void TRBBorrow::Release() {
            if (this->state == State::BorrowForEnqueue) {
                this->trb->abstract.SetCycle(this->cycle_state);
                this->state = State::Empty;
            } else if (this->state == State::BorrowForRecycle) {
                this->ring->RecycleTRB(this->trb);
                this->state = State::Empty;
            }
        }
    
        void EndpointRingManager::Initialize(EndpointContext &ep_context, TRB *ring, size_t ring_size) {        
            this->ring = ring;
            this->ring_size = ring_size;

            this->Reset(ep_context);
        }

        void EndpointRingManager::Reset(EndpointContext &ep_context) {
            memset(this->ring, 0, sizeof(TRB) * this->ring_size);
            this->producer_cycle_state = true;
            this->enqueue = &this->ring[0];
            this->dequeue = &this->ring[0];

            ep_context.dcs = this->producer_cycle_state;
            ep_context.tr_dequeue_ptr = ((uint64_t) &this->ring[0]) >> 4;
        }
    
        size_t EndpointRingManager::GetFreeTRBCount() {
            /* Last TRB is reserved for link. */
            size_t adjusted_ring_size = (this->ring_size - 1);

            if (this->ring_full) {
                return 0;
            }
        
            return adjusted_ring_size - ((this->enqueue + adjusted_ring_size - this->dequeue) % adjusted_ring_size);
        }

        TRB *EndpointRingManager::EnqueueTRB(bool chain) {
            if (ring_full) {
                return nullptr;
            }
        
            TRB *trb = this->enqueue++;
        
            if (this->enqueue == this->ring + (this->ring_size - 1)) {
                this->enqueue->link.Initialize(&this->ring[0]);
                this->enqueue->link.SetToggleCycle(true);
                this->enqueue->link.SetChain(chain);
                this->enqueue->abstract.SetCycle(this->producer_cycle_state);

                this->enqueue = &this->ring[0];
                this->producer_cycle_state = !this->producer_cycle_state;
            }

            if (this->enqueue == this->dequeue) {
                this->ring_full = true;
            }
        
            trb->abstract.Clear();

            return trb;
        }

        void EndpointRingManager::RecycleTRB(TRB *trb) {
            if (trb + 1 == &this->ring[this->ring_size-1]) {
                this->dequeue = &this->ring[0];
            } else {
                this->dequeue = trb + 1;
            }

            this->ring_full = false;
        }

        void EndpointRingManager::DebugState() {
            ScreenLogLevel ll = (ScreenLogLevel) (SCREEN_LOG_LEVEL_DEBUG | SCREEN_LOG_LEVEL_NO_PREFIX);
            print(ll, "%8s", "address ");
            for (size_t i = 0; i < this->ring_size; i++) {
                TRB *ptr = &this->ring[i];
                print(ll, "%x", ((uint32_t) ptr >> 4) & 0xf);
            }
            print(ll, "\n%8s", "cycle ");
            for (size_t i = 0; i < this->ring_size; i++) {
                TRB *ptr = &this->ring[i];
                print(ll, "%d", ptr->abstract.GetCycle());
            }
            print(ll, "\n%8s", "deq/enq ");
            for (size_t i = 0; i < this->ring_size; i++) {
                TRB *ptr = &this->ring[i];
                if (this->dequeue == ptr && this->enqueue == ptr) {
                    print(ll, "!");
                } else if (this->dequeue == ptr) {
                    print(ll, "D");
                } else if (this->enqueue == ptr) {
                    print(ll, "E");
                } else if (ptr == &this->ring[this->ring_size-1]) {
                    print(ll, "L");
                } else {
                    print(ll, " ");
                }
            }
            print(ll, "\n%8s", "ioc ");
            for (size_t i = 0; i < this->ring_size; i++) {
                TRB *ptr = &this->ring[i];
                print(ll, "%d", ptr->transfer.interrupt_on_completion);
            }
            print(ll, "\n");
        }

        void EndpointRingManager::DumpTransferRingForDebug() {
            ScreenLogLevel ll = (ScreenLogLevel) (SCREEN_LOG_LEVEL_DEBUG | SCREEN_LOG_LEVEL_NO_PREFIX);
            print(ll, "  enq deq type  \n");
            for (TRB *ptr = &this->ring[0]; ptr < &this->ring[this->ring_size]; ptr++) {
                print(ll, "  %3s %3s  %3d  %08x %08x %08x %08x\n",
                      ptr == this->enqueue ? "[E]" : "",
                      ptr == this->dequeue ? "[D]" : "",
                      ptr->transfer.trb_type,
                      ptr->abstract.field0,
                      ptr->abstract.field1,
                      ptr->abstract.field2,
                      ptr->abstract.field3);
            }
            print(ll, "pcs: %d\n", this->producer_cycle_state);
            print(ll, "full: %s\n", this->ring_full ? "yes" : "no");
        }

        void Endpoints::ClearNonControlEndpoints() {
            memset(&contexts[2], 0, sizeof(contexts) - 2*sizeof(contexts[0]));
        }

        void Endpoints::SetEP0DeviceAddress(uint16_t addr) {
            this->contexts[0].device_address = addr;
        }
    
        Endpoints endpoints __attribute__((section(".dram")));
        
    } // namespace xusb

} // namespace ams
