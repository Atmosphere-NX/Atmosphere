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
#include "htclow_ctrl_packet.hpp"

namespace ams::htclow::ctrl {

    class HtcctrlPacketFactory {
        private:
            mem::StandardAllocator *m_allocator;
            u32 m_sequence_id;
        public:
            HtcctrlPacketFactory(mem::StandardAllocator *allocator) : m_allocator(allocator) {
                /* Get the current time. */
                const u64 time = os::GetSystemTick().GetInt64Value();

                /* Set a random sequence id. */
                {
                    util::TinyMT rng;
                    rng.Initialize(reinterpret_cast<const u32 *>(std::addressof(time)), sizeof(time) / sizeof(u32));

                    m_sequence_id = rng.GenerateRandomU32();
                }
            }
        public:
            std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> MakeSuspendPacket();
            std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> MakeResumePacket();
            std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> MakeReadyPacket(const void *body, int body_size);
            std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> MakeInformationPacket(const void *body, int body_size);
            std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> MakeDisconnectPacket();
            std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> MakeConnectPacket(const void *body, int body_size);
            std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> MakeBeaconResponsePacket(const void *body, int body_size);

            void Delete(HtcctrlPacket *packet);
        private:
            std::unique_ptr<HtcctrlPacket, HtcctrlPacketDeleter> MakeSendPacketCommon(int body_size);
    };

}
