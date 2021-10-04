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

namespace ams::os {

    namespace {

        ALWAYS_INLINE bool IsBarrierInitialized(const BarrierType *barrier) {
            return barrier->max_threads != 0;
        }

        ALWAYS_INLINE u64 GetBarrierBaseCounterImpl(const BarrierType *barrier) {
            /* Check pre-conditions. */
            AMS_ASSERT(util::GetReference(barrier->cs_barrier).IsLockedByCurrentThread());

            /* Convert two u32s to u64. */
            return (static_cast<u64>(barrier->base_counter_lower) << 0) | (static_cast<u64>(barrier->base_counter_upper) << BITSIZEOF(barrier->base_counter_lower));
        }

        ALWAYS_INLINE void SetBarrierBaseCounterImpl(BarrierType *barrier, u64 value) {
            /* Check pre-conditions. */
            AMS_ASSERT(util::GetReference(barrier->cs_barrier).IsLockedByCurrentThread());

            /* Store as two u32s. */
            barrier->base_counter_lower = static_cast<u32>(value >> 0);
            barrier->base_counter_upper = static_cast<u32>(value >> BITSIZEOF(barrier->base_counter_lower));
        }

    }

    void InitializeBarrier(BarrierType *barrier, int num_threads) {
        /* Check pre-conditions. */
        AMS_ASSERT(num_threads >= 1);

        /* Construct objects. */
        util::ConstructAt(barrier->cs_barrier);
        util::ConstructAt(barrier->cv_gathered);

        /* Set member variables. */
        barrier->max_threads        = num_threads;
        barrier->waiting_threads    = 0;
        barrier->base_counter_lower = 0;
        barrier->base_counter_upper = 0;
    }

    void FinalizeBarrier(BarrierType *barrier) {
        /* Check pre-conditions. */
        AMS_ASSERT(IsBarrierInitialized(barrier));
        AMS_ASSERT(barrier->waiting_threads == 0);

        /* Clear max threads. */
        barrier->max_threads = 0;

        /* Destroy objects. */
        util::DestroyAt(barrier->cs_barrier);
        util::DestroyAt(barrier->cv_gathered);
    }

    void AwaitBarrier(BarrierType *barrier) {
        /* Check pre-conditions. */
        AMS_ASSERT(IsBarrierInitialized(barrier));

        /* Await the barrier. */
        {
            /* Acquire exclusive access to the barrier. */
            auto &cs = util::GetReference(barrier->cs_barrier);
            std::scoped_lock lk(cs);

            /* Read barrier state. */
            const u64 base_counter     = GetBarrierBaseCounterImpl(barrier);
            const auto max_threads     = barrier->max_threads;
                  auto waiting_threads = barrier->waiting_threads;

            /* Determine next base counter. */
            const u64 done_base_counter = base_counter + max_threads;

            /* Increment waiting threads. */
            ++waiting_threads;

            /* Check if all threads have synchronized. */
            if (waiting_threads >= max_threads) {
                /* They have, so reset waiting thread count. */
                barrier->waiting_threads = 0;

                /* Set the updated base counter. */
                SetBarrierBaseCounterImpl(barrier, done_base_counter);

                /* Broadcast to our cv. */
                util::GetReference(barrier->cv_gathered).Broadcast();
            } else {
                /* More threads are needed, so update waiting thread count. */
                barrier->waiting_threads = waiting_threads;

                /* Wait for remaining threads to await. */
                while (GetBarrierBaseCounterImpl(barrier) < done_base_counter) {
                    util::GetReference(barrier->cv_gathered).Wait(std::addressof(cs));
                }
            }
        }
    }

}
