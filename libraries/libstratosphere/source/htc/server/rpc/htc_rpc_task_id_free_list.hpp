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

    class RpcTaskIdFreeList {
        private:
            u32 m_task_ids[MaxRpcCount];
            u32 m_offset;
            u32 m_free_count;
        public:
            constexpr RpcTaskIdFreeList() : m_task_ids(), m_offset(0), m_free_count(MaxRpcCount) {
                for (auto i = 0; i < static_cast<int>(MaxRpcCount); ++i) {
                    m_task_ids[i] = i;
                }
            }

            Result Allocate(u32 *out) {
                /* Check that we have free tasks. */
                R_UNLESS(m_free_count > 0, htc::ResultOutOfRpcTask());

                /* Get index. */
                const auto index = m_offset;
                m_offset = (m_offset + 1) % MaxRpcCount;
                --m_free_count;

                /* Get the task id. */
                *out = m_task_ids[index];
                return ResultSuccess();
            }

            void Free(u32 task_id) {
                /* Check pre-conditions. */
                AMS_ASSERT(m_free_count < static_cast<int>(MaxRpcCount));

                /* Determine index. */
                const auto index = ((m_free_count++) + m_offset) % MaxRpcCount;

                /* Set the task id. */
                m_task_ids[index] = task_id;
            }
    };

}
