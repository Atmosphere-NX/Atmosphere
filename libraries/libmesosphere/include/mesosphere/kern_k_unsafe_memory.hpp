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
#include <mesosphere/kern_k_light_lock.hpp>

namespace ams::kern {

    class KUnsafeMemory {
        private:
            mutable KLightLock m_lock;
            size_t m_limit_size;
            size_t m_current_size;
        public:
            constexpr KUnsafeMemory() : m_lock(), m_limit_size(), m_current_size() { /* ... */ }

            bool TryReserve(size_t size) {
                MESOSPHERE_ASSERT_THIS();
                KScopedLightLock lk(m_lock);

                /* Test for overflow. */
                if (m_current_size > m_current_size + size) {
                    return false;
                }

                /* Test for limit allowance. */
                if (m_current_size + size > m_limit_size) {
                    return false;
                }

                /* Reserve the size. */
                m_current_size += size;
                return true;
            }

            void Release(size_t size) {
                MESOSPHERE_ASSERT_THIS();
                KScopedLightLock lk(m_lock);

                MESOSPHERE_ABORT_UNLESS(m_current_size >= size);
                m_current_size -= size;
            }

            size_t GetLimitSize() const {
                MESOSPHERE_ASSERT_THIS();
                KScopedLightLock lk(m_lock);
                return m_limit_size;
            }

            size_t GetCurrentSize() const {
                MESOSPHERE_ASSERT_THIS();
                KScopedLightLock lk(m_lock);
                return m_current_size;
            }

            Result SetLimitSize(size_t size) {
                MESOSPHERE_ASSERT_THIS();
                KScopedLightLock lk(m_lock);

                R_UNLESS(size >= m_current_size, svc::ResultLimitReached());
                m_limit_size = size;
                return ResultSuccess();
            }
    };
}
