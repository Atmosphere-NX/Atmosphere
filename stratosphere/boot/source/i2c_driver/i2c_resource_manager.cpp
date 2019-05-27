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

#include <switch.h>
#include <stratosphere.hpp>

#include "boot_pcv.hpp"
#include "i2c_resource_manager.hpp"

void I2cResourceManager::Initialize() {
    std::scoped_lock<HosMutex> lk(this->initialize_mutex);
    this->ref_cnt++;
}

void I2cResourceManager::Finalize() {
    std::scoped_lock<HosMutex> lk(this->initialize_mutex);
    if (this->ref_cnt == 0) {
        std::abort();
    }
    this->ref_cnt--;
    if (this->ref_cnt > 0) {
        return;
    }

    {
        std::scoped_lock<HosMutex> sess_lk(this->session_open_mutex);
        for (size_t i = 0; i < MaxDriverSessions; i++) {
            this->sessions[i].Close();
        }
    }
}

size_t I2cResourceManager::GetFreeSessionId() const {
    for (size_t i = 0; i < MaxDriverSessions; i++) {
        if (!this->sessions[i].IsOpen()) {
            return i;
        }
    }

    return InvalidSessionId;
}

void I2cResourceManager::OpenSession(I2cSessionImpl *out_session, I2cBus bus, u32 slave_address, AddressingMode addressing_mode, SpeedMode speed_mode, u32 max_retries, u64 retry_wait_time) {
    bool need_enable_ldo6 = false;
    size_t session_id = InvalidSessionId;
    /* Get, open session. */
    {
        std::scoped_lock<HosMutex> lk(this->session_open_mutex);
        if (out_session == nullptr || bus >= MaxBuses) {
            std::abort();
        }

        session_id = GetFreeSessionId();
        if (session_id == InvalidSessionId) {
            std::abort();
        }


        if ((bus == I2cBus_I2c2 || bus == I2cBus_I2c3) && (this->bus_accessors[I2cBus_I2c2].GetOpenSessions() == 0 && this->bus_accessors[I2cBus_I2c3].GetOpenSessions() == 0)) {
            need_enable_ldo6 = true;
        }

        out_session->session_id = session_id;
        out_session->bus = bus;
        this->sessions[session_id].Open(bus, slave_address, addressing_mode, speed_mode, &this->bus_accessors[bus], max_retries, retry_wait_time);
    }

    this->sessions[session_id].Start();
    if (need_enable_ldo6) {
        Pcv::Initialize();
        if (R_FAILED(Pcv::SetVoltageValue(10, 2'900'000))) {
            std::abort();
        }
        if (R_FAILED(Pcv::SetVoltageEnabled(10, true))) {
            std::abort();
        }
        Pcv::Finalize();
        svcSleepThread(560'000ul);
    }
}

void I2cResourceManager::CloseSession(const I2cSessionImpl &session) {
    bool need_disable_ldo6 = false;
    /* Get, open session. */
    {
        std::scoped_lock<HosMutex> lk(this->session_open_mutex);
        if (!this->sessions[session.session_id].IsOpen()) {
            std::abort();
        }

        this->sessions[session.session_id].Close();

        if ((session.bus == I2cBus_I2c2 || session.bus == I2cBus_I2c3) && (this->bus_accessors[I2cBus_I2c2].GetOpenSessions() == 0 && this->bus_accessors[I2cBus_I2c3].GetOpenSessions() == 0)) {
            need_disable_ldo6 = true;
        }
    }

    if (need_disable_ldo6) {
        Pcv::Initialize();
        if (R_FAILED(Pcv::SetVoltageEnabled(10, false))) {
            std::abort();
        }
        Pcv::Finalize();
    }

}

void I2cResourceManager::SuspendBuses() {
    if (this->ref_cnt == 0) {
        std::abort();
    }

    if (!this->suspended) {
        {
            std::scoped_lock<HosMutex> lk(this->session_open_mutex);
            this->suspended = true;
            for (size_t i = 0; i < MaxBuses; i++) {
                if (i != PowerBusId && this->bus_accessors[i].GetOpenSessions() > 0) {
                    this->bus_accessors[i].Suspend();
                }
            }
        }
        Pcv::Initialize();
        if (R_FAILED(Pcv::SetVoltageEnabled(10, false))) {
            std::abort();
        }
        Pcv::Finalize();
    }
}

void I2cResourceManager::ResumeBuses() {
    if (this->ref_cnt == 0) {
        std::abort();
    }

    if (this->suspended) {
        if (this->bus_accessors[I2cBus_I2c2].GetOpenSessions() > 0 || this->bus_accessors[I2cBus_I2c3].GetOpenSessions() > 0) {
            Pcv::Initialize();
            if (R_FAILED(Pcv::SetVoltageValue(10, 2'900'000))) {
                std::abort();
            }
            if (R_FAILED(Pcv::SetVoltageEnabled(10, true))) {
                std::abort();
            }
            Pcv::Finalize();
            svcSleepThread(1'560'000ul);
        }
        {
            std::scoped_lock<HosMutex> lk(this->session_open_mutex);
            for (size_t i = 0; i < MaxBuses; i++) {
                if (i != PowerBusId && this->bus_accessors[i].GetOpenSessions() > 0) {
                    this->bus_accessors[i].Resume();
                }
            }
        }
        this->suspended = false;
    }
}

void I2cResourceManager::SuspendPowerBus() {
    if (this->ref_cnt == 0) {
        std::abort();
    }
    std::scoped_lock<HosMutex> lk(this->session_open_mutex);

    if (!this->power_bus_suspended) {
        this->power_bus_suspended = true;
        if (this->bus_accessors[PowerBusId].GetOpenSessions() > 0) {
            this->bus_accessors[PowerBusId].Suspend();
        }
    }
}

void I2cResourceManager::ResumePowerBus() {
    if (this->ref_cnt == 0) {
        std::abort();
    }
    std::scoped_lock<HosMutex> lk(this->session_open_mutex);

    if (this->power_bus_suspended) {
        if (this->bus_accessors[PowerBusId].GetOpenSessions() > 0) {
            this->bus_accessors[PowerBusId].Resume();
        }
        this->power_bus_suspended = false;
    }
}
