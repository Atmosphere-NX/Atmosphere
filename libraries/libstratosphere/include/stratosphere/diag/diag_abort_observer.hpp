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
#include <vapours.hpp>
#include <stratosphere/diag/diag_log_types.hpp>

namespace ams::diag {

    using AbortObserver = void (*)(const AbortInfo &);

    struct AbortObserverHolder {
        AbortObserver observer;
        AbortObserverHolder *next;
        bool is_registered;
    };

    void InitializeAbortObserverHolder(AbortObserverHolder *holder, AbortObserver observer);

    void RegisterAbortObserver(AbortObserverHolder *holder);
    void UnregisterAbortObserver(AbortObserverHolder *holder);
    void EnableDefaultAbortObserver(bool en);

    struct SdkAbortInfo {
        AbortInfo abort_info;
        Result result;
        const ::ams::os::UserExceptionInfo *exc_info;
    };

    using SdkAbortObserver = void (*)(const SdkAbortInfo &);

    struct SdkAbortObserverHolder {
        SdkAbortObserver observer;
        SdkAbortObserverHolder *next;
        bool is_registered;
    };

    void InitializeSdkAbortObserverHolder(SdkAbortObserverHolder *holder, SdkAbortObserver observer);

    void RegisterSdkAbortObserver(SdkAbortObserverHolder *holder);
    void UnregisterSdkAbortObserver(SdkAbortObserverHolder *holder);

}
