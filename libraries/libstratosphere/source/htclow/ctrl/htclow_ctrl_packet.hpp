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
#pragma once
#include <stratosphere.hpp>
#include "../htclow_packet.hpp"

namespace ams::htclow::ctrl {

    enum HtcctrlPacketType : u16 {
        HtcctrlPacketType_ConnectFromHost       = 16,
        HtcctrlPacketType_ConnectFromTarget     = 17,
        HtcctrlPacketType_ReadyFromHost         = 18,
        HtcctrlPacketType_ReadyFromTarget       = 19,
        HtcctrlPacketType_SuspendFromHost       = 20,
        HtcctrlPacketType_SuspendFromTarget     = 21,
        HtcctrlPacketType_ResumeFromHost        = 22,
        HtcctrlPacketType_ResumeFromTarget      = 23,
        HtcctrlPacketType_DisconnectFromHost    = 24,
        HtcctrlPacketType_DisconnectFromTarget  = 25,
        HtcctrlPacketType_BeaconQuery           = 28,
        HtcctrlPacketType_BeaconResponse        = 29,
        HtcctrlPacketType_InformationFromTarget = 33,
    };

    static constexpr inline u32 HtcctrlSignature = 0x78825637;

    struct HtcctrlPacketHeader {
        u32 signature;
        u32 sequence_id;
        u32 reserved;
        u32 body_size;
        s16 version;
        HtcctrlPacketType packet_type;
        impl::ChannelInternalType channel;
        u64 share;
    };
    static_assert(util::is_pod<HtcctrlPacketHeader>::value);
    static_assert(sizeof(HtcctrlPacketHeader) == 0x20);

    static constexpr inline size_t HtcctrlPacketBodySizeMax = 0x1000;

    struct HtcctrlPacketBody {
        u8 data[HtcctrlPacketBodySizeMax];
    };

    class HtcctrlPacket : public BasePacket<HtcctrlPacketHeader>, public util::IntrusiveListBaseNode<HtcctrlPacket> {
        public:
            using BasePacket<HtcctrlPacketHeader>::BasePacket;
    };

    struct HtcctrlPacketDeleter {
        mem::StandardAllocator *m_allocator;

        void operator()(HtcctrlPacket *packet) {
            std::destroy_at(packet);
            m_allocator->Free(packet);
        }
    };

}
