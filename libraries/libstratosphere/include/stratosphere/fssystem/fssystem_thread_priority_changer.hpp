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
#include <vapours.hpp>
#include <stratosphere/os.hpp>

namespace ams::fssystem {

    class ScopedThreadPriorityChanger {
        public:
            enum class Mode {
                Absolute,
                Relative,
            };
        private:
            os::ThreadType *m_thread;
            s32 m_priority;
        public:
            ALWAYS_INLINE explicit ScopedThreadPriorityChanger(s32 priority, Mode mode) : m_thread(os::GetCurrentThread()), m_priority(0) {
                const auto result_priority = std::min((mode == Mode::Relative) ? os::GetThreadPriority(m_thread) + priority : priority, os::LowestSystemThreadPriority);
                m_priority = os::ChangeThreadPriority(m_thread, result_priority);
            }

            ALWAYS_INLINE ~ScopedThreadPriorityChanger() {
                os::ChangeThreadPriority(m_thread, m_priority);
            }
    };

    class ScopedThreadPriorityChangerByAccessPriority {
        public:
            enum class AccessMode {
                Read,
                Write,
            };
        private:
            static s32 GetThreadPriorityByAccessPriority(AccessMode mode);
        private:
            ScopedThreadPriorityChanger m_scoped_changer;
        public:
            ALWAYS_INLINE explicit ScopedThreadPriorityChangerByAccessPriority(AccessMode mode) : m_scoped_changer(GetThreadPriorityByAccessPriority(mode), ScopedThreadPriorityChanger::Mode::Absolute) {
                /* ... */
            }
    };

}
