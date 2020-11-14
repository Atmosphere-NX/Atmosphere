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
                os::Event erpt_event;
                os::Event battery_event;
                ThrowContext context;
                FatalEventManager event_manager;
                bool has_thrown;
            private:
                Result TrySetHasThrown() {
                    R_UNLESS(!this->has_thrown, ResultAlreadyThrown());
                    this->has_thrown = true;
                    return ResultSuccess();
                }
            public:
                ServiceContext()
                    : erpt_event(os::EventClearMode_ManualClear), battery_event(os::EventClearMode_ManualClear),
                      context(std::addressof(erpt_event), std::addressof(battery_event)), has_thrown(false)
                {
                    /* ... */
                }

                Result GetEvent(const os::SystemEventType **out) {
                    return this->event_manager.GetEvent(out);
                }

                Result ThrowFatal(Result result, os::ProcessId process_id) {
                    return this->ThrowFatalWithCpuContext(result, process_id, FatalPolicy_ErrorReportAndErrorScreen, {});
                }

                Result ThrowFatalWithPolicy(Result result, os::ProcessId process_id, FatalPolicy policy) {
                    return this->ThrowFatalWithCpuContext(result, process_id, policy, {});
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

            /* At this point we have exclusive access to this->context. */
            this->context.result = result;
            this->context.cpu_ctx = cpu_ctx;

            /* Cap the stack trace to a sane limit. */
            if (cpu_ctx.architecture == CpuContext::Architecture_Aarch64) {
                this->context.cpu_ctx.aarch64_ctx.stack_trace_size = std::max(size_t(this->context.cpu_ctx.aarch64_ctx.stack_trace_size), aarch64::CpuContext::MaxStackTraceDepth);
            } else {
                this->context.cpu_ctx.aarch32_ctx.stack_trace_size = std::max(size_t(this->context.cpu_ctx.aarch32_ctx.stack_trace_size), aarch32::CpuContext::MaxStackTraceDepth);
            }

            /* Get program id. */
            pm::info::GetProgramId(&this->context.program_id, process_id);
            this->context.is_creport = (this->context.program_id == ncm::SystemProgramId::Creport);

            if (!this->context.is_creport) {
                /* On firmware version 2.0.0, use debugging SVCs to collect information. */
                if (hos::GetVersion() >= hos::Version_2_0_0) {
                    fatal::srv::TryCollectDebugInformation(&this->context, process_id);
                }
            } else {
                /* We received info from creport. Parse program id from afsr0. */
                if (cpu_ctx.architecture == CpuContext::Architecture_Aarch64) {
                    this->context.program_id = cpu_ctx.aarch64_ctx.GetProgramIdForAtmosphere();
                } else {
                    this->context.program_id = cpu_ctx.aarch32_ctx.GetProgramIdForAtmosphere();
                }
            }

            /* Decide whether to generate a report. */
            this->context.generate_error_report = (policy == FatalPolicy_ErrorReportAndErrorScreen);

            /* Adjust error code (2000-0000 -> 2162-0002). */
            if (R_SUCCEEDED(this->context.result)) {
                this->context.result = err::ResultSystemModuleAborted();
            }

            switch (policy) {
                case FatalPolicy_ErrorReportAndErrorScreen:
                case FatalPolicy_ErrorScreen:
                    /* Signal that we're throwing. */
                    this->event_manager.SignalEvents();

                    if (GetFatalConfig().ShouldTransitionToFatal()) {
                        RunTasks(&this->context);
                    }
                    break;
                /* N aborts here. Should we just return an error code? */
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            return ResultSuccess();
        }

    }

    Result ThrowFatalForSelf(Result result) {
        return g_context.ThrowFatalWithPolicy(result, os::GetCurrentProcessId(), FatalPolicy_ErrorScreen);
    }

    Result Service::ThrowFatal(Result result, const sf::ClientProcessId &client_pid) {
        return g_context.ThrowFatal(result, client_pid.GetValue());
    }

    Result Service::ThrowFatalWithPolicy(Result result, const sf::ClientProcessId &client_pid, FatalPolicy policy) {
        return g_context.ThrowFatalWithPolicy(result, client_pid.GetValue(), policy);
    }

    Result Service::ThrowFatalWithCpuContext(Result result, const sf::ClientProcessId &client_pid, FatalPolicy policy, const CpuContext &cpu_ctx) {
        return g_context.ThrowFatalWithCpuContext(result, client_pid.GetValue(), policy, cpu_ctx);
    }

    Result PrivateService::GetFatalEvent(sf::OutCopyHandle out_h) {
        const os::SystemEventType *event;
        R_TRY(g_context.GetEvent(std::addressof(event)));
        out_h.SetValue(os::GetReadableHandleOfSystemEvent(event));
        return ResultSuccess();
    }

}