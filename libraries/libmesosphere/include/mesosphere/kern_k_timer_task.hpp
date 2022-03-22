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
#include <mesosphere/kern_common.hpp>

namespace ams::kern {

    class KTimerTask : public util::IntrusiveRedBlackTreeBaseNode<KTimerTask> {
        private:
            s64 m_time;
        public:
            static constexpr ALWAYS_INLINE int Compare(const KTimerTask &lhs, const KTimerTask &rhs) {
                if (lhs.GetTime() < rhs.GetTime()) {
                    return -1;
                } else {
                    return 1;
                }
            }
        public:
            constexpr explicit ALWAYS_INLINE KTimerTask(util::ConstantInitializeTag) : util::IntrusiveRedBlackTreeBaseNode<KTimerTask>(util::ConstantInitialize), m_time(0) { /* ... */ }
            explicit ALWAYS_INLINE KTimerTask() : m_time(0) { /* ... */ }

            constexpr ALWAYS_INLINE void SetTime(s64 t) {
                m_time = t;
            }

            constexpr ALWAYS_INLINE s64 GetTime() const {
                return m_time;
            }

            /* NOTE: This is virtual in Nintendo's kernel. Prior to 13.0.0, KWaitObject was also a TimerTask; this is no longer the case. */
            /* Since this is now KThread exclusive, we have devirtualized (see inline declaration for this inside kern_kthread.hpp). */
            void OnTimer();
    };

}
