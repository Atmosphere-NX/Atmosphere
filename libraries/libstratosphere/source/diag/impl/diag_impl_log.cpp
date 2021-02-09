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
#include <stratosphere/diag/diag_log_observer.hpp>
#include <stratosphere/diag/impl/diag_impl_log.hpp>
#include <stratosphere/diag/impl/diag_impl_string.hpp>

namespace ams::diag::impl {

    namespace {

        struct LogPrintData {
            char *print_str_start;
            char *print_str_end;
            char *print_str_cur;
            const LogMetaData &log_metadata;
            bool is_head;
            char buffer[40];
        };

        void PutCharacters(void *user_data, const char *chars, int chars_len) {
            auto print_data = reinterpret_cast<LogPrintData*>(user_data);
            if (chars_len > 0) {
                while (true) {
                    if (print_data->print_str_cur >= print_data->print_str_end) {
                        break;
                    }

                    const auto copy_len = std::min(static_cast<size_t>(chars_len), static_cast<size_t>(print_data->print_str_end - print_data->print_str_cur));
                    std::memcpy(print_data->print_str_cur, chars, copy_len);
                    chars_len -= copy_len;
                    chars += copy_len;
                    print_data->print_str_cur += copy_len;
                    if (chars_len == 0) {
                        return;
                    }
                }
                if (!print_data->log_metadata.use_default_locale_charset) {
                    const auto cur_print_str_len = static_cast<size_t>(print_data->print_str_cur - print_data->print_str_start);
                    const auto actual_print_str_len = GetValidSizeAsUtf8String(print_data->print_str_start, cur_print_str_len);
                    if (actual_print_str_len < cur_print_str_len) {
                        auto print_str_cur_end = print_data->print_str_start + actual_print_str_len;
                        *print_str_cur_end = '\0';

                        const LogBody log_body = {
                            .text = print_data->print_str_start,
                            .text_length = actual_print_str_len,
                            .is_head = print_data->is_head,
                            .is_tail = false,
                        };
                        CallAllLogObserver(print_data->log_metadata, log_body);
                        auto move_len = cur_print_str_len - actual_print_str_len;
                        std::memmove(print_data->print_str_start, print_str_cur_end, move_len);
                        print_data->print_str_cur = print_data->print_str_start + move_len;
                        print_data->is_head = false;
                        AMS_ASSERT(print_data->print_str_cur < print_data->print_str_end);
                    }
                }
                *print_data->print_str_cur = '\0';

                const LogBody log_body = {
                    .text = print_data->print_str_start,
                    .text_length = static_cast<size_t>(print_data->print_str_cur - print_data->print_str_start),
                    .is_head = print_data->is_head,
                    .is_tail = false,
                };
                CallAllLogObserver(print_data->log_metadata, log_body);
                print_data->print_str_cur = print_data->print_str_start;
                print_data->is_head = false;
                AMS_ASSERT(print_data->print_str_cur < print_data->print_str_end);
            }
        }

    }

    void DefaultLogObserver(const LogMetaData &log_metadata, const LogBody &log_body, void *user_data);

    LogObserverHolder g_default_log_observer = { DefaultLogObserver, nullptr, true, nullptr };
    LogObserverHolder *g_log_observer_list_head = std::addressof(g_default_log_observer);

    void LogImpl(const LogMetaData &log_metadata, const char *fmt, ...) {
        std::va_list vl;
        va_start(vl, fmt);
        VLogImpl(log_metadata, fmt, vl);
        va_end(vl);
    }

    void VLogImpl(const LogMetaData &log_metadata, const char *fmt, std::va_list vl) {
        LogPrintData print_data = {
            .print_str_start = print_data.buffer,
            .print_str_end = static_cast<char*>(print_data.buffer) + sizeof(print_data.buffer),
            .print_str_cur = print_data.buffer,
            .log_metadata = log_metadata,
            .is_head = true
        };
        
        util::VFormatString(PutCharacters, std::addressof(print_data), fmt, vl);

        const LogBody log_body = {
            .text = print_data.print_str_start,
            .text_length = static_cast<size_t>(print_data.print_str_cur - print_data.print_str_start),
            .is_head = print_data.is_head,
            .is_tail = true,
        };
        CallAllLogObserver(print_data.log_metadata, log_body);
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
