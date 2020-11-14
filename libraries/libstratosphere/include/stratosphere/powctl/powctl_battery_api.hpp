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
#include <stratosphere/powctl/powctl_types.hpp>
#include <stratosphere/powctl/powctl_session_api.hpp>

namespace ams::powctl {

    /* Battery API. */
    Result GetBatterySocRep(float *out_percent, Session &session);

    Result GetBatterySocVf(float *out_percent, Session &session);

    Result GetBatteryFullCapacity(int *out_mah, Session &session);
    Result GetBatteryRemainingCapacity(int *out_mah, Session &session);

    Result SetBatteryPercentageMinimumAlertThreshold(Session &session, float percentage);
    Result SetBatteryPercentageMaximumAlertThreshold(Session &session, float percentage);
    Result SetBatteryPercentageFullThreshold(Session &session, float percentage);

    Result GetBatteryAverageCurrent(int *out_ma, Session &session);
    Result GetBatteryCurrent(int *out_ma, Session &session);

    Result GetBatteryInternalState(void *dst, size_t *out_size, Session &session, size_t dst_size);
    Result SetBatteryInternalState(Session &session, const void *src, size_t src_size);

    Result GetBatteryNeedToRestoreParameters(bool *out, Session &session);
    Result SetBatteryNeedToRestoreParameters(Session &session, bool en);

    Result IsBatteryI2cShutdownEnabled(bool *out, Session &session);
    Result SetBatteryI2cShutdownEnabled(Session &session, bool en);

    Result IsBatteryPresent(bool *out, Session &session);

    Result GetBatteryCycles(int *out, Session &session);
    Result SetBatteryCycles(Session &session, int cycles);

    Result GetBatteryAge(float *out_percent, Session &session);

    Result GetBatteryTemperature(float *out_c, Session &session);
    Result GetBatteryMaximumTemperature(float *out_c, Session &session);

    Result SetBatteryTemperatureMinimumAlertThreshold(Session &session, float c);
    Result SetBatteryTemperatureMaximumAlertThreshold(Session &session, float c);

    Result GetBatteryVCell(int *out_mv, Session &session);
    Result GetBatteryAverageVCell(int *out_mv, Session &session);

    Result GetBatteryAverageVCellTime(TimeSpan *out, Session &session);

    Result GetBatteryOpenCircuitVoltage(int *out_mv, Session &session);

    Result SetBatteryVoltageMinimumAlertThreshold(Session &session, int mv);
    Result SetBatteryVoltageMaximumAlertThreshold(Session &session, int mv);

}
