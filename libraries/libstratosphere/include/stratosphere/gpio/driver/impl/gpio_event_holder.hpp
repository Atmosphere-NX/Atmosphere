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

namespace ams::gpio::driver::impl {

    class EventHolder {
        NON_COPYABLE(EventHolder);
        NON_MOVEABLE(EventHolder);
        private:
            os::SystemEventType *m_event;
        public:
            constexpr EventHolder() : m_event(nullptr) { /* ... */ }

            void AttachEvent(os::SystemEventType *event) {
                m_event = event;
            }

            os::SystemEventType *DetachEvent() {
                auto ev = m_event;
                m_event = nullptr;
                return ev;
            }

            os::SystemEventType *GetSystemEvent() {
                return m_event;
            }

            bool IsBound() const {
                return m_event != nullptr;
            }
    };

}
