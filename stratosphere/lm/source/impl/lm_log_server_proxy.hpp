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
#include "lm_a_logger.hpp"

namespace ams::lm::impl {

    class LogServerProxy {
        private:
            alignas(os::ThreadStackAlignment) u8 htcs_thread_stack[4_KB];
            os::ThreadType htcs_thread;
            os::SdkConditionVariable some_cond_var;
            os::Event finalize_event;
            os::SdkMutex some_cond_var_lock;
            os::SdkMutex some_fn_lock;
            u32 unk_3;
            u32 unk_4;
            ALoggerSomeFunction some_fn;
            u64 data_4;
            char data_5[3566];
        public:
            LogServerProxy() : htcs_thread_stack{}, htcs_thread(), some_cond_var(), finalize_event(os::EventClearMode_ManualClear), some_cond_var_lock(), some_fn_lock(), unk_3(0), some_fn(nullptr), data_4(0), data_5{} {}

            void StartHtcsThread(ThreadFunc htcs_entry) {
                R_ABORT_UNLESS(os::CreateThread(std::addressof(this->htcs_thread), htcs_entry, this, this->htcs_thread_stack, sizeof(this->htcs_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(lm, HtcsConnection)));
                os::SetThreadNamePointer(std::addressof(this->htcs_thread), AMS_GET_SYSTEM_THREAD_NAME(lm, HtcsConnection));
                os::StartThread(std::addressof(this->htcs_thread));
            }

            void DisposeHtcsThread() {
                this->finalize_event.Signal();
                /* nn::htcs::Close(<htcs-fd>); */
                os::WaitThread(std::addressof(this->htcs_thread));
                os::DestroyThread(std::addressof(this->htcs_thread));
            }

            void SetSomeFunction(ALoggerSomeFunction some_fn) {
                std::scoped_lock lk(this->some_fn_lock);
                this->some_fn = some_fn;
            }

            os::Event &GetFinalizeEvent() {
                return this->finalize_event;
            }
    };

    LogServerProxy *GetLogServerProxy();

}