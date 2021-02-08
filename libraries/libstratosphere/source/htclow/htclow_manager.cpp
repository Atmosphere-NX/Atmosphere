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
#include <stratosphere.hpp>
#include "htclow_manager.hpp"
#include "htclow_manager_impl.hpp"

namespace ams::htclow {

    HtclowManager::HtclowManager(mem::StandardAllocator *allocator) : m_allocator(allocator), m_impl(static_cast<HtclowManagerImpl *>(allocator->Allocate(sizeof(HtclowManagerImpl), alignof(HtclowManagerImpl)))) {
        std::construct_at(m_impl, m_allocator);
    }

    HtclowManager::~HtclowManager() {
        std::destroy_at(m_impl);
        m_allocator->Free(m_impl);
    }

    Result HtclowManager::OpenDriver(impl::DriverType driver_type) {
        return m_impl->OpenDriver(driver_type);
    }

    void HtclowManager::CloseDriver() {
        return m_impl->CloseDriver();
    }

    void HtclowManager::Disconnect() {
        return m_impl->Disconnect();
    }

}
