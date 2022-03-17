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
#include <stratosphere/windows.hpp>
#include "os_internal_critical_section_impl.os.windows.hpp"

namespace ams::os::impl {

    void InternalCriticalSectionImpl::Initialize() {
        ::InitializeCriticalSection(std::addressof(util::GetReference(m_windows_critical_section_storage).cs));
    }

    void InternalCriticalSectionImpl::Finalize() {
        ::DeleteCriticalSection(std::addressof(util::GetReference(m_windows_critical_section_storage).cs));
    }

    void InternalCriticalSectionImpl::Enter() {
        ::EnterCriticalSection(std::addressof(util::GetReference(m_windows_critical_section_storage).cs));
    }

    bool InternalCriticalSectionImpl::TryEnter() {
        return ::TryEnterCriticalSection(std::addressof(util::GetReference(m_windows_critical_section_storage).cs)) != 0;
    }

    void InternalCriticalSectionImpl::Leave() {
        return ::LeaveCriticalSection(std::addressof(util::GetReference(m_windows_critical_section_storage).cs));
    }

    bool InternalCriticalSectionImpl::IsLockedByCurrentThread() const {
        /* Get the cs. */
        CRITICAL_SECTION * const cs = std::addressof(util::GetReference(m_windows_critical_section_storage).cs);

        /* If the critical section has no owning thread, it's not locked. */
        if (cs->OwningThread == nullptr) {
            return false;
        }

        /* If it has an owner, TryLock() will succeed if and only if we own the critical section. */
        if (::TryEnterCriticalSection(cs) == 0) {
            return false;
        }

        /* We now hold the critical section. To avoid a race, check that we didn't just acquire it by chance. */
        const auto holds_lock = cs->RecursionCount > 1;

        /* Leave, since we just successfully entered. */
        ::LeaveCriticalSection(cs);

        return holds_lock;
    }

}
