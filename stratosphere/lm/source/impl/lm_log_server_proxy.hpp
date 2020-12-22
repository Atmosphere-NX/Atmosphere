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
#include "lm_sd_card_logging.hpp"

namespace ams::lm::impl {

    class LogServerProxy {
        NON_COPYABLE(LogServerProxy);
        NON_MOVEABLE(LogServerProxy);
        private:
            alignas(os::ThreadStackAlignment) u8 htcs_thread_stack[4_KB];
            os::ThreadType htcs_thread;
            os::SdkConditionVariable some_cond_var;
            os::Event finalize_event;
            os::SdkMutex some_cond_var_lock;
            os::SdkMutex update_enabled_fn_lock;
            std::atomic_int htcs_server_fd;
            std::atomic_int htcs_client_fd;
            UpdateEnabledFunction update_enabled_fn;
        public:
            LogServerProxy() : htcs_thread_stack{}, htcs_thread(), some_cond_var(), finalize_event(os::EventClearMode_ManualClear), some_cond_var_lock(), update_enabled_fn_lock(), htcs_server_fd(INT_MAX), htcs_client_fd(INT_MAX), update_enabled_fn(nullptr) /*, data_4(0), data_5{}*/ {}

            void StartHtcsThread(ThreadFunc htcs_entry);
            void DisposeHtcsThread();
            bool LogOverHtcs(const void *log_data, size_t log_size);

            void SetUpdateEnabledFunction(UpdateEnabledFunction update_enabled_fn) {
                std::scoped_lock lk(this->update_enabled_fn_lock);
                this->update_enabled_fn = update_enabled_fn;
            }

            inline void SetHtcsClientFd(int fd) {
                this->htcs_client_fd = fd;
            }
            
            inline void SetHtcsServerFd(int fd) {
                this->htcs_server_fd = fd;
            }

            inline os::Event &GetFinalizeEvent() {
                return this->finalize_event;
            }

            void DoHtcsLoopThing() {
                std::scoped_lock lk(this->some_cond_var_lock);
                this->some_cond_var.Broadcast();
            }

            void SetEnabled(bool enabled) {
                std::scoped_lock lk(this->update_enabled_fn_lock);
                if (this->update_enabled_fn) {
                    this->update_enabled_fn(enabled);
                }
            }
    };

    LogServerProxy *GetLogServerProxy();

}