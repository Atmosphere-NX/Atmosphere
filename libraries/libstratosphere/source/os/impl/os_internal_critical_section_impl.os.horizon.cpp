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
#include "os_timeout_helper.hpp"
#include "os_thread_manager.hpp"

namespace ams::os::impl {

    #if defined(ATMOSPHERE_ARCH_ARM64)

        void InternalCriticalSectionImpl::Enter() {
            AMS_ASSERT(svc::GetThreadLocalRegion()->disable_count == 0);

            /* Use the libnx impl. */
            static_assert(std::is_same<decltype(this->thread_handle), ::Mutex>::value);
            return ::mutexLock(std::addressof(this->thread_handle));
        }

        bool InternalCriticalSectionImpl::TryEnter() {
            AMS_ASSERT(svc::GetThreadLocalRegion()->disable_count == 0);

            /* Use the libnx impl. */
            static_assert(std::is_same<decltype(this->thread_handle), ::Mutex>::value);
            return ::mutexTryLock(std::addressof(this->thread_handle));
        }

        void InternalCriticalSectionImpl::Leave() {
            AMS_ASSERT(svc::GetThreadLocalRegion()->disable_count == 0);

            /* Use the libnx impl. */
            static_assert(std::is_same<decltype(this->thread_handle), ::Mutex>::value);
            return ::mutexUnlock(std::addressof(this->thread_handle));
        }

        bool InternalCriticalSectionImpl::IsLockedByCurrentThread() const {
            /* Use the libnx impl. */
            static_assert(std::is_same<decltype(this->thread_handle), ::Mutex>::value);
            return ::mutexIsLockedByCurrentThread(std::addressof(this->thread_handle));
        }

    #else

        #error "Architecture not yet supported for os::InternalCriticalSectionImpl"

    #endif

}
