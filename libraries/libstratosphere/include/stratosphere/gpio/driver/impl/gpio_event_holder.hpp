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
#include <vapours.hpp>
#include <stratosphere/os.hpp>

namespace ams::gpio::driver::impl {

    class EventHolder {
        NON_COPYABLE(EventHolder);
        NON_MOVEABLE(EventHolder);
        private:
            os::SystemEventType *event;
        public:
            constexpr EventHolder() : event(nullptr) { /* ... */ }

            void AttachEvent(os::SystemEventType *event) {
                this->event = event;
            }

            os::SystemEventType *DetachEvent() {
                auto ev = this->event;
                this->event = nullptr;
                return ev;
            }

            os::SystemEventType *GetSystemEvent() {
                return this->event;
            }

            bool IsBound() const {
                return this->event != nullptr;
            }
    };

}
