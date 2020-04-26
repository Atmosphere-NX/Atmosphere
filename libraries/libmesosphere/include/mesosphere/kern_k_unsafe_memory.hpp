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
            mutable KLightLock lock;
            size_t limit_size;
            size_t current_size;
        public:
            constexpr KUnsafeMemory() : lock(), limit_size(), current_size() { /* ... */ }

            bool TryReserve(size_t size) {
                MESOSPHERE_ASSERT_THIS();
                KScopedLightLock lk(this->lock);

                /* Test for overflow. */
                if (this->current_size > this->current_size + size) {
                    return false;
                }

                /* Test for limit allowance. */
                if (this->current_size + size > this->limit_size) {
                    return false;
                }

                /* Reserve the size. */
                this->current_size += size;
                return true;
            }

            void Release(size_t size) {
                MESOSPHERE_ASSERT_THIS();
                KScopedLightLock lk(this->lock);

                MESOSPHERE_ABORT_UNLESS(this->current_size >= size);
                this->current_size -= size;
            }

            size_t GetLimitSize() const {
                MESOSPHERE_ASSERT_THIS();
                KScopedLightLock lk(this->lock);
                return this->limit_size;
            }

            size_t GetCurrentSize() const {
                MESOSPHERE_ASSERT_THIS();
                KScopedLightLock lk(this->lock);
                return this->current_size;
            }

            Result SetLimitSize(size_t size) {
                MESOSPHERE_ASSERT_THIS();
                KScopedLightLock lk(this->lock);

                R_UNLESS(size >= this->current_size, svc::ResultLimitReached());
                this->limit_size = size;
                return ResultSuccess();
            }
    };
}
