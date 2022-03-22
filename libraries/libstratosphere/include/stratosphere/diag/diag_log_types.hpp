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

namespace ams::diag {

    enum LogSeverity {
        LogSeverity_Trace = 0,
        LogSeverity_Info  = 1,
        LogSeverity_Warn  = 2,
        LogSeverity_Error = 3,
        LogSeverity_Fatal = 4,
    };

    struct SourceInfo {
        int line_number;
        const char *file_name;
        const char *function_name;
    };

    struct LogMetaData {
        SourceInfo source_info;
        const char *module_name;
        LogSeverity severity;
        int verbosity;
        bool use_default_locale_charset;
        void *additional_data;
        size_t additional_data_size;
    };

    struct LogBody {
        const char *message;
        size_t message_size;
        bool is_head;
        bool is_tail;
    };

    struct LogMessage {
        const char *fmt;
        std::va_list *vl;
    };

}
