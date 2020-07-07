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
#include "erpt_srv_service.hpp"
#include "erpt_srv_context_impl.hpp"
#include "erpt_srv_session_impl.hpp"
#include "erpt_srv_stream.hpp"

namespace ams::erpt::srv {

    namespace {

        struct ErrorReportServerOptions {
            static constexpr size_t PointerBufferSize = 0;
            static constexpr size_t MaxDomains        = 64;
            static constexpr size_t MaxDomainObjects  = 2 * ReportCountMax + 5 + 2;
        };

        constexpr inline size_t ErrorReportNumServers      = 2;
        constexpr inline size_t ErrorReportReportSessions  = 5;
        constexpr inline size_t ErrorReportContextSessions = 10;
        constexpr inline size_t ErrorReportMaxSessions     = ErrorReportReportSessions + ErrorReportContextSessions;

        constexpr inline sm::ServiceName ErrorReportContextServiceName = sm::ServiceName::Encode("erpt:c");
        constexpr inline sm::ServiceName ErrorReportReportServiceName  = sm::ServiceName::Encode("erpt:r");

        alignas(os::ThreadStackAlignment) u8 g_server_thread_stack[16_KB];

        class ErrorReportServiceManager : public ams::sf::hipc::ServerManager<ErrorReportNumServers, ErrorReportServerOptions, ErrorReportMaxSessions> {
            private:
                os::ThreadType thread;
                std::shared_ptr<erpt::sf::IContext> context_session_object;
            private:
                static void ThreadFunction(void *_this) {
                    reinterpret_cast<ErrorReportServiceManager *>(_this)->SetupAndLoopProcess();
                }

                void SetupAndLoopProcess();
            public:
                ErrorReportServiceManager(erpt::srv::ContextImpl *c)
                    : context_session_object(ams::sf::GetSharedPointerTo<erpt::sf::IContext, erpt::srv::ContextImpl>(c))
                {
                    /* ... */
                }

                Result Initialize() {
                    R_ABORT_UNLESS((this->RegisterServer<erpt::sf::IContext, erpt::srv::ContextImpl>(ErrorReportContextServiceName, ErrorReportContextSessions, this->context_session_object)));
                    R_ABORT_UNLESS((this->RegisterServer<erpt::sf::ISession, erpt::srv::SessionImpl>(ErrorReportReportServiceName, ErrorReportReportSessions)));

                    this->ResumeProcessing();

                    R_ABORT_UNLESS(os::CreateThread(std::addressof(this->thread), ThreadFunction, this, g_server_thread_stack, sizeof(g_server_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(erpt, IpcServer)));
                    os::SetThreadNamePointer(std::addressof(this->thread), AMS_GET_SYSTEM_THREAD_NAME(erpt, IpcServer));

                    os::StartThread(std::addressof(this->thread));

                    return ResultSuccess();
                }

                void Wait() {
                    os::WaitThread(std::addressof(this->thread));
                }
        };

        void ErrorReportServiceManager::SetupAndLoopProcess() {
            const psc::PmModuleId dependencies[] = { psc::PmModuleId_Fs };
            psc::PmModule  pm_module;
            psc::PmState   pm_state;
            psc::PmFlagSet pm_flags;
            os::WaitableHolderType module_event_holder;

            R_ABORT_UNLESS(pm_module.Initialize(psc::PmModuleId_Erpt, dependencies, util::size(dependencies), os::EventClearMode_ManualClear));

            os::InitializeWaitableHolder(std::addressof(module_event_holder), pm_module.GetEventPointer()->GetBase());
            os::SetWaitableHolderUserData(std::addressof(module_event_holder), static_cast<uintptr_t>(psc::PmModuleId_Erpt));
            this->AddUserWaitableHolder(std::addressof(module_event_holder));

            while (true) {
                /* NOTE: Nintendo checks the user holder data to determine what's signaled, we will prefer to just check the address. */
                auto *signaled_holder = this->WaitSignaled();
                if (signaled_holder != std::addressof(module_event_holder)) {
                    R_ABORT_UNLESS(this->Process(signaled_holder));
                } else {
                    pm_module.GetEventPointer()->Clear();
                    if (R_SUCCEEDED(pm_module.GetRequest(std::addressof(pm_state), std::addressof(pm_flags)))) {
                        switch (pm_state) {
                            case psc::PmState_Awake:
                            case psc::PmState_ReadyAwaken:
                                Stream::EnableFsAccess(true);
                                break;
                            case psc::PmState_ReadySleep:
                            case psc::PmState_ReadyShutdown:
                                Stream::EnableFsAccess(false);
                                break;
                            default:
                                break;
                        }
                        R_ABORT_UNLESS(pm_module.Acknowledge(pm_state, ResultSuccess()));
                    } else {
                        AMS_ASSERT(false);
                    }
                    this->AddUserWaitableHolder(signaled_holder);
                }
            }
        }

        constinit erpt::srv::ContextImpl g_context_object;
        ErrorReportServiceManager g_erpt_server_manager(std::addressof(g_context_object));

    }

    Result InitializeService() {
        return g_erpt_server_manager.Initialize();
    }

    void WaitService() {
        return g_erpt_server_manager.Wait();
    }

}
