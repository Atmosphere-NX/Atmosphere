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
#pragma once
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_current_context.hpp>
#include <mesosphere/kern_k_scoped_lock.hpp>

namespace ams::kern {

    class KLightLock {
        private:
            std::atomic<uintptr_t> tag;
        public:
            constexpr KLightLock() : tag(0) { /* ... */ }

            void Lock() {
                MESOSPHERE_ASSERT_THIS();

                const uintptr_t cur_thread = reinterpret_cast<uintptr_t>(GetCurrentThreadPointer());
                const uintptr_t cur_thread_tag = (cur_thread | 1);

                while (true) {
                    uintptr_t old_tag = this->tag.load(std::memory_order_relaxed);

                    while (!this->tag.compare_exchange_weak(old_tag, (old_tag == 0) ? cur_thread : old_tag | 1, std::memory_order_acquire)) {
                        if ((old_tag | 1) == cur_thread_tag) {
                            return;
                        }
                    }

                    if ((old_tag == 0) || ((old_tag | 1) == cur_thread_tag)) {
                        break;
                    }

                    this->LockSlowPath(old_tag | 1, cur_thread);
                }
            }

            void Unlock() {
                MESOSPHERE_ASSERT_THIS();

                const uintptr_t cur_thread = reinterpret_cast<uintptr_t>(GetCurrentThreadPointer());
                uintptr_t expected = cur_thread;
                do {
                    if (expected != cur_thread) {
                        return this->UnlockSlowPath(cur_thread);
                    }
                } while (!this->tag.compare_exchange_weak(expected, 0, std::memory_order_release));
            }

            void LockSlowPath(uintptr_t owner, uintptr_t cur_thread);
            void UnlockSlowPath(uintptr_t cur_thread);

            bool IsLocked() const { return this->tag != 0; }
            bool IsLockedByCurrentThread() const { return (this->tag | 0x1ul) == (reinterpret_cast<uintptr_t>(GetCurrentThreadPointer()) | 0x1ul); }
    };

    using KScopedLightLock = KScopedLock<KLightLock>;

}
