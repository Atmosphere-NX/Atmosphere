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
#include "lm_common.hpp"

namespace ams::lm::impl {

    class LogBuffer {
        NON_COPYABLE(LogBuffer);
        NON_MOVEABLE(LogBuffer);
        public:
            using FlushFunction = bool (*)(void *log_data, size_t log_data_size);
        private:
            void *data_buffer;
            size_t current_data_size;
            uint64_t some_count;
            LogBuffer *second_log_buffer;
            LogBuffer *main_target_buffer;
            LogBuffer *sub_target_buffer;
            size_t data_buffer_size;
            FlushFunction flush_func;
            os::SdkMutex main_target_buffer_lock;
            os::SdkMutex sub_target_buffer_lock;
            os::SdkConditionVariable flush_done_cv;
            os::SdkConditionVariable flush_ready_cv;
            bool some_flag;
            uint64_t some_count_2;
        public:
            LogBuffer(void *data_buffer, size_t data_buffer_size, FlushFunction flush_func, LogBuffer *second_log_buffer) : data_buffer(data_buffer), second_log_buffer(second_log_buffer), main_target_buffer(this), sub_target_buffer(second_log_buffer), data_buffer_size(data_buffer_size), flush_func(flush_func) {}
            
            bool Log(const void *log_data, size_t log_data_size, bool flush);
            bool Flush();

            inline bool DoFlush(LogBuffer *log_buffer) {
                auto ok = this->flush_func(log_buffer->data_buffer, log_buffer->current_data_size);
                if (ok) {
                    this->sub_target_buffer->current_data_size = 0;
                }
                return ok;
            }

            inline void SomeInlinedFn() {
                std::scoped_lock lk(this->main_target_buffer_lock);
                if (this->some_count_2 == 0) {
                    this->some_flag = true;
                    this->flush_done_cv.Broadcast();
                }
            }
    };

    LogBuffer *GetLogBuffer();

}