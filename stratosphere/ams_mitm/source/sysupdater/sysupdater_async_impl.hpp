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
#include "sysupdater_thread_allocator.hpp"

namespace ams::mitm::sysupdater {

    class ErrorContextHolder {
        private:
            err::ErrorContext error_context;
        public:
            constexpr ErrorContextHolder() : error_context{} { /* ... */ }

            virtual ~ErrorContextHolder() { /* ... */ }

            template<typename T>
            Result SaveErrorContextIfFailed(T &async, Result result) {
                if (R_FAILED(result)) {
                    async.GetErrorContext(std::addressof(this->error_context));
                    return result;
                }

                return ResultSuccess();
            }

            template<typename T>
            Result GetAndSaveErrorContext(T &async) {
                R_TRY(this->SaveErrorContextIfFailed(async, async.Get()));
                return ResultSuccess();
            }

            template<typename T>
            Result SaveInternalTaskErrorContextIfFailed(T &async, Result result) {
                if (R_FAILED(result)) {
                    async.CreateErrorContext(std::addressof(this->error_context));
                    return result;
                }

                return ResultSuccess();
            }

            const err::ErrorContext &GetErrorContextImpl() {
                return this->error_context;
            }
    };

    class AsyncBase {
        public:
            virtual ~AsyncBase() { /* ... */ }

            static Result ToAsyncResult(Result result);

            Result Cancel() {
                this->CancelImpl();
                return ResultSuccess();
            }

            virtual Result GetErrorContext(sf::Out<err::ErrorContext> out) {
                *out = {};
                return ResultSuccess();
            }
        private:
            virtual void CancelImpl() = 0;
    };

    class AsyncResultBase : public AsyncBase {
        public:
            virtual ~AsyncResultBase() { /* ... */ }

            Result Get() {
                return ToAsyncResult(this->GetImpl());
            }
        private:
            virtual Result GetImpl() = 0;
    };
    static_assert(ns::impl::IsIAsyncResult<AsyncResultBase>);

    /* NOTE: Based off of ns AsyncPrepareCardUpdateImpl. */
    /* We don't implement the RequestServer::ManagedStop details, as we don't implement stoppable request list. */
    class AsyncPrepareSdCardUpdateImpl : public AsyncResultBase, private ErrorContextHolder {
        private:
            Result result;
            os::SystemEvent event;
            std::optional<ThreadInfo> thread_info;
            ncm::InstallTaskBase *task;
        public:
            AsyncPrepareSdCardUpdateImpl(ncm::InstallTaskBase *task) : result(ResultSuccess()), event(os::EventClearMode_ManualClear, true), thread_info(), task(task) { /* ... */ }
            virtual ~AsyncPrepareSdCardUpdateImpl();

            os::SystemEvent &GetEvent() { return this->event; }

            virtual Result GetErrorContext(sf::Out<err::ErrorContext> out) override {
                *out = ErrorContextHolder::GetErrorContextImpl();
                return ResultSuccess();
            }

            Result Run();
        private:
            Result Execute();

            virtual void CancelImpl() override;
            virtual Result GetImpl() override { return this->result; }
    };

}
