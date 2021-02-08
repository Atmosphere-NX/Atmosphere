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

namespace ams::htclow::mux {

    constexpr inline int MaxTaskCount = 0x80;

    class TaskManager {
        private:
            struct Task {
                impl::ChannelInternalType channel;
                os::EventType event;
                u8 _30;
                u8 _31;
                u8 _32;
                u64 _38;
            };
        private:
            bool m_valid[MaxTaskCount];
            Task m_tasks[MaxTaskCount];
        public:
            TaskManager() : m_valid() { /* ... */ }
    };

}
