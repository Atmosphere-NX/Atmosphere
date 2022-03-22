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
#include <stratosphere.hpp>
#include "erpt_srv_service.hpp"
#include "erpt_srv_context_impl.hpp"
#include "erpt_srv_session_impl.hpp"
#include "erpt_srv_stream.hpp"
#include "erpt_srv_forced_shutdown.hpp"

namespace ams::erpt::srv {

    extern ams::sf::ExpHeapAllocator g_sf_allocator;

    namespace {

        struct ErrorReportServerOptions {
            static constexpr size_t PointerBufferSize   = 0;
            static constexpr size_t MaxDomains          = 64;
            static constexpr size_t MaxDomainObjects    = 2 * ReportCountMax + 5 + 2;
            static constexpr bool CanDeferInvokeRequest = false;
            static constexpr bool CanManageMitmServers  = false;
        };

        constexpr inline size_t ErrorReportNumServers      = 2;
        constexpr inline size_t ErrorReportReportSessions  = 5;
        constexpr inline size_t ErrorReportContextSessions = 10;
        constexpr inline size_t ErrorReportMaxSessions     = ErrorReportReportSessions + ErrorReportContextSessions;

        constexpr inline sm::ServiceName ErrorReportContextServiceName = sm::ServiceName::Encode("erpt:c");
        constexpr inline sm::ServiceName ErrorReportReportServiceName  = sm::ServiceName::Encode("erpt:r");

        alignas(os::ThreadStackAlignment) constinit u8 g_server_thread_stack[16_KB];

        enum PortIndex {
            PortIndex_Report,
            PortIndex_Context,
        };

        class ErrorReportServiceManager : public ams::sf::hipc::ServerManager<ErrorReportNumServers, ErrorReportServerOptions, ErrorReportMaxSessions> {
            private:
                os::ThreadType m_thread;
                ams::sf::UnmanagedServiceObject<erpt::sf::IContext, erpt::srv::ContextImpl> m_context_session_object;
            private:
                static void ThreadFunction(void *_this) {
                    reinterpret_cast<ErrorReportServiceManager *>(_this)->SetupAndLoopProcess();
                }

                void SetupAndLoopProcess();

                virtual Result OnNeedsToAccept(int port_index, Server *server) override {
                    switch (port_index) {
                        case PortIndex_Report:
                            {
                                auto intf = ams::sf::ObjectFactory<ams::sf::ExpHeapAllocator::Policy>::CreateSharedEmplaced<erpt::sf::ISession, erpt::srv::SessionImpl>(std::addressof(g_sf_allocator));
                                AMS_ABORT_UNLESS(intf != nullptr);
                                return this->AcceptImpl(server, intf);
                            }
                        case PortIndex_Context:
                            return AcceptImpl(server, m_context_session_object.GetShared());
                        default:
                            return erpt::ResultNotSupported();
                    }
                }
            public:
                Result Initialize() {
                    R_ABORT_UNLESS(this->RegisterServer(PortIndex_Context, ErrorReportContextServiceName, ErrorReportContextSessions));
                    R_ABORT_UNLESS(this->RegisterServer(PortIndex_Report,  ErrorReportReportServiceName, ErrorReportReportSessions));

                    this->ResumeProcessing();

                    R_ABORT_UNLESS(os::CreateThread(std::addressof(m_thread), ThreadFunction, this, g_server_thread_stack, sizeof(g_server_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(erpt, IpcServer)));
                    os::SetThreadNamePointer(std::addressof(m_thread), AMS_GET_SYSTEM_THREAD_NAME(erpt, IpcServer));

                    os::StartThread(std::addressof(m_thread));

                    return ResultSuccess();
                }

                void Wait() {
                    os::WaitThread(std::addressof(m_thread));
                }
        };

        void ErrorReportServiceManager::SetupAndLoopProcess() {
            const psc::PmModuleId dependencies[] = { psc::PmModuleId_Fs };
            psc::PmModule  pm_module;
            psc::PmState   pm_state;
            psc::PmFlagSet pm_flags;
            os::MultiWaitHolderType module_event_holder;

            R_ABORT_UNLESS(pm_module.Initialize(psc::PmModuleId_Erpt, dependencies, util::size(dependencies), os::EventClearMode_ManualClear));

            os::InitializeMultiWaitHolder(std::addressof(module_event_holder), pm_module.GetEventPointer()->GetBase());
            os::SetMultiWaitHolderUserData(std::addressof(module_event_holder), static_cast<uintptr_t>(psc::PmModuleId_Erpt));
            this->AddUserMultiWaitHolder(std::addressof(module_event_holder));

            while (true) {
                /* NOTE: Nintendo checks the user holder data to determine what's signaled, we will prefer to just check the address. */
                auto *signaled_holder = this->WaitSignaled();
                if (signaled_holder != std::addressof(module_event_holder)) {
                    R_ABORT_UNLESS(this->Process(signaled_holder));
                } else {
                    pm_module.GetEventPointer()->Clear();
                    if (R_SUCCEEDED(pm_module.GetRequest(std::addressof(pm_state), std::addressof(pm_flags)))) {
                        switch (pm_state) {
                            case psc::PmState_FullAwake:
                            case psc::PmState_MinimumAwake:
                                Stream::EnableFsAccess(true);
                                break;
                            case psc::PmState_ShutdownReady:
                                FinalizeForcedShutdownDetection();
                                [[fallthrough]];
                            case psc::PmState_SleepReady:
                                Stream::EnableFsAccess(false);
                                break;
                            default:
                                break;
                        }
                        R_ABORT_UNLESS(pm_module.Acknowledge(pm_state, ResultSuccess()));
                    } else {
                        AMS_ASSERT(false);
                    }
                    this->AddUserMultiWaitHolder(signaled_holder);
                }
            }
        }

        constinit util::TypedStorage<ErrorReportServiceManager> g_erpt_server_manager;

    }

    Result InitializeService() {
        util::ConstructAt(g_erpt_server_manager);
        return util::GetReference(g_erpt_server_manager).Initialize();
    }

    void WaitService() {
        return util::GetReference(g_erpt_server_manager).Wait();
    }

}
