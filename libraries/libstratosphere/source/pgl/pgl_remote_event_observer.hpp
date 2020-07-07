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

    class RemoteEventObserver final {
        NON_COPYABLE(RemoteEventObserver);
        NON_MOVEABLE(RemoteEventObserver);
        private:
            ::PglEventObserver observer;
        public:
            constexpr RemoteEventObserver(const ::PglEventObserver &o) : observer(o) { /* ... */ }
            ~RemoteEventObserver() {
                ::pglEventObserverClose(std::addressof(this->observer));
            }

            Result GetProcessEventHandle(ams::sf::OutCopyHandle out) {
                ::Event ev;
                R_TRY(::pglEventObserverGetProcessEvent(std::addressof(this->observer), std::addressof(ev)));
                out.SetValue(ev.revent);
                return ResultSuccess();
            }

            Result GetProcessEventInfo(ams::sf::Out<pm::ProcessEventInfo> out) {
                static_assert(sizeof(*out.GetPointer()) == sizeof(::PmProcessEventInfo));
                return  ::pglEventObserverGetProcessEventInfo(std::addressof(this->observer), reinterpret_cast<::PmProcessEventInfo *>(out.GetPointer()));
            }
    };
    static_assert(pgl::sf::IsIEventObserver<RemoteEventObserver>);

}