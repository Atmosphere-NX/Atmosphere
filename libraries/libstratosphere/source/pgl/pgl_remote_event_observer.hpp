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
#include <stratosphere.hpp>

namespace ams::pgl {

    #if defined(ATMOSPHERE_OS_HORIZON)
    class RemoteEventObserver {
        NON_COPYABLE(RemoteEventObserver);
        NON_MOVEABLE(RemoteEventObserver);
        private:
            ::PglEventObserver m_observer;
        public:
            constexpr RemoteEventObserver(const ::PglEventObserver &o) : m_observer(o) { /* ... */ }
            ~RemoteEventObserver() {
                ::pglEventObserverClose(std::addressof(m_observer));
            }

            Result GetProcessEventHandle(ams::sf::OutCopyHandle out) {
                ::Event ev;
                ON_RESULT_SUCCESS { out.SetValue(ev.revent, true); };

                R_RETURN(::pglEventObserverGetProcessEvent(std::addressof(m_observer), std::addressof(ev)));
            }

            Result GetProcessEventInfo(ams::sf::Out<pm::ProcessEventInfo> out) {
                static_assert(sizeof(*out.GetPointer()) == sizeof(::PmProcessEventInfo));
                R_RETURN(::pglEventObserverGetProcessEventInfo(std::addressof(m_observer), reinterpret_cast<::PmProcessEventInfo *>(out.GetPointer())));
            }

            Result GetProcessEventHandle(ams::tipc::OutCopyHandle out) {
                ::Event ev;
                ON_RESULT_SUCCESS { out.SetValue(ev.revent); };

                R_RETURN(::pglEventObserverGetProcessEvent(std::addressof(m_observer), std::addressof(ev)));
            }

            Result GetProcessEventInfo(ams::tipc::Out<pm::ProcessEventInfo> out) {
                static_assert(sizeof(*out.GetPointer()) == sizeof(::PmProcessEventInfo));
                R_RETURN(::pglEventObserverGetProcessEventInfo(std::addressof(m_observer), reinterpret_cast<::PmProcessEventInfo *>(out.GetPointer())));
            }
    };
    static_assert(pgl::sf::IsIEventObserver<RemoteEventObserver>);
    static_assert(pgl::tipc::IsIEventObserver<RemoteEventObserver>);
    #endif

}