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
#include <stratosphere.hpp>

namespace ams::regulator {

    void Initialize() {
        /* ... */
    }

    void Finalize() {
        /* ... */
    }

    Result OpenSession(RegulatorSession *out, DeviceCode device_code) {
        AMS_UNUSED(out, device_code);
        return ResultSuccess();
    }

    void CloseSession(RegulatorSession *session) {
        AMS_UNUSED(session);
    }

    bool GetVoltageEnabled(RegulatorSession *session) {
        AMS_UNUSED(session);
        return true;
    }

    Result SetVoltageEnabled(RegulatorSession *session, bool enabled) {
        AMS_UNUSED(session, enabled);
        return ResultSuccess();
    }

    Result SetVoltageValue(RegulatorSession *session, u32 micro_volts) {
        AMS_UNUSED(session, micro_volts);
        return ResultSuccess();
    }

}
