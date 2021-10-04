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
#include <stratosphere.hpp>

namespace ams::htclow::ctrl {

    enum HtcctrlState : u32 {
        HtcctrlState_DriverConnected       =  0,
        HtcctrlState_SentConnectFromHost   =  1,
        HtcctrlState_Connected             =  2,
        HtcctrlState_SentReadyFromHost     =  3,
        HtcctrlState_Ready                 =  4,
        HtcctrlState_SentSuspendFromTarget =  5,
        HtcctrlState_EnterSleep            =  6,
        HtcctrlState_Sleep                 =  7,
        HtcctrlState_ExitSleep             =  8,
        HtcctrlState_SentResumeFromTarget  =  9,
        HtcctrlState_Disconnected          = 10,
        HtcctrlState_DriverDisconnected    = 11,
        HtcctrlState_Error                 = 12,
    };

    constexpr bool IsStateTransitionAllowed(HtcctrlState from, HtcctrlState to) {
        switch (from) {
            case HtcctrlState_DriverConnected:
                return to == HtcctrlState_SentConnectFromHost ||
                       to == HtcctrlState_Disconnected ||
                       to == HtcctrlState_DriverDisconnected ||
                       to == HtcctrlState_Error;
            case HtcctrlState_SentConnectFromHost:
                return to == HtcctrlState_Connected ||
                       to == HtcctrlState_Disconnected ||
                       to == HtcctrlState_DriverDisconnected ||
                       to == HtcctrlState_Error;
            case HtcctrlState_Connected:
                return to == HtcctrlState_SentReadyFromHost ||
                       to == HtcctrlState_Disconnected ||
                       to == HtcctrlState_DriverDisconnected ||
                       to == HtcctrlState_Error;
            case HtcctrlState_SentReadyFromHost:
                return to == HtcctrlState_Ready ||
                       to == HtcctrlState_Disconnected ||
                       to == HtcctrlState_DriverDisconnected ||
                       to == HtcctrlState_Error;
            case HtcctrlState_Ready:
                return to == HtcctrlState_SentSuspendFromTarget ||
                       to == HtcctrlState_Disconnected ||
                       to == HtcctrlState_DriverDisconnected ||
                       to == HtcctrlState_Error;
            case HtcctrlState_SentSuspendFromTarget:
                return to == HtcctrlState_EnterSleep ||
                       to == HtcctrlState_Disconnected ||
                       to == HtcctrlState_DriverDisconnected ||
                       to == HtcctrlState_Error;
            case HtcctrlState_EnterSleep:
                return to == HtcctrlState_Sleep ||
                       to == HtcctrlState_Disconnected ||
                       to == HtcctrlState_DriverDisconnected ||
                       to == HtcctrlState_Error;
            case HtcctrlState_Sleep:
                return to == HtcctrlState_ExitSleep;
            case HtcctrlState_ExitSleep:
                return to == HtcctrlState_SentResumeFromTarget ||
                       to == HtcctrlState_Disconnected ||
                       to == HtcctrlState_DriverDisconnected ||
                       to == HtcctrlState_Error;
            case HtcctrlState_SentResumeFromTarget:
                return to == HtcctrlState_Ready ||
                       to == HtcctrlState_Disconnected ||
                       to == HtcctrlState_DriverDisconnected ||
                       to == HtcctrlState_Error;
            case HtcctrlState_Disconnected:
                return to == HtcctrlState_SentConnectFromHost ||
                       to == HtcctrlState_Disconnected ||
                       to == HtcctrlState_DriverDisconnected ||
                       to == HtcctrlState_Error;
            case HtcctrlState_DriverDisconnected:
                return to == HtcctrlState_DriverConnected;
            case HtcctrlState_Error:
                return to == HtcctrlState_Disconnected ||
                       to == HtcctrlState_DriverDisconnected ||
                       to == HtcctrlState_Error;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    constexpr bool IsDisconnected(HtcctrlState state) {
        switch (state) {
            case HtcctrlState_Disconnected:
            case HtcctrlState_DriverDisconnected:
                return true;
            default:
                return false;
        }
    }

    constexpr bool IsConnecting(HtcctrlState state) {
        switch (state) {
            case HtcctrlState_DriverConnected:
            case HtcctrlState_SentConnectFromHost:
            case HtcctrlState_Disconnected:
                return true;
            default:
                return false;
        }
    }

    constexpr bool IsConnected(HtcctrlState state) {
        switch (state) {
            case HtcctrlState_Connected:
            case HtcctrlState_SentReadyFromHost:
            case HtcctrlState_Ready:
            case HtcctrlState_SentSuspendFromTarget:
            case HtcctrlState_EnterSleep:
            case HtcctrlState_Sleep:
            case HtcctrlState_ExitSleep:
            case HtcctrlState_SentResumeFromTarget:
                return true;
            default:
                return false;
        }
    }

    constexpr bool IsReadied(HtcctrlState state) {
        switch (state) {
            case HtcctrlState_Ready:
            case HtcctrlState_SentSuspendFromTarget:
            case HtcctrlState_EnterSleep:
            case HtcctrlState_Sleep:
            case HtcctrlState_ExitSleep:
            case HtcctrlState_SentResumeFromTarget:
                return true;
            default:
                return false;
        }
    }

    constexpr bool IsSleeping(HtcctrlState state) {
        switch (state) {
            case HtcctrlState_SentSuspendFromTarget:
            case HtcctrlState_EnterSleep:
            case HtcctrlState_Sleep:
            case HtcctrlState_ExitSleep:
            case HtcctrlState_SentResumeFromTarget:
                return true;
            default:
                return false;
        }
    }

}
