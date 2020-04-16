/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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
#include <stratosphere/os.hpp>
#include <stratosphere/pm.hpp>
#include <stratosphere/pgl/pgl_types.hpp>
#include <stratosphere/pgl/sf/pgl_sf_i_event_observer.hpp>

namespace ams::pgl {

    class EventObserver {
        NON_COPYABLE(EventObserver);
        private:
            std::shared_ptr<pgl::sf::IEventObserver> interface;
        public:
            EventObserver() { /* ... */ }
            explicit EventObserver(std::shared_ptr<pgl::sf::IEventObserver> intf) : interface(std::move(intf)) { /* ... */ }

            EventObserver(EventObserver &&rhs) {
                this->interface = std::move(rhs.interface);
            }

            EventObserver &operator=(EventObserver &&rhs) {
                EventObserver(std::move(rhs)).Swap(*this);
                return *this;
            }

            void Swap(EventObserver &rhs) {
                std::swap(this->interface, rhs.interface);
            }
        public:
            Result GetSystemEvent(os::SystemEventType *out) {
                ams::sf::CopyHandle handle;
                R_TRY(this->interface->GetProcessEventHandle(std::addressof(handle)));
                os::AttachSystemEvent(out, handle.GetValue(), true, svc::InvalidHandle, false, os::EventClearMode_AutoClear);
                return ResultSuccess();
            }

            Result GetProcessEventInfo(pm::ProcessEventInfo *out) {
                return this->interface->GetProcessEventInfo(out);
            }
    };

}
