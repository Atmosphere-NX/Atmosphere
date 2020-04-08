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
#include "../i2c_api.hpp"
#include "i2c_driver_types.hpp"
#include "i2c_bus_accessor.hpp"
#include "i2c_session.hpp"

namespace ams::i2c::driver::impl {

    class ResourceManager {
        public:
            static constexpr size_t MaxDriverSessions = 40;
            static constexpr size_t PowerBusId = ConvertToIndex(Bus::I2C5);
            static constexpr size_t InvalidSessionId = static_cast<size_t>(-1);
        private:
            os::Mutex initialize_mutex;
            os::Mutex session_open_mutex;
            size_t ref_cnt = 0;
            bool suspended = false;
            bool power_bus_suspended = false;
            Session sessions[MaxDriverSessions];
            BusAccessor bus_accessors[ConvertToIndex(Bus::Count)];
            TYPED_STORAGE(os::Mutex) transaction_mutexes[ConvertToIndex(Bus::Count)];
        public:
            ResourceManager() : initialize_mutex(false), session_open_mutex(false) {
                for (size_t i = 0; i < util::size(this->transaction_mutexes); i++) {
                    new (GetPointer(this->transaction_mutexes[i])) os::Mutex(false);
                }
            }

            ~ResourceManager() {
                for (size_t i = 0; i < util::size(this->transaction_mutexes); i++) {
                    GetReference(this->transaction_mutexes[i]).~Mutex();
                }
            }
        private:
            size_t GetFreeSessionId() const;
        public:
            /* N uses a singleton here, we'll oblige. */
            static ResourceManager &GetInstance() {
                static ResourceManager s_instance;
                return s_instance;
            }

            bool IsInitialized() const {
                return this->ref_cnt > 0;
            }

            Session& GetSession(size_t id) {
                return this->sessions[id];
            }

            os::Mutex& GetTransactionMutex(Bus bus) {
                return GetReference(this->transaction_mutexes[ConvertToIndex(bus)]);
            }

            void Initialize();
            void Finalize();

            void OpenSession(driver::Session *out_session, Bus bus, u32 slave_address, AddressingMode addressing_mode, SpeedMode speed_mode, u32 max_retries, u64 retry_wait_time);
            void CloseSession(const driver::Session &session);
            void SuspendBuses();
            void ResumeBuses();
            void SuspendPowerBus();
            void ResumePowerBus();
    };

}
