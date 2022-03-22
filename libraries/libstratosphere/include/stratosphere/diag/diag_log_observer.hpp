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

    using LogObserver = void (*)(const LogMetaData &meta, const LogBody &body, void *arg);

    struct LogObserverHolder {
        LogObserver log_observer;
        LogObserverHolder *next;
        bool is_registered;
        void *arg;
    };

    constexpr inline void InitializeLogObserverHolder(LogObserverHolder *holder, LogObserver observer, void *arg) {
        holder->log_observer  = observer;
        holder->next          = nullptr;
        holder->is_registered = false;
        holder->arg           = arg;
    }

    void RegisterLogObserver(LogObserverHolder *holder);
    void UnregisterLogObserver(LogObserverHolder *holder);

}
