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
#include "../reg/reg_xusb_dev.h"

#include<vapours/results/xusb_gadget_results.hpp>

#include<stddef.h>

namespace ams {

    namespace xusb {
    
        struct EndpointContext {
            uint32_t ep_state:3;
            uint32_t :5;
            uint32_t multi:2;
            uint32_t max_p_streams:5;
            uint32_t lsa:1;
            uint32_t interval:8;
            uint32_t :8;

            uint32_t :1;
            uint32_t cerr:2;
            uint32_t ep_type:3;
            uint32_t :1;
            uint32_t hid:1;
            uint32_t max_burst_size:8;
            uint32_t max_packet_size:16;

            uint64_t dcs:1;
            uint64_t :3;
            uint64_t tr_dequeue_ptr:60;

            uint32_t average_trb_length:16;
            uint32_t max_esit_payload:16;
    
            uint32_t event_data_transfer_length_accumulator:24;
            uint32_t :1;
            uint32_t ptd:1;
            uint32_t sxs:1;
            uint32_t seqnum:5;

            uint32_t cprog:8;
            uint32_t s_byte:7;
            uint32_t tp:2;
            uint32_t rec:1;
            uint32_t cec:2;
            uint32_t ced:1;
            uint32_t hsp:1;
            uint32_t rty:1;
            uint32_t std:1;
            uint32_t status:8;

            uint32_t data_offset:17;
            uint32_t :4;
            uint32_t lpa:1;
            uint32_t num_trb:5;
            uint32_t num_p:5;

            uint32_t scratchpad1;

            uint32_t scratchpad2;

            uint32_t cping:8;
            uint32_t sping:8;
            uint32_t tc:2;
            uint32_t ns:1;
            uint32_t ro:1;
            uint32_t tlm:1;
            uint32_t dlm:1;
            uint32_t hsp2:1;
            uint32_t rty2:1;
            uint32_t stop_reclaim_request:8;

            uint32_t device_address:8;
            uint32_t hub_address:8;
            uint32_t root_port_number:8;
            uint32_t slot_id:8;

            uint32_t routing_string:20;
            uint32_t speed:4;
            uint32_t lpu:1;
            uint32_t myy:1;
            uint32_t hub:1;
            uint32_t dci:5;

            uint32_t tt_hub_slot_id:8;
            uint32_t tt_port_num:8;
            uint32_t ssf:4;
            uint32_t sps:2;
            uint32_t interrupt_target:10;

            uint64_t frz:1;
            uint64_t end:1;
            uint64_t elm:1;
            uint64_t mrk:1;
            uint64_t endpoint_link:60;

            void InitializeForControl();
            void Initialize(const usb::EndpointDescriptor &desc);
        } __attribute__((aligned(64)));

        enum Dir {
            Out,
            In,
        };

        struct EpAddr {
            static inline EpAddr Decode(uint8_t addr) {
                return EpAddr {
                    .no = (uint8_t) (addr & 0b1111),
                    .direction = (addr >> 7) ? Dir::In : Dir::Out,
                };
            }
            
            uint8_t no;
            Dir direction;
        };

        class EndpointRingManager {
            public:
            void Initialize(EndpointContext &ep_context, TRB *ring, size_t ring_size);
            void Reset(EndpointContext &ep_context);
        
            inline size_t GetRingUsableCount() {
                return this->ring_size - 1;
            }
            size_t GetFreeTRBCount();

            void DumpTransferRingForDebug(); // long form
            void DebugState();
        
            TRB *EnqueueTRB(bool chain);
            void RecycleTRB(TRB *trb);
        
            inline bool GetProducerCycleState() {
                return this->producer_cycle_state;
            }

            private:
            TRB *ring = nullptr;
            size_t ring_size = 0;
            bool ring_full = false;
        
            bool producer_cycle_state = true;

            TRB *dequeue = nullptr;
            TRB *enqueue = nullptr;
        };

        /* TRB ownership:
           Initially, all TRBs are owned by the transfer ring manager.
           By calling EnqueueTRB(), you are borrowing a TRB from the ring.
           By calling Release() or allowing TRBBorrow to be destructed, the borrow is transferred to the hardware.
           When the hardware is finished with the TRB and interrupts for completion, the borrow is transferred back to the software.
           By calling Release() or allowing TRBRecycleHelper to be destructed, the TRB is recycled to the transfer ring manager.
        */

        class EndpointAccessor;
    
        class TRBBorrow {
            public:
            inline TRBBorrow() :
                state(State::Empty) {
            }

            private:
            friend class EndpointAccessor;
        
            inline TRBBorrow(EndpointRingManager *ring, TRB *trb) :
                ring(ring),
                trb(trb),
                state(State::BorrowForRecycle) {
            }

            inline TRBBorrow(EndpointRingManager *ring, TRB *trb, bool cycle_state) :
                ring(ring),
                trb(trb),
                state(State::BorrowForEnqueue),
                cycle_state(cycle_state) {
            }

            public:

            inline ~TRBBorrow() {
                Release();
            }

            TRBBorrow(TRBBorrow &) = delete;
            TRBBorrow(TRBBorrow &&);
            TRBBorrow &operator=(TRBBorrow &) = delete;
            TRBBorrow &operator=(TRBBorrow &&);
                
            void Release();

            inline TRB &operator*() {
                return *this->trb;
            }

            inline TRB *operator->() {
                return this->trb;
            }
                
            private:
                
            EndpointRingManager *ring;
            TRB *trb;

            enum class State {
                Empty,
                BorrowForEnqueue,
                BorrowForRecycle
            } state;
                
            bool cycle_state;
        };

        class Endpoints;
    
        class EndpointAccessor {
            public:
            constexpr inline EndpointAccessor(uint8_t ep_index, Endpoints &eps) :
                no(ep_index),
                endpoints(eps) {
            }

            void InitializeForControl(TRB *ring, size_t ring_size) const;
            void Initialize(const usb::EndpointDescriptor &desc, TRB *ring, size_t ring_size) const;
            void Disable() const;
        
            inline size_t GetRingUsableCount() const {
                return this->GetRingManager().GetRingUsableCount();
            }
            
            inline size_t GetFreeTRBCount() const {
                return this->GetRingManager().GetFreeTRBCount();
            }
            
            inline Result EnqueueTRB(TRBBorrow *borrow, bool chain=false) const {
                // it is important to call this before Enqueue
                bool cycle_state = this->GetRingManager().GetProducerCycleState();

                TRB *trb = this->GetRingManager().EnqueueTRB(chain);

                R_UNLESS(trb != nullptr, xusb::ResultTransferRingFull());
                
                *borrow = TRBBorrow(
                    &this->GetRingManager(),
                    trb,
                    cycle_state);

                return ResultSuccess();
            }

            Result TransferNormal(void *buffer, size_t size, TRB **trb_out) const;
            
            void Halt() const;
            void ClearHalt() const;

            void Pause() const;
            void ClearPause() const;
            
            void Reload() const;
            void RingDoorbell() const;
            void RingDoorbell(uint16_t stream_id) const;
            TRBBorrow AcceptCompletedTRB(TransferEventTRB *trb) const;
            
            inline uint8_t GetIndex() const {
                return this->no;
            }

            void ResetAndReloadTransferRing() const;
        
            void DumpTransferRingForDebug() const;

            private:
            inline EndpointContext &GetContext() const;
            inline EndpointRingManager &GetRingManager() const;
        
            void Initialize(TRB *ring, size_t ring_size);
            
            uint8_t no;
            Endpoints &endpoints;
        };
    
        class Endpoints {
            public:
            static const size_t NR_EPS = 32;

            private:
            friend class EndpointAccessor;
            // structure-of-arrays instead of array-of-structures
            // to improve alignment.
            EndpointRingManager rings[NR_EPS];
            EndpointContext contexts[NR_EPS];

            public:
            void ClearNonControlEndpoints();
            void SetEP0DeviceAddress(uint16_t addr);
        
            constexpr inline EndpointAccessor operator[](EpAddr addr) {
                return EndpointAccessor(addr.no * 2 + (addr.direction == Dir::In ? 1 : 0), *this);
            };

            constexpr inline EndpointAccessor operator[](int no) {
                return EndpointAccessor(no, *this);
            };

            inline uintptr_t GetContextsForHardware() {
                return (uintptr_t) contexts;
            }
        };

        inline EndpointContext &EndpointAccessor::GetContext() const {
            return this->endpoints.contexts[this->no];
        }
    
        inline EndpointRingManager &EndpointAccessor::GetRingManager() const {
            return this->endpoints.rings[this->no];
        }

    
        extern Endpoints endpoints;
    
        static_assert(sizeof(EndpointContext) == 64, "Endpoint context should be 64 bytes");
    
    } // namespace xusb

} // namespace ams
