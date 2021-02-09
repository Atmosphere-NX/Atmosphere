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

namespace ams::diag::impl {

    void LogImpl(const LogMetaData &log_metadata, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
    void VLogImpl(const LogMetaData &log_metadata, const char *fmt, std::va_list vl);
    void CallAllLogObserver(const LogMetaData &log_metadata, const LogBody &log_body);

    #define AMS_LOG(fmt, ...) ({ \
        constexpr ::ams::diag::LogMetaData log_metadata = { \
            .source_info = { \
                .line_number = __LINE__, \
                .file_name = __FILE__, \
                .function_name = AMS_CURRENT_FUNCTION_NAME \
            }, \
            .log_severity = ::ams::diag::LogSeverity_Info \
        }; \
        ::ams::diag::impl::LogImpl(log_metadata, fmt, __VA_ARGS__); \
    })

}
