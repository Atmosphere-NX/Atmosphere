/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include "fatal_config.hpp"
#include "fatal_debug.hpp"
#include "fatal_service.hpp"
#include "fatal_service_for_self.hpp"
#include "fatal_event_manager.hpp"
#include "fatal_task.hpp"

namespace sts::fatal::srv {

    namespace {

        /* Service Context. */
        class ServiceContext {
            private:
                ThrowContext context;
                FatalEventManager event_manager;
                bool has_thrown;
            private:
                Result TrySetHasThrown() {
                    if (this->has_thrown) {
                        return ResultFatalAlreadyThrown;
                    }
                    this->has_thrown = true;
                    return ResultSuccess;
                }
            public:
                ServiceContext() {
                    this->context.ClearState();
                    R_ASSERT(eventCreate(&this->context.erpt_event, false));
                    R_ASSERT(eventCreate(&this->context.battery_event, false));
                    this->has_thrown = false;
                }

                Result GetEvent(Handle *out) {
                    return this->event_manager.GetEvent(out);
                }

                Result ThrowFatal(u32 error_code, u64 process_id) {
                    return this->ThrowFatalWithCpuContext(error_code, process_id, FatalType_ErrorReportAndErrorScreen, {});
                }

                Result ThrowFatalWithPolicy(u32 error_code, u64 process_id, FatalType policy) {
                    return this->ThrowFatalWithCpuContext(error_code, process_id, policy, {});
                }

                Result ThrowFatalWithCpuContext(u32 error_code, u64 process_id, FatalType policy, const CpuContext &cpu_ctx);
        };

        /* Context global. */
        ServiceContext g_context;

        /* Throw implementation. */
        Result ServiceContext::ThrowFatalWithCpuContext(u32 error_code, u64 process_id, FatalType policy, const CpuContext &cpu_ctx) {
            /* We don't support Error Report only fatals. */
            if (policy == FatalType_ErrorReport) {
                return ResultSuccess;
            }

            /* Note that we've thrown fatal. */
            R_TRY(this->TrySetHasThrown());

            /* At this point we have exclusive access to this->context. */
            this->context.error_code = error_code;
            this->context.cpu_ctx = cpu_ctx;

            /* Cap the stack trace to a sane limit. */
            if (cpu_ctx.architecture == CpuContext::Architecture_Aarch64) {
                this->context.cpu_ctx.aarch64_ctx.stack_trace_size = std::max(size_t(this->context.cpu_ctx.aarch64_ctx.stack_trace_size), aarch64::CpuContext::MaxStackTraceDepth);
            } else {
                this->context.cpu_ctx.aarch32_ctx.stack_trace_size = std::max(size_t(this->context.cpu_ctx.aarch32_ctx.stack_trace_size), aarch32::CpuContext::MaxStackTraceDepth);
            }

            /* Get title id. */
            pm::info::GetTitleId(&this->context.title_id, process_id);
            this->context.is_creport = (this->context.title_id == ncm::TitleId::Creport);

            if (!this->context.is_creport) {
                /* On firmware version 2.0.0, use debugging SVCs to collect information. */
                if (GetRuntimeFirmwareVersion() >= FirmwareVersion_200) {
                    fatal::srv::TryCollectDebugInformation(&this->context, process_id);
                }
            } else {
                /* We received info from creport. Parse title id from afsr0. */
                if (cpu_ctx.architecture == CpuContext::Architecture_Aarch64) {
                    this->context.title_id = cpu_ctx.aarch64_ctx.GetTitleIdForAtmosphere();
                } else {
                    this->context.title_id = cpu_ctx.aarch32_ctx.GetTitleIdForAtmosphere();
                }
            }

            /* Decide whether to generate a report. */
            this->context.generate_error_report = (policy == FatalType_ErrorReportAndErrorScreen);

            /* Adjust error code (2000-0000 -> 2162-0002). */
            if (this->context.error_code == ResultSuccess) {
                this->context.error_code = ResultErrSystemModuleAborted;
            }

            switch (policy) {
                case FatalType_ErrorReportAndErrorScreen:
                case FatalType_ErrorScreen:
                    /* Signal that we're throwing. */
                    this->event_manager.SignalEvents();

                    if (GetFatalConfig().ShouldTransitionToFatal()) {
                        RunTasks(&this->context);
                    }
                    break;
                default:
                    /* N aborts here. Should we just return an error code? */
                    std::abort();
            }

            return ResultSuccess;
        }

    }

    Result ThrowFatalForSelf(Result error_code) {
        u64 process_id = 0;
        R_ASSERT(svcGetProcessId(&process_id, CUR_PROCESS_HANDLE));
        return g_context.ThrowFatalWithPolicy(static_cast<u32>(error_code), process_id, FatalType_ErrorScreen);
    }

    Result UserService::ThrowFatal(u32 error, PidDescriptor pid_desc) {
        return g_context.ThrowFatal(error, pid_desc.pid);
    }

    Result UserService::ThrowFatalWithPolicy(u32 error, PidDescriptor pid_desc, FatalType policy) {
        return g_context.ThrowFatalWithPolicy(error, pid_desc.pid, policy);
    }

    Result UserService::ThrowFatalWithCpuContext(u32 error, PidDescriptor pid_desc, FatalType policy, InBuffer<u8> _ctx) {
        if (_ctx.num_elements < sizeof(CpuContext)) {
            return g_context.ThrowFatalWithPolicy(error, pid_desc.pid, policy);
        } else {
            return g_context.ThrowFatalWithCpuContext(error, pid_desc.pid, policy, *reinterpret_cast<const CpuContext *>(_ctx.buffer));
        }
    }

    Result PrivateService::GetFatalEvent(Out<CopiedHandle> out_h) {
        return g_context.GetEvent(out_h.GetHandlePointer());
    }

}