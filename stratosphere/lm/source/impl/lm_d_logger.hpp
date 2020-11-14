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
#include "../detail/lm_log_packet.hpp"

namespace ams::lm::impl {

    using DLoggerSomeFunction = bool(*)(void*, size_t);
    using LogGetterWriteFunction = bool(*)(void*, size_t, u64);

    class DLogger {
        private:
            void *log_data_buffer;
            size_t size;
            size_t cur_offset;
            DLoggerSomeFunction some_fn;
            bool unk_flag;
        public:
            DLogger(void *log_data_buf, size_t size, DLoggerSomeFunction some_fn) : log_data_buffer(log_data_buf), size(size), cur_offset(0), some_fn(some_fn), unk_flag(false) {}

            bool HasLogData();
            bool Log(void *log_data, size_t size);
    };

    DLogger *GetDLogger();

    bool DLoggerLogGetterWriteFunction(void *log_data, size_t size, u64 unk_param);
    
    void WriteLogToLogGetter(const void *log_data, size_t size, LogGetterWriteFunction write_fn, u64 unk_param);
    size_t ReadLogFromLogGetter(void *out_log_data, size_t size, u64 *out_packet_drop_count);

    inline void WriteLogToLogGetterDefault(const void *log_data, size_t size) {
        WriteLogToLogGetter(log_data, size, DLoggerLogGetterWriteFunction, 0);
    }

}