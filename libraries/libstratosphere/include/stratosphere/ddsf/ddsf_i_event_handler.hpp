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

namespace ams::ddsf {

    class EventHandlerManager;

    class IEventHandler {
        NON_COPYABLE(IEventHandler);
        NON_MOVEABLE(IEventHandler);
        friend class EventHandlerManager;
        private:
            os::MultiWaitHolderType m_holder;
            uintptr_t m_user_data;
            bool m_is_initialized;
            bool m_is_registered;
        private:
            void Link(os::MultiWaitType *multi_wait) {
                AMS_ASSERT(this->IsInitialized());
                AMS_ASSERT(!this->IsRegistered());
                AMS_ASSERT(multi_wait != nullptr);
                os::LinkMultiWaitHolder(multi_wait, std::addressof(m_holder));
            }

            void Unlink() {
                AMS_ASSERT(this->IsInitialized());
                AMS_ASSERT(this->IsRegistered());
                os::UnlinkMultiWaitHolder(std::addressof(m_holder));
            }

            static IEventHandler &ToEventHandler(os::MultiWaitHolderType *holder) {
                AMS_ASSERT(holder != nullptr);
                auto &event_handler = *reinterpret_cast<IEventHandler *>(os::GetMultiWaitHolderUserData(holder));
                AMS_ASSERT(event_handler.IsInitialized());
                return event_handler;
            }
        public:
            IEventHandler() : m_holder(), m_user_data(0), m_is_initialized(false), m_is_registered(false) { /* ... */ }

            ~IEventHandler() {
                if (this->IsRegistered()) {
                    this->Unlink();
                }
                if (this->IsInitialized()) {
                    this->Finalize();
                }
            }

            bool IsInitialized() const { return m_is_initialized; }
            bool IsRegistered() const { return m_is_registered; }

            uintptr_t GetUserData() const { return m_user_data; }
            void SetUserData(uintptr_t d) { m_user_data = d; }

            template<typename T>
            void Initialize(T *object) {
                AMS_ASSERT(object != nullptr);
                AMS_ASSERT(!this->IsInitialized());
                os::InitializeMultiWaitHolder(std::addressof(m_holder), object);
                os::SetMultiWaitHolderUserData(std::addressof(m_holder), reinterpret_cast<uintptr_t>(this));
                m_is_initialized = true;
                m_is_registered  = false;
            }

            void Finalize() {
                AMS_ASSERT(this->IsInitialized());
                AMS_ASSERT(!this->IsRegistered());
                os::FinalizeMultiWaitHolder(std::addressof(m_holder));
                m_is_initialized = false;
                m_is_registered  = false;
            }
        protected:
            virtual void HandleEvent() = 0;
    };

}
