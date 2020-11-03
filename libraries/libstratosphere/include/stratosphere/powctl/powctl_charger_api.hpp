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

    /* Charger API. */
    Result GetChargerChargeCurrentState(ChargeCurrentState *out, Session &session);
    Result SetChargerChargeCurrentState(Session &session, ChargeCurrentState state);

    Result GetChargerFastChargeCurrentLimit(u32 *out_ma, Session &session);
    Result SetChargerFastChargeCurrentLimit(Session &session, u32 ma);

    Result GetChargerChargeVoltageLimit(u32 *out_mv, Session &session);
    Result SetChargerChargeVoltageLimit(Session &session, u32 mv);

    Result SetChargerChargerConfiguration(Session &session, ChargerConfiguration cfg);

    Result IsChargerHiZEnabled(bool *out, Session &session);
    Result SetChargerHiZEnabled(Session &session, bool en);

    Result GetChargerInputCurrentLimit(u32 *out_ma, Session &session);
    Result SetChargerInputCurrentLimit(Session &session, u32 ma);

    Result GetChargerInputVoltageLimit(u32 *out_mv, Session &session);
    Result SetChargerInputVoltageLimit(Session &session, u32 mv);

    Result GetChargerChargerStatus(ChargerStatus *out, Session &session);

    Result IsChargerWatchdogTimerEnabled(bool *out, Session &session);
    Result SetChargerWatchdogTimerEnabled(Session &session, bool en);

    Result SetChargerWatchdogTimerTimeout(Session &session, TimeSpan timeout);
    Result ResetChargerWatchdogTimer(Session &session);

    Result GetChargerBatteryCompensation(u32 *out_mo, Session &session);
    Result SetChargerBatteryCompensation(Session &session, u32 mo);

    Result GetChargerVoltageClamp(u32 *out_mv, Session &session);
    Result SetChargerVoltageClamp(Session &session, u32 mv);

}
