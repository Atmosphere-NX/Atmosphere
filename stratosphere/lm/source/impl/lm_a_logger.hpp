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

    using ALoggerSomeFunction = void(*)(bool);

    class ALogger {
        private:
            os::SdkMutex sdk_mutex;
            bool some_flag;
            bool sd_card_mounted;
            bool some_flag_3;
            char log_file_path[128];
            bool some_flag_4;
            size_t log_file_offset;
            ALoggerSomeFunction some_fn;
            size_t unk;
        public:
            ALogger() : sdk_mutex(), some_flag(false), sd_card_mounted(false), some_flag_3(false), log_file_path{}, some_flag_4(false), log_file_offset(0), some_fn(nullptr), unk(0) {}
            
            void Dispose();

            bool EnsureLogFile();
            bool SaveLog(void *buf, size_t size);

            void SetSomeFunction(ALoggerSomeFunction fn) {
                std::scoped_lock lk(this->sdk_mutex);
                this->some_fn = fn;
            }

            void CallSomeFunction(bool flag) {
                std::scoped_lock lk(this->sdk_mutex);
                if(this->some_fn != nullptr) {
                    (this->some_fn)(flag);
                }
            }
    };

    ALogger *GetALogger();

}