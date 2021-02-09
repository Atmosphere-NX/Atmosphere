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
#include <stratosphere/diag/impl/diag_impl_string.hpp>

namespace ams::diag::impl {

    namespace {

        os::SdkMutex g_log_observer_lock;

        constexpr const char *g_log_severity_color_table[LogSeverity_Count] = {
            "\x1B[90m",        /* Dark gray for Trace. */
            nullptr,           /* No special color for Info. */
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

        /* Avoid '$' as module name. */
        const auto has_module_name = log_metadata.module_name != nullptr && std::strlen(log_metadata.module_name) > 1;

        if (!has_module_name && !color_type) {
            /* Just send the text log. */
            return PrintDebugString(log_body.text, log_body.text_length);
        }

        size_t used_buffer_space = 0;
        if (has_module_name && log_body.is_head) {
            const auto fmt_prefix = (color_type != nullptr) ? color_type : "";

            const char *module_name = log_metadata.module_name;
            if (module_name[0] == '$') {
                /* Skip logging this initial character if it's present. */
                module_name++;
            }

            const auto tick = os::ConvertToTimeSpan(os::GetSystemTick());
            used_buffer_space += util::SNPrintf(g_default_log_observer_text_buffer, sizeof(g_default_log_observer_text_buffer), "%s%d:%02d:%02d.%03d [%-5s] ", fmt_prefix, tick.GetHours(), tick.GetMinutes(), tick.GetSeconds(), tick.GetMilliSeconds(), module_name);
        } else if (color_type != nullptr) {
            used_buffer_space += util::SNPrintf(g_default_log_observer_text_buffer, sizeof(g_default_log_observer_text_buffer), "%s", color_type);
        }

        const auto max_buffer_size = sizeof(g_default_log_observer_text_buffer) - used_buffer_space - (color_type != nullptr ? std::strlen(color_type) - 1 : 0);

        auto log_text_size = std::min<size_t>(max_buffer_size, log_body.text_length);
        if (log_body.text_length && (log_body.text[log_body.text_length - 1] == '\n')) {
            /* If the last character is a newline character, skip it. */
            log_text_size--;
        }

        /* If we used a color before, reset it now. */
        used_buffer_space += util::SNPrintf(g_default_log_observer_text_buffer + used_buffer_space, max_buffer_size, "%.*s%s%s", log_text_size, log_body.text, (color_type != nullptr) ? "\x1B[0m" : "");
        PrintDebugString(g_default_log_observer_text_buffer, used_buffer_space);
    }

}
