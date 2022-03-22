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

    /* For convenience. */
    template<typename T>
    concept IsTypeCheckableTask = std::derived_from<T, Task> && requires (T &t) {
        { t.GetTaskType() } -> std::convertible_to<decltype(T::TaskType)>;
    };

    static_assert(!IsTypeCheckableTask<Task>);
    static_assert(IsTypeCheckableTask<GetEnvironmentVariableTask>);

    class RpcTaskTable {
        private:
            /* htcs::ReceiveSmallTask/htcs::ReceiveSendTask are the largest tasks, containing an inline 0xE000 buffer. */
            /* We allow for ~0x100 task overhead from the additional events those contain. */
            /* NOTE: Nintendo hardcodes a maximum size of 0xE1D8, despite SendSmallTask being 0xE098 as of latest check. */
            static constexpr size_t MaxTaskSize = 0xE100;
            using TaskStorage = typename std::aligned_storage<MaxTaskSize, alignof(void *)>::type;
        private:
            bool m_valid[MaxRpcCount];
            TaskStorage m_storages[MaxRpcCount];
        private:
            template<typename T>
            ALWAYS_INLINE T *GetPointer(u32 index) {
                static_assert(alignof(T) <= alignof(TaskStorage));
                static_assert(sizeof(T)  <= sizeof(TaskStorage));
                return reinterpret_cast<T *>(std::addressof(m_storages[index]));
            }

            ALWAYS_INLINE bool IsValid(u32 index) {
                return index < MaxRpcCount && m_valid[index];
            }
        public:
            constexpr RpcTaskTable() = default;

            template<typename T> requires std::derived_from<T, Task>
            T *New(u32 index) {
                /* Sanity check input. */
                AMS_ASSERT(!this->IsValid(index));

                /* Set valid. */
                m_valid[index] = true;

                /* Allocate the task. */
                T *task = this->GetPointer<T>(index);

                /* Create the task. */
                std::construct_at(task);

                /* Return the task. */
                return task;
            }

            template<typename T> requires std::derived_from<T, Task>
            T *Get(u32 index) {
                /* Check that the task is valid. */
                if (!this->IsValid(index)) {
                    return nullptr;
                }

                /* Get the task pointer. */
                T *task = this->GetPointer<T>(index);

                /* Type check the task. */
                if constexpr (IsTypeCheckableTask<T>) {
                    if (task->GetTaskType() != T::TaskType) {
                        task = nullptr;
                    }
                }

                /* Return the task. */
                return task;
            }

            template<typename T> requires std::derived_from<T, Task>
            void Delete(u32 index) {
                /* Check that the task is valid. */
                if (!this->IsValid(index)) {
                    return;
                }

                /* Delete the task. */
                std::destroy_at(this->GetPointer<T>(index));

                /* Mark the task as invalid. */
                m_valid[index] = false;
            }

            void Delete(u32 index) {
                if (this->IsValid(index)) {
                    this->Delete<Task>(index);
                }
            }
    };

}
