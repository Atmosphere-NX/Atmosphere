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

#include "usb_types.h"

#include<stdint.h>
#include<string.h>

namespace ams {

    namespace xusb {

        enum class TRBType : uint8_t {
            // transfer TRBs
            Normal = 1,
            SetupStage = 2,
            DataStage = 3,
            StatusStage = 4,
            Isoch = 5,
            Link = 6,
            EventData = 7,
            NoOp = 8,

            // event TRBs
            TransferEvent = 32,
            PortStatusChangeEvent = 34,
            SetupPacketEvent = 63, // vendor defined
        };

        union TRB;
    
        struct AbstractTRB {
            uint32_t field0;
            uint32_t field1;
            uint32_t field2;
            uint32_t field3;

            inline void Clear() {
                // leave cycle bit so hardware doesn't accidentlly consume this TRB
                this->field0 = 0;
                this->field1 = 0;
                this->field2 = 0;
                this->field3&= 1;
            }
        
            inline bool GetCycle() {
                return this->field3 & 1;
            }

            inline void SetCycle(bool cycle) {
                this->field3&= ~1;
                this->field3|= cycle;
            }

            inline TRBType GetType() {
                return (TRBType) ((this->field3 >> 10) & 0b111111);
            }

            inline void SetType(TRBType type) {
                this->field3&= ~(0b111111 << 10);
                this->field3|= (((uint32_t) type) & 0b111111) << 10;
            }
        } __attribute__((aligned(16)));
    
        struct LinkTRB : protected AbstractTRB {
            inline void Initialize(TRB *link) {
                AbstractTRB::SetType(TRBType::Link);
                this->field0 = (uint32_t) link;
            }

            inline void SetToggleCycle(bool tc) {
                this->field3&= ~2;
                this->field3|= tc << 1;
            }

            inline void SetChain(bool chain) {
                this->field3&= ~0x10;
                this->field3|= chain << 4;
            }
        } __attribute__((aligned(16)));
    
        struct TransferTRB {
            uint32_t data_ptr_lo;
            uint32_t data_ptr_hi;
        
            uint32_t transfer_length:17;
            uint32_t td_size:5;
            uint32_t interrupter_target:10;

            uint32_t :0;
            uint32_t cycle:1;
            uint32_t evaluate_next_trb:1;
            uint32_t interrupt_on_short_packet:1;
            uint32_t no_snoop:1;
            uint32_t chain_bit:1;
            uint32_t interrupt_on_completion:1;
            uint32_t immediate_data:1;
            uint32_t :2; // reserved
            uint32_t block_event_interrupt:1;
            uint32_t trb_type:6;
            uint32_t dir:1;
            uint32_t :15;

            inline void InitializeNoOp() {
                this->trb_type = (uint32_t) TRBType::NoOp;
            }

            inline void InitializeNormal(void *data, size_t length) {
                this->data_ptr_lo = (uint32_t) data;
                this->transfer_length = length;
                this->trb_type = (uint32_t) TRBType::Normal;
            }

            inline void InitializeDataStage(bool dir, void *data, size_t length) {
                this->data_ptr_lo = (uint32_t) data;
                this->transfer_length = length;
                this->trb_type = (uint32_t) TRBType::DataStage;
                this->dir = dir;
            }

            // see Table 4-7 on page 213 of XHCI spec
            inline void InitializeStatusStage(bool dir) {
                this->trb_type = (uint32_t) TRBType::StatusStage;
                this->dir = dir;
            }
        } __attribute__((aligned(16)));

        struct TransferEventTRB {
            uint32_t trb_ptr_lo;
            uint32_t trb_ptr_hi;
            uint32_t trb_transfer_length:24;
            uint32_t completion_code:8;
            uint32_t cycle:1;
            uint32_t :1;
            uint32_t ed:1;
            uint32_t :7;
            uint32_t trb_type:6;
            uint32_t ep_id:5;

            inline TRB *GetSourceTRB() {
                return (TRB*) trb_ptr_lo;
            }
        } __attribute__((aligned(16)));
    
        struct SetupPacketEventTRB {
            usb::SetupPacket packet;
            uint16_t seq_num;
            uint16_t field2;
            uint32_t field3;
        
            inline bool GetEventDataFlag() {
                return (field3 & 0b100) != 0;
            }
        } __attribute__((aligned(16)));
    
        union TRB {
            public:
            AbstractTRB abstract;
            LinkTRB link;
            TransferTRB transfer;

            TransferEventTRB transfer_event;
            SetupPacketEventTRB setup_packet_event;
        } __attribute__((aligned(16)));

        static_assert(sizeof(TRB) == 16, "TRB should be 16 bytes");
        
    } // namespace xusb

} // namespace ams
