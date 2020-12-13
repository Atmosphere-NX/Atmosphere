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

    namespace {

        os::SdkMutex g_log_observer_lock;

        constexpr const char *g_log_severity_color_table[LogSeverity_Count] = {
            "\x1B[90m",        /* Dark gray for Trace. */
            "",                /* No special color for Info. */
            "\x1B[33m",        /* Yellow for Warn. */
            "\x1B[31m",        /* Red for Error. */
            "\x1B[41m\x1B[37m" /* White + red for Fatal. */
        };

        char g_default_log_observer_text_buffer[0xA0];

    }

    void DefaultLogObserver(const LogMetaData &log_metadata, const LogBody &log_body, void *user_data) {
        std::scoped_lock lk(g_log_observer_lock);

        const char *color_type = nullptr;
        /* Get the color according to the log severity. */
        if (log_metadata.log_severity < LogSeverity_Count) {
            color_type = g_log_severity_color_table[log_metadata.log_severity];
        }
        auto has_module_name = false;
        if (log_metadata.module_name != nullptr) {
            has_module_name = strlen(log_metadata.module_name) > 1; /* Avoid '$' as module name. */
        }

        if (!has_module_name && !color_type) {
            /* Just send the text log. */
            PrintDebugString(log_body.log_text, log_body.log_text_length);
            return;
        }

        size_t used_buffer_space = 0;
        if (has_module_name && log_body.log_is_head) {
            auto fmt_prefix = (color_type != nullptr) ? color_type : "";

            auto module_name = log_metadata.module_name;
            if (module_name[0] == '$') {
                /* Skip logging this initial character if it's present. */
                module_name++;
            }

            auto system_tick = os::GetSystemTick();
            auto tick_ts = os::ConvertToTimeSpan(system_tick);
            used_buffer_space += util::SNPrintf(g_default_log_observer_text_buffer, sizeof(g_default_log_observer_text_buffer), "%s%d:%02d:%02d.%03d [%-5s] ", fmt_prefix, tick_ts.GetHours(), tick_ts.GetMinutes(), tick_ts.GetSeconds(), tick_ts.GetMilliSeconds(), module_name);
        }
        else if (color_type) {
            used_buffer_space += util::SNPrintf(g_default_log_observer_text_buffer, sizeof(g_default_log_observer_text_buffer), "%s", color_type);
        }

        auto left_buffer_space = sizeof(g_default_log_observer_text_buffer) - used_buffer_space;
        if (color_type) {
            /* Also subtract the size of the color formatting, if present. */
            left_buffer_space -= strlen(color_type);
        }

        auto log_text_size = std::min(left_buffer_space, log_body.log_text_length);
        if (log_body.log_text_length && (log_body.log_text[log_body.log_text_length - 1] == '\n')) {
            /* If the last character is a newline character, skip it. */
            log_text_size--;
        }

        /* If we used a color before, reset it now. */
        auto fmt_prefix = (color_type != nullptr) ? "\x1B[0m" : "";

        used_buffer_space += util::SNPrintf(g_default_log_observer_text_buffer + used_buffer_space, left_buffer_space, "%.*s%s%s", log_text_size, log_body.log_text, fmt_prefix);
        PrintDebugString(g_default_log_observer_text_buffer, used_buffer_space);
    }

}
