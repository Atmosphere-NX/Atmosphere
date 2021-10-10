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

namespace ams::fatal::srv {

    class FatalEventManager {
        NON_COPYABLE(FatalEventManager);
        NON_MOVEABLE(FatalEventManager);
        public:
            static constexpr size_t NumFatalEvents = 3;
        private:
            os::SdkMutex m_lock;
            size_t m_num_events_gotten = 0;
            os::SystemEventType m_events[NumFatalEvents];
        public:
            FatalEventManager();
            Result GetEvent(const os::SystemEventType **out);
            void SignalEvents();
    };

}
