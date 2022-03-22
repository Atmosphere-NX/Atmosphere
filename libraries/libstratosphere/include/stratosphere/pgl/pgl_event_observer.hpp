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
#include <stratosphere/os.hpp>
#include <stratosphere/pm.hpp>
#include <stratosphere/pgl/pgl_types.hpp>
#include <stratosphere/pgl/sf/pgl_sf_i_event_observer.hpp>
#include <stratosphere/pgl/tipc/pgl_tipc_i_event_observer.hpp>

namespace ams::pgl {

    namespace impl {

        class EventObserverInterface {
            NON_COPYABLE(EventObserverInterface);
            NON_MOVEABLE(EventObserverInterface);
            public:
                constexpr EventObserverInterface() = default;

                virtual ~EventObserverInterface() { /* ... */ }

                virtual Result GetSystemEvent(os::SystemEventType *out)       = 0;
                virtual Result GetProcessEventInfo(pm::ProcessEventInfo *out) = 0;
        };

        class EventObserverByCmif final : public EventObserverInterface {
            NON_COPYABLE(EventObserverByCmif);
            NON_MOVEABLE(EventObserverByCmif);
            private:
                ams::sf::SharedPointer<pgl::sf::IEventObserver> m_cmif_interface;
            public:
                explicit EventObserverByCmif(ams::sf::SharedPointer<pgl::sf::IEventObserver> intf) : m_cmif_interface(intf) { /* ... */ }
            public:
                virtual Result GetSystemEvent(os::SystemEventType *out) override {
                    ams::sf::NativeHandle handle;
                    R_TRY(m_cmif_interface->GetProcessEventHandle(std::addressof(handle)));

                    os::AttachReadableHandleToSystemEvent(out, handle.GetOsHandle(), handle.IsManaged(), os::EventClearMode_AutoClear);
                    handle.Detach();

                    return ResultSuccess();
                }

                virtual Result GetProcessEventInfo(pm::ProcessEventInfo *out) override {
                    return m_cmif_interface->GetProcessEventInfo(out);
                }
        };

        template<typename T> requires tipc::IsIEventObserver<T>
        class EventObserverByTipc final : public EventObserverInterface {
            NON_COPYABLE(EventObserverByTipc);
            NON_MOVEABLE(EventObserverByTipc);
            private:
                T m_tipc_interface;
            public:
                template<typename... Args>
                explicit EventObserverByTipc(Args &&... args) : m_tipc_interface(std::forward<Args>(args)...) { /* ... */ }
            public:
                virtual Result GetSystemEvent(os::SystemEventType *out) override {
                    os::NativeHandle handle;
                    R_TRY(m_tipc_interface.GetProcessEventHandle(std::addressof(handle)));
                    os::AttachReadableHandleToSystemEvent(out, handle, true, os::EventClearMode_AutoClear);
                    return ResultSuccess();
                }

                virtual Result GetProcessEventInfo(pm::ProcessEventInfo *out) override {
                    return m_tipc_interface.GetProcessEventInfo(ams::tipc::Out<pm::ProcessEventInfo>(out));
                }
        };

    }

    class EventObserver {
        NON_COPYABLE(EventObserver);
        private:
            struct Deleter {
                void operator()(impl::EventObserverInterface *);
            };
        public:
            using UniquePtr = std::unique_ptr<impl::EventObserverInterface, Deleter>;
        private:
            UniquePtr m_impl;
        public:
            EventObserver() { /* ... */ }

            explicit EventObserver(UniquePtr impl) : m_impl(std::move(impl)) { /* ... */ }

            EventObserver(EventObserver &&rhs) {
                m_impl = std::move(rhs.m_impl);
            }

            EventObserver &operator=(EventObserver &&rhs) {
                EventObserver(std::move(rhs)).Swap(*this);
                return *this;
            }

            void Swap(EventObserver &rhs) {
                std::swap(m_impl, rhs.m_impl);
            }
        public:
            Result GetSystemEvent(os::SystemEventType *out) {
                return m_impl->GetSystemEvent(out);
            }

            Result GetProcessEventInfo(pm::ProcessEventInfo *out) {
                return m_impl->GetProcessEventInfo(out);
            }
    };

}
