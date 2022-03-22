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
#include <stratosphere.hpp>
#include "sprofile_srv_profile_manager.hpp"
#include "sprofile_srv_profile_update_observer_impl.hpp"

namespace ams::sprofile::srv {

    namespace {

        class AutoUnregisterObserver : public ProfileUpdateObserverImpl {
            private:
                ProfileUpdateObserverManager *m_manager;
            public:
                AutoUnregisterObserver(ProfileUpdateObserverManager *manager) : m_manager(manager) { /* ... */ }
                virtual ~AutoUnregisterObserver() { m_manager->CloseObserver(this); }
        };
        static_assert(sprofile::IsIProfileUpdateObserver<AutoUnregisterObserver>);

    }

    Result ProfileUpdateObserverManager::OpenObserver(sf::Out<sf::SharedPointer<::ams::sprofile::IProfileUpdateObserver>> &out, MemoryResource *memory_resource) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we can allocate. */
        R_UNLESS(m_observer_count < MaxObservers, sprofile::ResultMaxObservers());

        /* Allocate an object. */
        auto obj = sf::ObjectFactory<sf::MemoryResourceAllocationPolicy>::CreateSharedEmplaced<IProfileUpdateObserver, AutoUnregisterObserver>(memory_resource, this);
        R_UNLESS(obj != nullptr, sprofile::ResultAllocationFailed());

        /* Register the observer. */
        m_observers[m_observer_count++] = std::addressof(obj.GetImpl());

        /* Return the object. */
        *out = std::move(obj);
        return ResultSuccess();
    }

    void ProfileUpdateObserverManager::CloseObserver(ProfileUpdateObserverImpl *observer) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Find the observer. */
        int index = -1;
        for (auto i = 0; i < m_observer_count; ++i) {
            if (m_observers[i] == observer) {
                index = i;
                break;
            }
        }
        AMS_ABORT_UNLESS(index != -1);

        /* Remove from our list. */
        m_observers[index] = m_observers[--m_observer_count];

        /* Sanity check. */
        AMS_ABORT_UNLESS(m_observer_count >= 0);
    }

}
