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
#include "sprofile_srv_service_for_system_process.hpp"
#include "sprofile_srv_profile_reader_impl.hpp"
#include "sprofile_srv_profile_controller_for_debug_impl.hpp"

namespace ams::sprofile::srv {

    Result ServiceForSystemProcess::OpenProfileReader(sf::Out<sf::SharedPointer<::ams::sprofile::IProfileReader>> out) {
        /* Allocate an object. */
        auto obj = sf::ObjectFactory<sf::MemoryResourceAllocationPolicy>::CreateSharedEmplaced<IProfileReader, ProfileReaderImpl>(m_memory_resource, m_profile_manager);
        R_UNLESS(obj != nullptr, sprofile::ResultAllocationFailed());

        /* Return the object. */
        *out = std::move(obj);
        return ResultSuccess();
    }

    Result ServiceForSystemProcess::OpenProfileUpdateObserver(sf::Out<sf::SharedPointer<::ams::sprofile::IProfileUpdateObserver>> out) {
        return m_profile_manager->GetUpdateObserverManager().OpenObserver(out, m_memory_resource);
    }

    Result ServiceForSystemProcess::OpenProfileControllerForDebug(sf::Out<sf::SharedPointer<::ams::sprofile::IProfileControllerForDebug>> out) {
        /* Require debug mode in order to open a debug controller. */
        R_UNLESS(settings::fwdbg::IsDebugModeEnabled(), sprofile::ResultNotPermitted());

        /* Allocate an object. */
        auto obj = sf::ObjectFactory<sf::MemoryResourceAllocationPolicy>::CreateSharedEmplaced<IProfileControllerForDebug, ProfileControllerForDebugImpl>(m_memory_resource, m_profile_manager);
        R_UNLESS(obj != nullptr, sprofile::ResultAllocationFailed());

        /* Return the object. */
        *out = std::move(obj);
        return ResultSuccess();
    }

}
