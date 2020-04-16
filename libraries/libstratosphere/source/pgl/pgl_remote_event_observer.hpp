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
#include <stratosphere.hpp>

namespace ams::pgl {

    class RemoteEventObserver final : public pgl::sf::IEventObserver {
        NON_COPYABLE(RemoteEventObserver);
        NON_MOVEABLE(RemoteEventObserver);
        private:
            ::PglEventObserver observer;
        public:
            constexpr RemoteEventObserver(const ::PglEventObserver &o) : observer(o) { /* ... */ }
            virtual ~RemoteEventObserver() override {
                ::pglEventObserverClose(std::addressof(this->observer));
            }

            virtual Result GetProcessEventHandle(ams::sf::OutCopyHandle out) override {
                ::Event ev;
                R_TRY(::pglEventObserverGetProcessEvent(std::addressof(this->observer), std::addressof(ev)));
                out.SetValue(ev.revent);
                return ResultSuccess();
            }

            virtual Result GetProcessEventInfo(ams::sf::Out<pm::ProcessEventInfo> out) override {
                static_assert(sizeof(*out.GetPointer()) == sizeof(::PmProcessEventInfo));
                return  ::pglEventObserverGetProcessEventInfo(std::addressof(this->observer), reinterpret_cast<::PmProcessEventInfo *>(out.GetPointer()));
            }
    };

}