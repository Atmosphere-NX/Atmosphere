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
#include <stratosphere.hpp>

namespace ams::os::impl {

    template<typename T>
    concept IsMessageQueueType = requires(T &t) {
        { t.buffer   } -> std::convertible_to<uintptr_t *>;
        { t.offset   } -> std::convertible_to<s32>;
        { t.count    } -> std::same_as<decltype(t.offset) &>;
        { t.capacity } -> std::same_as<decltype(t.count) &>;
    };

    template<typename T> requires IsMessageQueueType<T>
    class MessageQueueHelper {
        public:
            static ALWAYS_INLINE bool IsMessageQueueFull(const T *mq) {
                return mq->count >= mq->capacity;
            }

            static ALWAYS_INLINE bool IsMessageQueueEmpty(const T *mq) {
                return mq->count == 0;
            }

            static void EnqueueUnsafe(T *mq, uintptr_t data) {
                /* Ensure our limits are correct. */
                      auto count    = mq->count;
                const auto capacity = mq->capacity;
                AMS_ASSERT(count < capacity);

                /* Determine where we're writing. */
                auto ind = mq->offset + count;
                if (ind >= capacity) {
                    ind -= capacity;
                }
                AMS_ASSERT(0 <= ind && ind < capacity);

                /* Write the data. */
                mq->buffer[ind] = data;
                ++count;

                /* Update tracking. */
                mq->count = count;
            }

            static uintptr_t DequeueUnsafe(T *mq) {
                /* Ensure our limits are correct. */
                      auto count    = mq->count;
                      auto offset   = mq->offset;
                const auto capacity = mq->capacity;
                AMS_ASSERT(count > 0);
                AMS_ASSERT(offset >= 0 && offset < capacity);

                /* Get the data. */
                auto data = mq->buffer[offset++];

                /* Calculate new tracking variables. */
                if (offset >= capacity) {
                    offset -= capacity;
                }
                --count;

                /* Update tracking. */
                mq->offset = offset;
                mq->count  = count;

                return data;
            }

            static void JamUnsafe(T *mq, uintptr_t data) {
                /* Ensure our limits are correct. */
                      auto count    = mq->count;
                const auto capacity = mq->capacity;
                AMS_ASSERT(count < capacity);

                /* Determine where we're writing. */
                auto offset = mq->offset - 1;
                if (offset < 0) {
                    offset += capacity;
                }
                AMS_ASSERT(0 <= offset && offset < capacity);

                /* Write the data. */
                mq->buffer[offset] = data;
                ++count;

                /* Update tracking. */
                mq->offset = offset;
                mq->count  = count;
            }

            static uintptr_t PeekUnsafe(const T *mq) {
                /* Ensure our limits are correct. */
                const auto count    = mq->count;
                const auto offset   = mq->offset;
                AMS_ASSERT(count > 0);
                AMS_UNUSED(count);

                return mq->buffer[offset];
            }
    };

}
