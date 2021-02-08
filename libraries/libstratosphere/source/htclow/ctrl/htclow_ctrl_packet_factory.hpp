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
#include <stratosphere.hpp>

namespace ams::htclow::ctrl {

    class HtcctrlPacketFactory {
        private:
            mem::StandardAllocator *m_allocator;
            u32 m_seed;
        public:
            HtcctrlPacketFactory(mem::StandardAllocator *allocator) : m_allocator(allocator) {
                /* Get the current time. */
                const u64 time = os::GetSystemTick().GetInt64Value();

                /* Set the random seed. */
                {
                    util::TinyMT rng;
                    rng.Initialize(reinterpret_cast<const u32 *>(std::addressof(time)), sizeof(time) / sizeof(u32));

                    m_seed = rng.GenerateRandomU32();
                }
            }
    };

}
