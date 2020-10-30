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

namespace ams::ddsf {

    class EventHandlerManager;

    class IEventHandler {
        NON_COPYABLE(IEventHandler);
        NON_MOVEABLE(IEventHandler);
        friend class EventHandlerManager;
        private:
            os::WaitableHolderType holder;
            uintptr_t user_data;
            bool is_initialized;
            bool is_registered;
        private:
            void Link(os::WaitableManagerType *manager) {
                AMS_ASSERT(this->IsInitialized());
                AMS_ASSERT(!this->IsRegistered());
                AMS_ASSERT(manager != nullptr);
                os::LinkWaitableHolder(manager, std::addressof(this->holder));
            }

            void Unlink() {
                AMS_ASSERT(this->IsInitialized());
                AMS_ASSERT(this->IsRegistered());
                os::UnlinkWaitableHolder(std::addressof(this->holder));
            }

            static IEventHandler &ToEventHandler(os::WaitableHolderType *holder) {
                AMS_ASSERT(holder != nullptr);
                auto &event_handler = *reinterpret_cast<IEventHandler *>(os::GetWaitableHolderUserData(holder));
                AMS_ASSERT(event_handler.IsInitialized());
                return event_handler;
            }
        public:
            IEventHandler() : holder(), user_data(0), is_initialized(false), is_registered(false) { /* ... */ }

            ~IEventHandler() {
                if (this->IsRegistered()) {
                    this->Unlink();
                }
                if (this->IsInitialized()) {
                    this->Finalize();
                }
            }

            bool IsInitialized() const { return this->is_initialized; }
            bool IsRegistered() const { return this->is_registered; }

            uintptr_t GetUserData() const { return this->user_data; }
            void SetUserData(uintptr_t d) { this->user_data = d; }

            template<typename T>
            void Initialize(T *object) {
                AMS_ASSERT(object != nullptr);
                AMS_ASSERT(!this->IsInitialized());
                os::InitializeWaitableHolder(std::addressof(this->holder), object);
                os::SetWaitableHolderUserData(std::addressof(this->holder), reinterpret_cast<uintptr_t>(this));
                this->is_initialized = true;
                this->is_registered  = false;
            }

            void Finalize() {
                AMS_ASSERT(this->IsInitialized());
                AMS_ASSERT(!this->IsRegistered());
                os::FinalizeWaitableHolder(std::addressof(this->holder));
                this->is_initialized = false;
                this->is_registered  = false;
            }
        protected:
            virtual void HandleEvent() = 0;
    };

}
