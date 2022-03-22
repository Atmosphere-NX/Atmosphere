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
#include "htc_rpc_tasks.hpp"
#include "htc_htcmisc_rpc_tasks.hpp"

namespace ams::htc::server::rpc {

    class RpcTaskQueue {
        private:
            u32 m_task_ids[MaxRpcCount];
            PacketCategory m_task_categories[MaxRpcCount];
            int m_offset;
            int m_count;
            os::SdkConditionVariable m_cv;
            os::SdkMutex m_mutex;
            bool m_cancelled;
        public:
            constexpr RpcTaskQueue() = default;

            void Initialize() {
                m_offset    = 0;
                m_count     = 0;
                m_cancelled = false;
            }

            void Finalize() {
                m_offset    = 0;
                m_count     = 0;
            }

            void Cancel() {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Cancel ourselves. */
                m_cancelled = true;

                /* Signal to consumers/producers. */
                m_cv.Signal();
            }

            void Add(u32 task_id, PacketCategory category) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Check pre-conditions. */
                AMS_ASSERT(m_count < static_cast<int>(MaxRpcCount));

                /* Determine index. */
                const auto index = ((m_count++) + m_offset) % MaxRpcCount;

                /* Set task. */
                m_task_ids[index]        = task_id;
                m_task_categories[index] = category;

                /* Signal. */
                if (m_count > 0) {
                    m_cv.Signal();
                }
            }

            Result Take(u32 *out_id, PacketCategory *out_category) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Wait until we can take. */
                while (m_count == 0 && !m_cancelled) {
                    m_cv.Wait(m_mutex);
                }

                /* Check that we're not cancelled. */
                R_UNLESS(!m_cancelled, htc::ResultCancelled());

                /* Determine index. */
                const auto index = m_offset;

                /* Advance the queue. */
                m_offset = (m_offset + 1) % MaxRpcCount;
                --m_count;

                /* Return the task info. */
                *out_id       = m_task_ids[index];
                *out_category = m_task_categories[index];
                return ResultSuccess();
            }
    };

}
