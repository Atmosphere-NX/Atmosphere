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
#include <mesosphere.hpp>

namespace ams::kern {

    NOINLINE bool KDebugBase::ProcessHolder::Open() {
        /* Atomically increment the reference count, only if it's positive. */
        u32 cur_ref_count = m_ref_count.load(std::memory_order_relaxed);
        do {
            if (AMS_UNLIKELY(cur_ref_count == 0)) {
                MESOSPHERE_AUDIT(cur_ref_count != 0);
                return false;
            }
            MESOSPHERE_ABORT_UNLESS(cur_ref_count < cur_ref_count + 1);
        } while (!m_ref_count.compare_exchange_weak(cur_ref_count, cur_ref_count + 1, std::memory_order_relaxed));

        return true;
    }

    NOINLINE void KDebugBase::ProcessHolder::Close() {
        /* Atomically decrement the reference count, not allowing it to become negative. */
        u32 cur_ref_count = m_ref_count.load(std::memory_order_relaxed);
        do {
            MESOSPHERE_ABORT_UNLESS(cur_ref_count > 0);
        } while (!m_ref_count.compare_exchange_weak(cur_ref_count, cur_ref_count - 1, std::memory_order_relaxed));

        /* If ref count hits zero, schedule the object for destruction. */
        if (cur_ref_count - 1 == 0) {
            this->Detach();
        }
    }

}
