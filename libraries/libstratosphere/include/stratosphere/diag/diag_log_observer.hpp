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
#include <stratosphere/diag/diag_log_types.hpp>

namespace ams::diag {

    struct LogObserverHolder {
        using LogFunction = void (*)(const LogMetaData &log_metadata, const LogBody &log_body, void *user_data);

        LogFunction log_function;
        LogObserverHolder *next;
        bool is_registered;
        void *user_data;
    };

    void InitializeLogObserverHolder(LogObserverHolder *observer_holder, LogObserverHolder::LogFunction log_function, void *user_data);

    void RegisterLogObserver(LogObserverHolder *observer_holder);
    void UnregisterLogObserver(LogObserverHolder *observer_holder);

    void ResetDefaultLogObserver();
    void ReplaceDefaultLogObserver(LogObserverHolder::LogFunction log_function);

}
