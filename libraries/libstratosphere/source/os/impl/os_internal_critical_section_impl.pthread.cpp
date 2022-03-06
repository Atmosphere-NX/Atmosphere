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
#if defined(ATMOSPHERE_OS_LINUX)
#include <sys/syscall.h>
#include <unistd.h>
#endif

#if defined(AMS_OS_IMPL_USE_PTHREADS)
namespace ams::os::impl {

    #if defined(AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD)
    namespace {

        #if defined(ATMOSPHERE_OS_LINUX)

        template<typename T>
        concept IsGlibcPthreadMutexImplementationWithOwnerField = requires (T &t) {
            { t.__data.__owner } -> std::convertible_to<int>;
        };

        static_assert(IsGlibcPthreadMutexImplementationWithOwnerField<pthread_mutex_t>);

        #endif


    }
    #endif

    void InternalCriticalSectionImpl::Initialize() {
        /* TODO: What should be done here? */
    }

    void InternalCriticalSectionImpl::Finalize() {
        /* TODO: What should be done here? */
    }

    void InternalCriticalSectionImpl::Enter() {
        AMS_ABORT_UNLESS(pthread_mutex_lock(std::addressof(m_pthread_mutex)) == 0);
    }

    bool InternalCriticalSectionImpl::TryEnter() {
        return pthread_mutex_trylock(std::addressof(m_pthread_mutex)) == 0;
    }

    void InternalCriticalSectionImpl::Leave() {
        AMS_ABORT_UNLESS(pthread_mutex_unlock(std::addressof(m_pthread_mutex)) == 0);
    }

    #if defined(AMS_OS_INTERNAL_CRITICAL_SECTION_IMPL_CAN_CHECK_LOCKED_BY_CURRENT_THREAD)
    bool InternalCriticalSectionImpl::IsLockedByCurrentThread() const {
        /* TODO: This should be less of a terrible hack. */
        #if defined(ATMOSPHERE_OS_LINUX)
            return m_pthread_mutex.__data.__owner == ::syscall(SYS_gettid);
        #else
            #error "Unknown OS/underlying implementation for pthread InternalCriticalSectionImpl::IsLockedByCurrentThread()"
        #endif
    }
    #endif

}
#endif
