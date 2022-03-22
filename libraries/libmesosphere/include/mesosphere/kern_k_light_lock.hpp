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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_current_context.hpp>
#include <mesosphere/kern_k_scoped_lock.hpp>

namespace ams::kern {

    class KLightLock {
        private:
            util::Atomic<uintptr_t> m_tag;
        public:
            constexpr ALWAYS_INLINE KLightLock() : m_tag(0) { /* ... */ }

            void Lock() {
                MESOSPHERE_ASSERT_THIS();

                const uintptr_t cur_thread = reinterpret_cast<uintptr_t>(GetCurrentThreadPointer());

                while (true) {
                    uintptr_t old_tag = m_tag.Load<std::memory_order_relaxed>();

                    while (!m_tag.CompareExchangeWeak<std::memory_order_acquire>(old_tag, (old_tag == 0) ? cur_thread : (old_tag | 1))) {
                        /* ... */
                    }

                    if (old_tag == 0 || this->LockSlowPath(old_tag | 1, cur_thread)) {
                        break;
                    }
                }
            }

            ALWAYS_INLINE void Unlock() {
                MESOSPHERE_ASSERT_THIS();

                const uintptr_t cur_thread = reinterpret_cast<uintptr_t>(GetCurrentThreadPointer());

                uintptr_t expected = cur_thread;
                if (!m_tag.CompareExchangeStrong<std::memory_order_release>(expected, 0)) {
                    this->UnlockSlowPath(cur_thread);
                }
            }

            bool LockSlowPath(uintptr_t owner, uintptr_t cur_thread);
            void UnlockSlowPath(uintptr_t cur_thread);

            ALWAYS_INLINE bool IsLocked() const { return m_tag.Load() != 0; }
            ALWAYS_INLINE bool IsLockedByCurrentThread() const { return (m_tag.Load() | 0x1ul) == (reinterpret_cast<uintptr_t>(GetCurrentThreadPointer()) | 0x1ul); }
    };

    using KScopedLightLock = KScopedLock<KLightLock>;

}
