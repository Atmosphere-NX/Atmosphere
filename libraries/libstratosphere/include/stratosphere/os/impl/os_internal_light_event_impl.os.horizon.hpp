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

namespace ams::os::impl {

    class TimeoutHelper;

    class InternalLightEventImpl {
        private:
            std::atomic<s32> m_state;
            u32 m_padding;
        public:
            explicit InternalLightEventImpl(bool signaled) { this->Initialize(signaled); }
            ~InternalLightEventImpl() { this->Finalize(); }

            void Initialize(bool signaled);
            void Finalize();

            void SignalWithAutoClear();
            void SignalWithManualClear();

            void Clear();

            void WaitWithAutoClear();
            void WaitWithManualClear();

            bool TryWaitWithAutoClear();
            bool TryWaitWithManualClear();

            bool TimedWaitWithAutoClear(const TimeoutHelper &timeout_helper);
            bool TimedWaitWithManualClear(const TimeoutHelper &timeout_helper);
    };

}
