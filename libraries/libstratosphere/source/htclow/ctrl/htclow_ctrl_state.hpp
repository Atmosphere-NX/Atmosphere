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
#include <stratosphere.hpp>

namespace ams::htclow::ctrl {

    enum HtcctrlState : u32 {
        HtcctrlState_0  =  0,
        HtcctrlState_1  =  1,
        HtcctrlState_2  =  2,
        HtcctrlState_3  =  3,
        HtcctrlState_4  =  4,
        HtcctrlState_5  =  5,
        HtcctrlState_6  =  6,
        HtcctrlState_7  =  7,
        HtcctrlState_8  =  8,
        HtcctrlState_9  =  9,
        HtcctrlState_10 = 10,
        HtcctrlState_11 = 11,
        HtcctrlState_12 = 12,
    };

    constexpr bool IsStateTransitionAllowed(HtcctrlState from, HtcctrlState to) {
        switch (from) {
            case HtcctrlState_0:
                return to == HtcctrlState_1 || to == HtcctrlState_10 || to == HtcctrlState_11 || to == HtcctrlState_12;
            case HtcctrlState_1:
                return to == HtcctrlState_2 || to == HtcctrlState_10 || to == HtcctrlState_11 || to == HtcctrlState_12;
            case HtcctrlState_2:
                return to == HtcctrlState_3 || to == HtcctrlState_10 || to == HtcctrlState_11 || to == HtcctrlState_12;
            case HtcctrlState_3:
                return to == HtcctrlState_4 || to == HtcctrlState_10 || to == HtcctrlState_11 || to == HtcctrlState_12;
            case HtcctrlState_4:
                return to == HtcctrlState_5 || to == HtcctrlState_10 || to == HtcctrlState_11 || to == HtcctrlState_12;
            case HtcctrlState_5:
                return to == HtcctrlState_6 || to == HtcctrlState_10 || to == HtcctrlState_11 || to == HtcctrlState_12;
            case HtcctrlState_6:
                return to == HtcctrlState_7 || to == HtcctrlState_10 || to == HtcctrlState_11 || to == HtcctrlState_12;
            case HtcctrlState_7:
                return to == HtcctrlState_8;
            case HtcctrlState_8:
                return to == HtcctrlState_9 || to == HtcctrlState_10 || to == HtcctrlState_11 || to == HtcctrlState_12;
            case HtcctrlState_9:
                return to == HtcctrlState_4 || to == HtcctrlState_10 || to == HtcctrlState_11 || to == HtcctrlState_12;
            case HtcctrlState_10:
                return to == HtcctrlState_1 || to == HtcctrlState_10 || to == HtcctrlState_11 || to == HtcctrlState_12;
            case HtcctrlState_11:
                return to == HtcctrlState_0;
            case HtcctrlState_12:
                return to == HtcctrlState_10 || to == HtcctrlState_11 || to == HtcctrlState_12;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    constexpr bool IsDisconnected(HtcctrlState state) {
        switch (state) {
            case HtcctrlState_10:
            case HtcctrlState_11:
                return true;
            default:
                return false;
        }
    }

    constexpr bool IsConnecting(HtcctrlState state) {
        switch (state) {
            case HtcctrlState_0:
            case HtcctrlState_1:
            case HtcctrlState_10:
                return true;
            default:
                return false;
        }
    }

    constexpr bool IsConnected(HtcctrlState state) {
        switch (state) {
            case HtcctrlState_2:
            case HtcctrlState_3:
            case HtcctrlState_4:
            case HtcctrlState_5:
            case HtcctrlState_6:
            case HtcctrlState_7:
            case HtcctrlState_8:
            case HtcctrlState_9:
                return true;
            default:
                return false;
        }
    }

    constexpr bool IsReadied(HtcctrlState state) {
        switch (state) {
            case HtcctrlState_4:
            case HtcctrlState_5:
            case HtcctrlState_6:
            case HtcctrlState_7:
            case HtcctrlState_8:
            case HtcctrlState_9:
                return true;
            default:
                return false;
        }
    }

    constexpr bool IsSleeping(HtcctrlState state) {
        switch (state) {
            case HtcctrlState_5:
            case HtcctrlState_6:
            case HtcctrlState_7:
            case HtcctrlState_8:
            case HtcctrlState_9:
                return true;
            default:
                return false;
        }
    }

}
