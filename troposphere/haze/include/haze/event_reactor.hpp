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

#include <haze/common.hpp>

namespace haze {

    class EventConsumer {
        public:
            virtual ~EventConsumer() = default;
            virtual void ProcessEvent() = 0;
    };

    class EventReactor {
        private:
            EventConsumer *m_consumers[svc::ArgumentHandleCountMax];
            Waiter m_waiters[svc::ArgumentHandleCountMax];
            s32 m_num_wait_objects;
            Result m_result;
        public:
            constexpr explicit EventReactor() : m_consumers(), m_waiters(), m_num_wait_objects(), m_result(ResultSuccess()) { /* ... */ }

            bool AddConsumer(EventConsumer *consumer, Waiter waiter);
            void RemoveConsumer(EventConsumer *consumer);
        public:
            void SetResult(Result r) { m_result = r; }
            Result GetResult() const { return m_result; }
        public:
            template <typename... Args> requires (sizeof...(Args) > 0)
            Result WaitFor(s32 *out_arg_waiter, Args &&... arg_waiters) {
                const Waiter arg_waiter_array[] = { arg_waiters... };
                return this->WaitForImpl(out_arg_waiter, arg_waiter_array, sizeof...(Args));
            }
        private:
            Result WaitForImpl(s32 *out_arg_waiter, const Waiter *arg_waiters, s32 num_arg_waiters);
    };

}
