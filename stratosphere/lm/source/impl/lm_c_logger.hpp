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
#include <stratosphere.hpp>

namespace ams::lm::impl {

    using CLoggerLogFunction = bool(*)(void*, size_t);

    class CLogger {
        private:
            void *data_buffer;
            size_t cur_offset;
            uint64_t some_refcount;
            void *unk_ref_2;
            uint64_t unk_3;
            uint64_t unk_4;
            CLogger *self_ref;
            void *unk_ref_3;
            size_t data_buffer_size;
            CLoggerLogFunction log_fn;
            os::SdkMutex sdk_mutex_1;
            os::SdkMutex sdk_mutex_2;
            os::SdkConditionVariable condvar_1;
            os::SdkConditionVariable condvar_2;
            bool some_flag;
            uint8_t pad[7];
            uint64_t unk_flag;
        public:
            CLogger(void *data_buffer, size_t data_buffer_size, CLoggerLogFunction log_fn) : data_buffer(data_buffer), unk_ref_2(/*?*/ nullptr), unk_ref_3(/*?*/ nullptr), data_buffer_size(data_buffer_size), log_fn(log_fn) {}
            
            bool Log(const void *data, size_t data_size);
            bool SomeMemberFn();

            inline bool CopyLogData(const void *data, size_t data_size) {
                if ((this->data_buffer_size - this->cur_offset) >= data_size) {
                    auto out_buf = reinterpret_cast<u8*>(this->data_buffer) + this->cur_offset;
                    this->cur_offset += data_size;
                    ++this->some_refcount;
                    this->sdk_mutex_1.Unlock();
                    std::memcpy(out_buf, data, data_size);
                    this->sdk_mutex_1.Lock();
                    if((--this->some_refcount) == 0) {
                        this->condvar_2.Signal();
                    }
                    this->sdk_mutex_1.Unlock();
                    return true;
                }
                return false;
            }
    };

    CLogger *GetCLogger();

}