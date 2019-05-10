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
#include <switch.h>
#include <stratosphere.hpp>

#include "i2c_types.hpp"
#include "i2c_bus_accessor.hpp"
#include "i2c_driver_session.hpp"

class I2cResourceManager {
    public:
        static constexpr size_t MaxDriverSessions = 40;
        static constexpr size_t MaxBuses = 6;
        static constexpr size_t PowerBusId = static_cast<size_t>(I2cBus_I2c5);
        static constexpr size_t InvalidSessionId = static_cast<size_t>(-1);
    private:
        HosMutex initialize_mutex;
        HosMutex session_open_mutex;
        size_t ref_cnt = 0;
        bool suspended = false;
        bool power_bus_suspended = false;
        I2cDriverSession sessions[MaxDriverSessions];
        I2cBusAccessor bus_accessors[MaxBuses];
        HosMutex transaction_mutexes[MaxBuses];
    public:
        I2cResourceManager() {
            /* ... */
        }
    private:
        size_t GetFreeSessionId() const;
    public:
        /* N uses a singleton here, we'll oblige. */
        static I2cResourceManager &GetInstance() {
            static I2cResourceManager s_instance;
            return s_instance;
        }

        bool IsInitialized() const {
            return this->ref_cnt > 0;
        }

        I2cDriverSession& GetSession(size_t id) {
            return this->sessions[id];
        }

        HosMutex& GetTransactionMutex(I2cBus bus) {
            if (bus >= MaxBuses) {
                std::abort();
            }
            return this->transaction_mutexes[bus];
        }

        void Initialize();
        void Finalize();

        void OpenSession(I2cSessionImpl *out_session, I2cBus bus, u32 slave_address, AddressingMode addressing_mode, SpeedMode speed_mode, u32 max_retries, u64 retry_wait_time);
        void CloseSession(const I2cSessionImpl &session);
        void SuspendBuses();
        void ResumeBuses();
        void SuspendPowerBus();
        void ResumePowerBus();
};

