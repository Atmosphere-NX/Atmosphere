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
#include "i2c_pcv.hpp"
#include "i2c_resource_manager.hpp"

namespace ams::i2c::driver::impl {

    void ResourceManager::Initialize() {
        std::scoped_lock lk(this->initialize_mutex);
        this->ref_cnt++;
    }

    void ResourceManager::Finalize() {
        std::scoped_lock lk(this->initialize_mutex);
        AMS_ABORT_UNLESS(this->ref_cnt > 0);
        this->ref_cnt--;
        if (this->ref_cnt > 0) {
            return;
        }

        {
            std::scoped_lock sess_lk(this->session_open_mutex);
            for (size_t i = 0; i < MaxDriverSessions; i++) {
                this->sessions[i].Close();
            }
        }
    }

    size_t ResourceManager::GetFreeSessionId() const {
        for (size_t i = 0; i < MaxDriverSessions; i++) {
            if (!this->sessions[i].IsOpen()) {
                return i;
            }
        }

        return InvalidSessionId;
    }

    void ResourceManager::OpenSession(driver::Session *out_session, Bus bus, u32 slave_address, AddressingMode addressing_mode, SpeedMode speed_mode, u32 max_retries, u64 retry_wait_time) {
        bool need_enable_ldo6 = false;
        size_t session_id = InvalidSessionId;
        /* Get, open session. */
        {
            std::scoped_lock lk(this->session_open_mutex);
            AMS_ABORT_UNLESS(out_session != nullptr);
            AMS_ABORT_UNLESS(bus < Bus::Count);

            session_id = GetFreeSessionId();
            AMS_ABORT_UNLESS(session_id != InvalidSessionId);


            if ((bus == Bus::I2C2 || bus == Bus::I2C3) && (this->bus_accessors[ConvertToIndex(Bus::I2C2)].GetOpenSessions() == 0 && this->bus_accessors[ConvertToIndex(Bus::I2C3)].GetOpenSessions() == 0)) {
                need_enable_ldo6 = true;
            }

            out_session->session_id = session_id;
            out_session->bus_idx = ConvertToIndex(bus);
            this->sessions[session_id].Open(bus, slave_address, addressing_mode, speed_mode, &this->bus_accessors[ConvertToIndex(bus)], max_retries, retry_wait_time);
        }

        this->sessions[session_id].Start();
        if (need_enable_ldo6) {
            pcv::Initialize();
            R_ABORT_UNLESS(pcv::SetVoltageValue(10, 2'900'000));
            R_ABORT_UNLESS(pcv::SetVoltageEnabled(10, true));
            pcv::Finalize();
            svcSleepThread(560'000ul);
        }
    }

    void ResourceManager::CloseSession(const driver::Session &session) {
        bool need_disable_ldo6 = false;
        /* Get, open session. */
        {
            std::scoped_lock lk(this->session_open_mutex);
            AMS_ABORT_UNLESS(this->sessions[session.session_id].IsOpen());

            this->sessions[session.session_id].Close();

            if ((ConvertFromIndex(session.bus_idx) == Bus::I2C2 || ConvertFromIndex(session.bus_idx) == Bus::I2C3) &&
                (this->bus_accessors[ConvertToIndex(Bus::I2C2)].GetOpenSessions() == 0 && this->bus_accessors[ConvertToIndex(Bus::I2C3)].GetOpenSessions() == 0)) {
                need_disable_ldo6 = true;
            }
        }

        if (need_disable_ldo6) {
            pcv::Initialize();
            R_ABORT_UNLESS(pcv::SetVoltageEnabled(10, false));
            pcv::Finalize();
        }

    }

    void ResourceManager::SuspendBuses() {
        AMS_ABORT_UNLESS(this->ref_cnt > 0);

        if (!this->suspended) {
            {
                std::scoped_lock lk(this->session_open_mutex);
                this->suspended = true;
                for (size_t i = 0; i < ConvertToIndex(Bus::Count); i++) {
                    if (i != PowerBusId && this->bus_accessors[i].GetOpenSessions() > 0) {
                        this->bus_accessors[i].Suspend();
                    }
                }
            }
            pcv::Initialize();
            R_ABORT_UNLESS(pcv::SetVoltageEnabled(10, false));
            pcv::Finalize();
        }
    }

    void ResourceManager::ResumeBuses() {
        AMS_ABORT_UNLESS(this->ref_cnt > 0);

        if (this->suspended) {
            if (this->bus_accessors[ConvertToIndex(Bus::I2C2)].GetOpenSessions() > 0 || this->bus_accessors[ConvertToIndex(Bus::I2C3)].GetOpenSessions() > 0) {
                pcv::Initialize();
                R_ABORT_UNLESS(pcv::SetVoltageValue(10, 2'900'000));
                R_ABORT_UNLESS(pcv::SetVoltageEnabled(10, true));
                pcv::Finalize();
                svcSleepThread(1'560'000ul);
            }
            {
                std::scoped_lock lk(this->session_open_mutex);
                for (size_t i = 0; i < ConvertToIndex(Bus::Count); i++) {
                    if (i != PowerBusId && this->bus_accessors[i].GetOpenSessions() > 0) {
                        this->bus_accessors[i].Resume();
                    }
                }
            }
            this->suspended = false;
        }
    }

    void ResourceManager::SuspendPowerBus() {
        AMS_ABORT_UNLESS(this->ref_cnt > 0);
        std::scoped_lock lk(this->session_open_mutex);

        if (!this->power_bus_suspended) {
            this->power_bus_suspended = true;
            if (this->bus_accessors[PowerBusId].GetOpenSessions() > 0) {
                this->bus_accessors[PowerBusId].Suspend();
            }
        }
    }

    void ResourceManager::ResumePowerBus() {
        AMS_ABORT_UNLESS(this->ref_cnt > 0);
        std::scoped_lock lk(this->session_open_mutex);

        if (this->power_bus_suspended) {
            if (this->bus_accessors[PowerBusId].GetOpenSessions() > 0) {
                this->bus_accessors[PowerBusId].Resume();
            }
            this->power_bus_suspended = false;
        }
    }

}

