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

namespace ams::htclow {

    enum PacketType : u16 {
        PacketType_Data    = 24,
        PacketType_MaxData = 25,
        PacketType_Error   = 26,
    };

    static constexpr inline u32 HtcGen2Signature = 0xA79F3540;

    struct PacketHeader {
        u32 signature;
        u32 offset;
        u32 reserved;
        u32 body_size;
        s16 version;
        PacketType packet_type;
        impl::ChannelInternalType channel;
        u64 share;
    };
    static_assert(util::is_pod<PacketHeader>::value);
    static_assert(sizeof(PacketHeader) == 0x20);

    static constexpr inline size_t PacketBodySizeMax = 0x3E000;

    struct PacketBody {
        u8 data[PacketBodySizeMax];
    };

    template<typename HeaderType>
    class BasePacket {
        private:
            mem::StandardAllocator *m_allocator;
            u8 *m_header;
            int m_packet_size;
        public:
            BasePacket(mem::StandardAllocator *allocator, int packet_size) : m_allocator(allocator), m_header(nullptr), m_packet_size(packet_size) {
                AMS_ASSERT(packet_size >= static_cast<int>(sizeof(HeaderType)));

                m_header = static_cast<u8 *>(m_allocator->Allocate(m_packet_size));
            }

            virtual ~BasePacket() {
                if (m_header != nullptr) {
                    m_allocator->Free(m_header);
                }
            }

            bool IsAllocationSucceeded() const {
                return m_header != nullptr;
            }

            HeaderType *GetHeader() {
                return reinterpret_cast<HeaderType *>(m_header);
            }

            const HeaderType *GetHeader() const {
                return reinterpret_cast<const HeaderType *>(m_header);
            }

            int GetBodySize() const {
                return m_packet_size - sizeof(HeaderType);
            }

            u8 *GetBody() const {
                if (this->GetBodySize() > 0) {
                    return m_header + sizeof(HeaderType);
                } else {
                    return nullptr;
                }
            }

            void CopyBody(const void *src, int src_size) {
                AMS_ASSERT(this->GetBodySize() >= 0);

                std::memcpy(this->GetBody(), src, src_size);
            }
    };

    class Packet : public BasePacket<PacketHeader>, public util::IntrusiveListBaseNode<Packet> {
        public:
            using BasePacket<PacketHeader>::BasePacket;
    };

    struct PacketDeleter {
        mem::StandardAllocator *m_allocator;

        void operator()(Packet *packet) {
            std::destroy_at(packet);
            m_allocator->Free(packet);
        }
    };

}
