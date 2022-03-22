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
#include "fatal_config.hpp"
#include "fatal_debug.hpp"
#include "fatal_service.hpp"
#include "fatal_service_for_self.hpp"
#include "fatal_event_manager.hpp"
#include "fatal_task.hpp"

namespace ams::fatal::srv {

    namespace {

        /* Service Context. */
        class ServiceContext {
            private:
                os::Event m_erpt_event;
                os::Event m_battery_event;
                ThrowContext m_context;
                FatalEventManager m_event_manager;
                bool m_has_thrown;
            private:
                Result TrySetHasThrown() {
                    R_UNLESS(!m_has_thrown, fatal::ResultAlreadyThrown());
                    m_has_thrown = true;
                    R_SUCCEED();
                }
            public:
                ServiceContext()
                    : m_erpt_event(os::EventClearMode_ManualClear), m_battery_event(os::EventClearMode_ManualClear),
                      m_context(std::addressof(m_erpt_event), std::addressof(m_battery_event)), m_has_thrown(false)
                {
                    /* ... */
                }

                Result GetEvent(const os::SystemEventType **out) {
                    R_RETURN(m_event_manager.GetEvent(out));
                }

                Result GetThrowContext(Result *out_error, ncm::ProgramId *out_program_id, FatalPolicy *out_policy, CpuContext *out_ctx) {
                    /* Set the output. */
                    *out_error      = m_context.result;
                    *out_program_id = m_context.throw_program_id;
                    *out_policy     = m_context.policy;
                    *out_ctx        = m_context.cpu_ctx;

                    R_SUCCEED();
                }

                Result ThrowFatal(Result result, os::ProcessId process_id) {
                    R_RETURN(this->ThrowFatalWithCpuContext(result, process_id, FatalPolicy_ErrorReportAndErrorScreen, {}));
                }

                Result ThrowFatalWithPolicy(Result result, os::ProcessId process_id, FatalPolicy policy) {
                    R_RETURN(this->ThrowFatalWithCpuContext(result, process_id, policy, {}));
                }

                Result ThrowFatalWithCpuContext(Result result, os::ProcessId process_id, FatalPolicy policy, const CpuContext &cpu_ctx);
        };

        /* Context global. */
        ServiceContext g_context;

        /* Throw implementation. */
        Result ServiceContext::ThrowFatalWithCpuContext(Result result, os::ProcessId process_id, FatalPolicy policy, const CpuContext &cpu_ctx) {
            /* We don't support Error-Report-only fatals. */
            R_SUCCEED_IF(policy == FatalPolicy_ErrorReport);

            /* Note that we've thrown fatal. */
            R_TRY(this->TrySetHasThrown());

            /* At this point we have exclusive access to m_context. */
            m_context.result  = result;
            m_context.cpu_ctx = cpu_ctx;
            m_context.policy  = policy;

            /* Cap the stack trace to a sane limit. */
            if (cpu_ctx.architecture == CpuContext::Architecture_Aarch64) {
                m_context.cpu_ctx.aarch64_ctx.stack_trace_size = std::max(size_t(m_context.cpu_ctx.aarch64_ctx.stack_trace_size), aarch64::CpuContext::MaxStackTraceDepth);
            } else {
                m_context.cpu_ctx.aarch32_ctx.stack_trace_size = std::max(size_t(m_context.cpu_ctx.aarch32_ctx.stack_trace_size), aarch32::CpuContext::MaxStackTraceDepth);
            }

            /* Get program id. */
            pm::info::GetProgramId(std::addressof(m_context.throw_program_id), process_id);
            m_context.is_creport = (m_context.throw_program_id == ncm::SystemProgramId::Creport);


            if (!m_context.is_creport) {
                m_context.program_id = m_context.throw_program_id;

                /* On firmware version 2.0.0, use debugging SVCs to collect information. */
                if (hos::GetVersion() >= hos::Version_2_0_0) {
                    fatal::srv::TryCollectDebugInformation(std::addressof(m_context), process_id);
                }
            } else {
                /* We received info from creport. Parse program id from afsr0. */
                if (cpu_ctx.architecture == CpuContext::Architecture_Aarch64) {
                    m_context.program_id = cpu_ctx.aarch64_ctx.GetProgramIdForAtmosphere();
                } else {
                    m_context.program_id = cpu_ctx.aarch32_ctx.GetProgramIdForAtmosphere();
                }
            }

            /* Decide whether to generate a report. */
            m_context.generate_error_report = (policy == FatalPolicy_ErrorReportAndErrorScreen);

            /* Adjust error code (ResultSuccess()/2000-0000 -> err::ResultSystemProgramAbort()/2162-0002). */
            if (R_SUCCEEDED(m_context.result)) {
                m_context.result = err::ResultSystemProgramAbort();
            }

            switch (policy) {
                case FatalPolicy_ErrorReportAndErrorScreen:
                case FatalPolicy_ErrorScreen:
                    /* Signal that we're throwing. */
                    m_event_manager.SignalEvents();

                    if (GetFatalConfig().ShouldTransitionToFatal()) {
                        RunTasks(std::addressof(m_context));
                    }
                    break;
                /* N aborts here. Should we just return an error code? */
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            R_SUCCEED();
        }

    }

    Result ThrowFatalForSelf(Result result) {
        R_RETURN(g_context.ThrowFatalWithPolicy(result, os::GetCurrentProcessId(), FatalPolicy_ErrorScreen));
    }

    Result Service::ThrowFatal(Result result, const sf::ClientProcessId &client_pid) {
        R_RETURN(g_context.ThrowFatal(result, client_pid.GetValue()));
    }

    Result Service::ThrowFatalWithPolicy(Result result, const sf::ClientProcessId &client_pid, FatalPolicy policy) {
        R_RETURN(g_context.ThrowFatalWithPolicy(result, client_pid.GetValue(), policy));
    }

    Result Service::ThrowFatalWithCpuContext(Result result, const sf::ClientProcessId &client_pid, FatalPolicy policy, const CpuContext &cpu_ctx) {
        R_RETURN(g_context.ThrowFatalWithCpuContext(result, client_pid.GetValue(), policy, cpu_ctx));
    }

    Result Service::GetFatalEvent(sf::OutCopyHandle out_h) {
        const os::SystemEventType *event;
        R_TRY(g_context.GetEvent(std::addressof(event)));
        out_h.SetValue(os::GetReadableHandleOfSystemEvent(event), false);
        R_SUCCEED();
    }

    Result Service::GetFatalContext(sf::Out<Result> out_error, sf::Out<ncm::ProgramId> out_program_id, sf::Out<fatal::FatalPolicy> out_policy, sf::Out<fatal::CpuContext> out_ctx) {
        R_RETURN(g_context.GetThrowContext(out_error.GetPointer(), out_program_id.GetPointer(), out_policy.GetPointer(), out_ctx.GetPointer()));
    }

}