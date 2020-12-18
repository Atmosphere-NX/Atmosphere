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
            constexpr ALWAYS_INLINE KTimerTask() : m_time(0) { /* ... */ }

            constexpr ALWAYS_INLINE void SetTime(s64 t) {
                m_time = t;
            }

            constexpr ALWAYS_INLINE s64 GetTime() const {
                return m_time;
            }

            virtual void OnTimer() = 0;

    };

}
