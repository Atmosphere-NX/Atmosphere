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

namespace ams::fs::impl {

    class EventNotifierObjectAdapter final : public ::ams::fs::IEventNotifier, public ::ams::fs::impl::Newable {
        private:
            sf::SharedPointer<fssrv::sf::IEventNotifier> m_object;
        public:
            EventNotifierObjectAdapter(sf::SharedPointer<fssrv::sf::IEventNotifier> &&obj) : m_object(obj) { /* ... */ }
            virtual ~EventNotifierObjectAdapter() { /* ... */ }
        private:
            virtual Result DoBindEvent(os::SystemEventType *out, os::EventClearMode clear_mode) override {
                /* Get the handle. */
                sf::NativeHandle handle;
                AMS_FS_R_TRY(m_object->GetEventHandle(std::addressof(handle)));

                /* Create the system event. */
                os::AttachReadableHandleToSystemEvent(out, handle.GetOsHandle(), handle.IsManaged(), clear_mode);
                handle.Detach();

                return ResultSuccess();
            }
    };

    class RemoteEventNotifierObjectAdapter final : public ::ams::fs::IEventNotifier, public ::ams::fs::impl::Newable {
        private:
            ::FsEventNotifier m_notifier;
        public:
            RemoteEventNotifierObjectAdapter(::FsEventNotifier &n) : m_notifier(n) { /* ... */ }
            virtual ~RemoteEventNotifierObjectAdapter() { fsEventNotifierClose(std::addressof(m_notifier)); }
        private:
            virtual Result DoBindEvent(os::SystemEventType *out, os::EventClearMode clear_mode) override {
                /* Get the handle. */
                ::Event e;
                R_TRY(fsEventNotifierGetEventHandle(std::addressof(m_notifier), std::addressof(e), false));

                /* Create the system event. */
                os::AttachReadableHandleToSystemEvent(out, e.revent, true, clear_mode);

                return ResultSuccess();
            }
    };

}
