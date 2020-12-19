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

namespace ams::kern {

    class KWorkerTask {
        private:
            KWorkerTask *m_next_task;
        public:
            constexpr ALWAYS_INLINE KWorkerTask() : m_next_task(nullptr) { /* ... */ }

            constexpr ALWAYS_INLINE KWorkerTask *GetNextTask() const { return m_next_task; }
            constexpr ALWAYS_INLINE void SetNextTask(KWorkerTask *task) { m_next_task = task; }

            virtual void DoWorkerTask() = 0;
    };

}
