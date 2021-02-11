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
#include "htcs_manager.hpp"
#include "htcs_manager_impl.hpp"

namespace ams::htcs::impl {

    HtcsManager::HtcsManager(mem::StandardAllocator *allocator, htclow::HtclowManager *htclow_manager) : m_allocator(allocator), m_impl(static_cast<HtcsManagerImpl *>(allocator->Allocate(sizeof(HtcsManagerImpl), alignof(HtcsManagerImpl)))) {
        std::construct_at(m_impl, m_allocator, htclow_manager);
    }

    HtcsManager::~HtcsManager() {
        std::destroy_at(m_impl);
        m_allocator->Free(m_impl);
    }

    os::EventType *HtcsManager::GetServiceAvailabilityEvent() {
        return m_impl->GetServiceAvailabilityEvent();
    }

    bool HtcsManager::IsServiceAvailable() {
        return m_impl->IsServiceAvailable();
    }

}
