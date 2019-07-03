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

#include <switch.h>
#include "fatal_throw.hpp"
#include "fatal_event_manager.hpp"
#include "fatal_task.hpp"
#include "fatal_config.hpp"
#include "fatal_debug.hpp"

static bool g_thrown = false;

static Result SetThrown() {
    /* This should be fine, since fatal only has a single IPC thread. */
    if (g_thrown) {
        return ResultFatalAlreadyThrown;
    }

    g_thrown = true;
    return ResultSuccess;
}

Result ThrowFatalForSelf(u32 error) {
    u64 pid = 0;

    svcGetProcessId(&pid, CUR_PROCESS_HANDLE);
    return ThrowFatalImpl(error, pid, FatalType_ErrorScreen, nullptr);
}

Result ThrowFatalImpl(u32 error, u64 pid, FatalType policy, FatalCpuContext *cpu_ctx) {
    FatalThrowContext ctx = {0};
    ctx.error_code = error;
    if (cpu_ctx != nullptr) {
        ctx.cpu_ctx = *cpu_ctx;
        /* Assume if we're provided a context that it's complete. */
        for (u32 i = 0; i < NumAarch64Gprs; i++) {
            ctx.has_gprs[i] = true;
        }
        /* Cap the stack trace size at a sane limit. */
        /* TODO: Better to set to zero, in order to manually collect debug info ourselves instead? */
        if (cpu_ctx->is_aarch32) {
            ctx.cpu_ctx.aarch32_ctx.stack_trace_size = std::max(ctx.cpu_ctx.aarch32_ctx.stack_trace_size, static_cast<u32>(Aarch32CpuContext::MaxStackTraceDepth));
        } else {
            ctx.cpu_ctx.aarch64_ctx.stack_trace_size = std::max(ctx.cpu_ctx.aarch64_ctx.stack_trace_size, static_cast<u32>(Aarch64CpuContext::MaxStackTraceDepth));
        }
    } else {
        std::memset(&ctx.cpu_ctx, 0, sizeof(ctx.cpu_ctx));
    }
    /* Reassign this unconditionally, for convenience. */
    cpu_ctx = &ctx.cpu_ctx;

    /* Get config. */
    const FatalConfig *config = GetFatalConfig();

    /* Get title id. On failure, it'll be zero. */
    sts::ncm::TitleId title_id = sts::ncm::TitleId::Invalid;
    sts::pm::info::GetTitleId(&title_id, pid);
    ctx.is_creport = title_id == sts::ncm::TitleId::Creport;

    /* Support for ams creport. TODO: Make this its own command? */
    if (ctx.is_creport && !cpu_ctx->is_aarch32 && cpu_ctx->aarch64_ctx.afsr0 != 0) {
        title_id = sts::ncm::TitleId{cpu_ctx->aarch64_ctx.afsr0};
    }

    /* Atmosphere extension: automatic debug info collection. */
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_200 && !ctx.is_creport) {
        if ((cpu_ctx->is_aarch32 && cpu_ctx->aarch32_ctx.stack_trace_size == 0) || (!cpu_ctx->is_aarch32 && cpu_ctx->aarch64_ctx.stack_trace_size == 0)) {
            TryCollectDebugInformation(&ctx, pid);
        }
    }

    switch (policy) {
        case FatalType_ErrorReport:
            /* TODO: Don't write an error report. */
            break;
        case FatalType_ErrorReportAndErrorScreen:
        case FatalType_ErrorScreen:
            {
                /* Ensure we only throw once. */
                R_TRY(SetThrown());

                /* Signal that fatal is about to happen. */
                GetEventManager()->SignalEvents();

                /* Create events. */
                Event erpt_event;
                Event battery_event;
                if (R_FAILED(eventCreate(&erpt_event, false)) || R_FAILED(eventCreate(&battery_event, false))) {
                    std::abort();
                }

                /* Run tasks. */
                if (config->transition_to_fatal) {
                    RunFatalTasks(&ctx, static_cast<u64>(title_id), policy == FatalType_ErrorReportAndErrorScreen, &erpt_event, &battery_event);
                } else {
                    /* If flag is not set, don't show the fatal screen. */
                    return ResultSuccess;
                }

            }
            break;
        default:
            /* N aborts here. Should we just return an error code? */
            std::abort();
    }

    return ResultSuccess;
}
