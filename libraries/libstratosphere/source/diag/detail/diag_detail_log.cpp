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

namespace ams::diag::detail {

    void DefaultLogObserver(const LogMetaData &log_metadata, const LogBody &log_body, void *user_data);

    LogObserverHolder g_default_log_observer = { DefaultLogObserver, nullptr, true, nullptr };
    LogObserverHolder *g_log_observer_list_head = std::addressof(g_default_log_observer);

    void LogImpl(const LogMetaData &log_metadata, const char *fmt, ...) {
        std::va_list v;
        va_start(v, fmt);
        VLogImpl(log_metadata, fmt, v);
        va_end(v);
    }

    void VLogImpl(const LogMetaData &log_metadata, const char *fmt, std::va_list va_args) {
        /* TODO */
    }

    void CallAllLogObserver(const LogMetaData &log_metadata, const LogBody &log_body) {
        if (g_log_observer_list_head) {
            auto observer = g_log_observer_list_head;
            do {
                observer->log_function(log_metadata, log_body, observer->user_data);
                observer = observer->next;
            } while(observer);
        }
    }

}
