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
#include "sprofile_srv_i_profile_update_observer.hpp"

namespace ams::sprofile::srv {

    class ProfileUpdateObserverImpl {
        private:
            os::SystemEvent m_event;
        public:
            ProfileUpdateObserverImpl() : m_event(os::EventClearMode_ManualClear, true) { /* ... */ }
            virtual ~ProfileUpdateObserverImpl() { /* ... */ }
        public:
            os::SystemEvent &GetEvent() { return m_event; }
            const os::SystemEvent &GetEvent() const { return m_event; }
    };

    class ProfileUpdateObserverManager {
        public:
            static constexpr auto MaxObservers = 10;
        private:
            ProfileUpdateObserverImpl *m_observers[MaxObservers];
            int m_observer_count;
            os::SdkMutex m_mutex;
        public:
            ProfileUpdateObserverManager() : m_observer_count(0), m_mutex() { /* ... */ }
        public:
            Result OpenObserver(sf::Out<sf::SharedPointer<::ams::sprofile::IProfileUpdateObserver>> &out, MemoryResource *memory_resource);
            void CloseObserver(ProfileUpdateObserverImpl *observer);
    };

}
