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
#include <stratosphere/pwm/pwm_types.hpp>

namespace ams::pwm {

    struct ChannelSession {
        void *_session;
    };

    Result OpenSession(ChannelSession *out, DeviceCode device_code);
    void CloseSession(ChannelSession &session);

    void     SetPeriod(ChannelSession &session, TimeSpan period);
    TimeSpan GetPeriod(ChannelSession &session);

    void SetDuty(ChannelSession &session, int duty);
    int  GetDuty(ChannelSession &session);

    void SetEnabled(ChannelSession &session, bool en);
    bool GetEnabled(ChannelSession &session);

    void   SetScale(ChannelSession &session, double scale);
    double GetScale(ChannelSession &session);

}
