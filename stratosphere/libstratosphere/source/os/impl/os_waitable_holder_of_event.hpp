/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "os_waitable_holder_base.hpp"
#include "os_waitable_object_list.hpp"

namespace ams::os::impl {

    class WaitableHolderOfEvent : public WaitableHolderOfUserObject {
        private:
            Event *event;
        private:
            TriBool IsSignaledImpl() const {
                return this->event->signaled ? TriBool::True : TriBool::False;
            }
        public:
            explicit WaitableHolderOfEvent(Event *e) : event(e) { /* ... */ }

            /* IsSignaled, Link, Unlink implemented. */
            virtual TriBool IsSignaled() const override {
                std::scoped_lock lk(this->event->lock);
                return this->IsSignaledImpl();
            }

            virtual TriBool LinkToObjectList() override {
                std::scoped_lock lk(this->event->lock);

                GetReference(this->event->waitable_object_list_storage).LinkWaitableHolder(*this);
                return this->IsSignaledImpl();
            }

            virtual void UnlinkFromObjectList() override {
                std::scoped_lock lk(this->event->lock);

                GetReference(this->event->waitable_object_list_storage).UnlinkWaitableHolder(*this);
            }
    };

}
