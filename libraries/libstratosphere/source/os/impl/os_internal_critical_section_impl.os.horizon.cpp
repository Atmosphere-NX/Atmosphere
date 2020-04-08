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

    namespace {

        ALWAYS_INLINE void DataMemoryBarrierForCriticalSection() {
            #if defined(ATMOSPHERE_ARCH_ARM64)
                /* ... */
            #else
                #error "Unknown architecture for os::impl::InternalCriticalSectionImpl DataMemoryBarrier"
            #endif
        }

        ALWAYS_INLINE u32 LoadExclusive(u32 *ptr) {
            u32 value;

            #if defined(ATMOSPHERE_ARCH_ARM64)
                __asm__ __volatile__("ldaxr %w[value], [%[ptr]]" : [value]"=&r"(value) : [ptr]"r"(ptr) : "memory");
            #else
                #error "Unknown architecture for os::impl::InternalCriticalSectionImpl LoadExclusive"
            #endif

            return value;
        }

        ALWAYS_INLINE int StoreExclusive(u32 *ptr, u32 value) {
            int result;

            #if defined(ATMOSPHERE_ARCH_ARM64)
                __asm__ __volatile__("stlxr %w[result], %w[value], [%[ptr]]" : [result]"=&r"(result) : [value]"r"(value), [ptr]"r"(ptr) : "memory");
            #else
                #error "Unknown architecture for os::impl::InternalCriticalSectionImpl StoreExclusive"
            #endif


            return result;
        }

        ALWAYS_INLINE void ClearExclusive() {
            #if defined(ATMOSPHERE_ARCH_ARM64)
                __asm__ __volatile__("clrex" ::: "memory");
            #else
                #error "Unknown architecture for os::impl::InternalCriticalSectionImpl ClearExclusive"
            #endif
        }

    }

    void InternalCriticalSectionImpl::Enter() {
        AMS_ASSERT(svc::GetThreadLocalRegion()->disable_count == 0);

        const auto cur_handle = GetCurrentThreadHandle();
        AMS_ASSERT((this->thread_handle & ~ams::svc::HandleWaitMask) != cur_handle);

        u32 value = LoadExclusive(std::addressof(this->thread_handle));
        while (true) {
            if (AMS_LIKELY(value == svc::InvalidHandle)) {
                if (AMS_UNLIKELY(StoreExclusive(std::addressof(this->thread_handle), cur_handle) != 0)) {
                    value = LoadExclusive(std::addressof(this->thread_handle));
                    continue;
                }
                break;
            }

            if (AMS_LIKELY((value & ams::svc::HandleWaitMask) == 0)) {
                if (AMS_UNLIKELY(StoreExclusive(std::addressof(this->thread_handle), value | ams::svc::HandleWaitMask) != 0)) {
                    value = LoadExclusive(std::addressof(this->thread_handle));
                    continue;
                }
            }

            R_ABORT_UNLESS(ams::svc::ArbitrateLock(value & ~ams::svc::HandleWaitMask, reinterpret_cast<uintptr_t>(std::addressof(this->thread_handle)), cur_handle));

            value = LoadExclusive(std::addressof(this->thread_handle));
            if (AMS_LIKELY((value & ~ams::svc::HandleWaitMask) == cur_handle)) {
                ClearExclusive();
                break;
            }
        }

        DataMemoryBarrierForCriticalSection();
    }

    bool InternalCriticalSectionImpl::TryEnter() {
        AMS_ASSERT(svc::GetThreadLocalRegion()->disable_count == 0);

        const auto cur_handle = GetCurrentThreadHandle();

        while (true) {
            u32 value = LoadExclusive(std::addressof(this->thread_handle));
            if (AMS_UNLIKELY(value != svc::InvalidHandle)) {
                break;
            }

            DataMemoryBarrierForCriticalSection();

            if (AMS_LIKELY(StoreExclusive(std::addressof(this->thread_handle), cur_handle) == 0)) {
                return true;
            }
        }

        ClearExclusive();
        DataMemoryBarrierForCriticalSection();

        return false;
    }

    void InternalCriticalSectionImpl::Leave() {
        AMS_ASSERT(svc::GetThreadLocalRegion()->disable_count == 0);

        const auto cur_handle = GetCurrentThreadHandle();
        u32 value = LoadExclusive(std::addressof(this->thread_handle));

        while (true) {
            if (AMS_UNLIKELY(value != cur_handle)) {
                ClearExclusive();
                break;
            }

            DataMemoryBarrierForCriticalSection();

            if (AMS_LIKELY(StoreExclusive(std::addressof(this->thread_handle), 0) == 0)) {
                break;
            }

            value = LoadExclusive(std::addressof(this->thread_handle));
        }

        DataMemoryBarrierForCriticalSection();

        AMS_ASSERT((value | ams::svc::HandleWaitMask) == (cur_handle | ams::svc::HandleWaitMask));
        if (value & ams::svc::HandleWaitMask) {
            R_ABORT_UNLESS(ams::svc::ArbitrateUnlock(reinterpret_cast<uintptr_t>(std::addressof(this->thread_handle))));
        }
    }

    bool InternalCriticalSectionImpl::IsLockedByCurrentThread() const {
        const auto cur_handle = GetCurrentThreadHandle();
        return (this->thread_handle & ~ams::svc::HandleWaitMask) == cur_handle;
    }

}
