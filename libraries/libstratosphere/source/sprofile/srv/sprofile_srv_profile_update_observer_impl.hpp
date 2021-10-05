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
#include "sprofile_srv_i_profile_update_observer.hpp"

namespace ams::sprofile::srv {

    class ProfileUpdateObserverImpl {
        public:
            static constexpr auto MaxProfiles = 4;
        private:
            os::SystemEvent m_event;
            Identifier m_profiles[MaxProfiles];
            int m_profile_count;
            os::SdkMutex m_mutex;
        public:
            ProfileUpdateObserverImpl() : m_event(os::EventClearMode_ManualClear, true), m_profile_count(0), m_mutex() { /* ... */ }
            virtual ~ProfileUpdateObserverImpl() { /* ... */ }
        public:
            os::SystemEvent &GetEvent() { return m_event; }
            const os::SystemEvent &GetEvent() const { return m_event; }
        public:
            Result Listen(Identifier profile) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Check if we can listen. */
                R_UNLESS(m_profile_count < MaxProfiles, sprofile::ResultMaxListeners());

                /* Check if we're already listening. */
                for (auto i = 0; i < m_profile_count; ++i) {
                    R_UNLESS(m_profiles[i] != profile, sprofile::ResultAlreadyListening());
                }

                /* Add the profile. */
                m_profiles[m_profile_count++] = profile;
                return ResultSuccess();
            }

            Result Unlisten(Identifier profile) {
                /* Check that we're listening. */
                for (auto i = 0; i < m_profile_count; ++i) {
                    if (m_profiles[i] == profile) {
                        m_profiles[i] = m_profiles[--m_profile_count];
                        AMS_ABORT_UNLESS(m_profile_count >= 0);
                        return ResultSuccess();
                    }
                }

                return sprofile::ResultNotListening();
            }

            Result GetEventHandle(sf::OutCopyHandle out) {
                out.SetValue(m_event.GetReadableHandle(), false);
                return ResultSuccess();
            }
        public:
            void OnUpdate(Identifier profile) {
                for (auto i = 0; i < m_profile_count; ++i) {
                    if (m_profiles[i] == profile) {
                        m_event.Signal();
                        break;
                    }
                }
            }
    };
    static_assert(sprofile::IsIProfileUpdateObserver<ProfileUpdateObserverImpl>);

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

            void OnUpdate(Identifier profile) {
                std::scoped_lock lk(m_mutex);

                for (auto i = 0; i < m_observer_count; ++i) {
                    m_observers[i]->OnUpdate(profile);
                }
            }
    };

}
