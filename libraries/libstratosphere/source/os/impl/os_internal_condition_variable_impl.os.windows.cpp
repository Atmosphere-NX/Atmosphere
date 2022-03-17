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
#include "os_timeout_helper.hpp"
#include "os_thread_manager.hpp"

namespace ams::os::impl {

    struct WindowsConditionVariable {
        CONDITION_VARIABLE cv;
    };

    void InternalConditionVariableImpl::Initialize() {
        ::InitializeConditionVariable(std::addressof(util::GetReference(m_windows_cv_storage).cv));
    }

    void InternalConditionVariableImpl::Finalize() {
        /* ... */
    }

    void InternalConditionVariableImpl::Signal() {
        ::WakeConditionVariable(std::addressof(util::GetReference(m_windows_cv_storage).cv));
    }

    void InternalConditionVariableImpl::Broadcast() {
        ::WakeAllConditionVariable(std::addressof(util::GetReference(m_windows_cv_storage).cv));
    }

    void InternalConditionVariableImpl::Wait(InternalCriticalSection *cs) {
        ::SleepConditionVariableCS(std::addressof(util::GetReference(m_windows_cv_storage).cv), std::addressof(util::GetReference(cs->Get()->m_windows_critical_section_storage).cs), INFINITE);
    }

    ConditionVariableStatus InternalConditionVariableImpl::TimedWait(InternalCriticalSection *cs, const TimeoutHelper &timeout_helper) {
        const auto ret = ::SleepConditionVariableCS(std::addressof(util::GetReference(m_windows_cv_storage).cv), std::addressof(util::GetReference(cs->Get()->m_windows_critical_section_storage).cs), timeout_helper.GetTimeLeftOnTarget());
        if (ret == 0) {
            if (::GetLastError() == ERROR_TIMEOUT) {
                return ConditionVariableStatus::TimedOut;
            }
        }

        return ConditionVariableStatus::Success;
    }

}
