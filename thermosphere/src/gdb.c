/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#if 0
#include "gdb.h"
#include "gdb/net.h"
#include "gdb/server.h"

#include "gdb/debug.h"

#include "gdb/watchpoints.h"
#include "gdb/breakpoints.h"
#include "gdb/stop_point.h"

void GDB_InitializeContext(GDBContext *ctx)
{
    memset(ctx, 0, sizeof(GDBContext));
    RecursiveLock_Init(&ctx->lock);

    RecursiveLock_Lock(&ctx->lock);

    svcCreateEvent(&ctx->continuedEvent, RESET_ONESHOT);
    svcCreateEvent(&ctx->processAttachedEvent, RESET_STICKY);

    ctx->eventToWaitFor = ctx->processAttachedEvent;
    ctx->continueFlags = (DebugFlags)(DBG_SIGNAL_FAULT_EXCEPTION_EVENTS | DBG_INHIBIT_USER_CPU_EXCEPTION_HANDLERS);

    RecursiveLock_Unlock(&ctx->lock);
}

void GDB_FinalizeContext(GDBContext *ctx)
{
    RecursiveLock_Lock(&ctx->lock);

    svcClearEvent(ctx->processAttachedEvent);

    svcCloseHandle(ctx->processAttachedEvent);
    svcCloseHandle(ctx->continuedEvent);

    RecursiveLock_Unlock(&ctx->lock);
}

Result GDB_AttachToProcess(GDBContext *ctx)
{
    Result r;

    // Two cases: attached during execution, or started attached
    // The second case will have, after RunQueuedProcess: attach process, debugger break, attach thread (with creator = 0)

    if (!(ctx->flags & GDB_FLAG_ATTACHED_AT_START))
        r = svcDebugActiveProcess(&ctx->debug, ctx->pid);
    else
    {
        r = 0;
    }
    if(R_SUCCEEDED(r))
    {
        // Note: ctx->pid will be (re)set while processing 'attach process'
        DebugEventInfo *info = &ctx->latestDebugEvent;
        ctx->processExited = ctx->processEnded = false;
        if (!(ctx->flags & GDB_FLAG_ATTACHED_AT_START))
        {
            while(R_SUCCEEDED(svcGetProcessDebugEvent(info, ctx->debug)) &&
                info->type != DBGEVENT_EXCEPTION &&
                info->exception.type != EXCEVENT_ATTACH_BREAK)
            {
                GDB_PreprocessDebugEvent(ctx, info);
                svcContinueDebugEvent(ctx->debug, ctx->continueFlags);
            }
        }
        else
        {
            // Attach process, debugger break
            for(u32 i = 0; i < 2; i++)
            {
                if (R_FAILED(r = svcGetProcessDebugEvent(info, ctx->debug)))
                    return r;
                GDB_PreprocessDebugEvent(ctx, info);
                if (R_FAILED(r = svcContinueDebugEvent(ctx->debug, ctx->continueFlags)))
                    return r;
            }

            if(R_FAILED(r = svcWaitSynchronization(ctx->debug, -1LL)))
                return r;
            if (R_FAILED(r = svcGetProcessDebugEvent(info, ctx->debug)))
                return r;
            // Attach thread
            GDB_PreprocessDebugEvent(ctx, info);
        }
    }
    else
        return r;

    r = svcSignalEvent(ctx->processAttachedEvent);
    if (R_SUCCEEDED(r))
        ctx->state = GDB_STATE_ATTACHED;
    return r;
}

void GDB_DetachFromProcess(GDBContext *ctx)
{
    DebugEventInfo dummy;
    for(u32 i = 0; i < ctx->nbBreakpoints; i++)
    {
        if(!ctx->breakpoints[i].persistent)
            GDB_DisableBreakpointById(ctx, i);
    }
    memset(&ctx->breakpoints, 0, sizeof(ctx->breakpoints));
    ctx->nbBreakpoints = 0;

    for(u32 i = 0; i < ctx->nbWatchpoints; i++)
    {
        GDB_RemoveWatchpoint(ctx, ctx->watchpoints[i], WATCHPOINT_DISABLED);
        ctx->watchpoints[i] = 0;
    }
    ctx->nbWatchpoints = 0;

    svcKernelSetState(0x10002, ctx->pid, false);
    memset(ctx->svcMask, 0, 32);

    memset(ctx->threadListData, 0, sizeof(ctx->threadListData));
    ctx->threadListDataPos = 0;

    //svcSignalEvent(server->statusUpdated);

    /*
        There's a possibility of a race condition with a possible user exception handler, but you shouldn't
        use 'kill' on APPLICATION titles in the first place (reboot hanging because the debugger is still running, etc).
    */

    ctx->continueFlags = (DebugFlags)0;

    while(R_SUCCEEDED(svcGetProcessDebugEvent(&dummy, ctx->debug)));
    while(R_SUCCEEDED(svcContinueDebugEvent(ctx->debug, ctx->continueFlags)));
    if(ctx->flags & GDB_FLAG_TERMINATE_PROCESS)
    {
        svcTerminateDebugProcess(ctx->debug);
        ctx->processEnded = true;
        ctx->processExited = false;
    }

    while(R_SUCCEEDED(svcGetProcessDebugEvent(&dummy, ctx->debug)));
    while(R_SUCCEEDED(svcContinueDebugEvent(ctx->debug, ctx->continueFlags)));

    svcCloseHandle(ctx->debug);
    ctx->debug = 0;
    memset(&ctx->launchedProgramInfo, 0, sizeof(FS_ProgramInfo));
    ctx->launchedProgramLaunchFlags = 0;

    ctx->eventToWaitFor = ctx->processAttachedEvent;
    ctx->continueFlags = (DebugFlags)(DBG_SIGNAL_FAULT_EXCEPTION_EVENTS | DBG_INHIBIT_USER_CPU_EXCEPTION_HANDLERS);
    ctx->pid = 0;
    ctx->currentThreadId = 0;
    ctx->selectedThreadId = 0;
    ctx->selectedThreadIdForContinuing = 0;
    ctx->nbThreads = 0;
    ctx->totalNbCreatedThreads = 0;
    memset(ctx->threadInfos, 0, sizeof(ctx->threadInfos));

    ctx->currentHioRequestTargetAddr = 0;
    memset(&ctx->currentHioRequest, 0, sizeof(PackedGdbHioRequest));
}

Result GDB_CreateProcess(GDBContext *ctx, const FS_ProgramInfo *progInfo, u32 launchFlags)
{
    Handle debug = 0;
    ctx->debug = 0;
    Result r = PMDBG_LaunchTitleDebug(&debug, progInfo, launchFlags);
    if(R_FAILED(r))
        return r;
    
    ctx->flags |= GDB_FLAG_CREATED | GDB_FLAG_ATTACHED_AT_START;
    ctx->debug = debug;
    ctx->launchedProgramInfo = *progInfo;
    ctx->launchedProgramLaunchFlags = launchFlags;
    r = GDB_AttachToProcess(ctx);
    return r;
}

GDB_DECLARE_HANDLER(Unsupported)
{
    return GDB_ReplyEmpty(ctx);
}

GDB_DECLARE_HANDLER(EnableExtendedMode)
{
    if (ctx->localPort >= GDB_PORT_BASE && ctx->localPort < GDB_PORT_BASE + MAX_DEBUG)
    {
        ctx->flags |= GDB_FLAG_EXTENDED_REMOTE;
        return GDB_ReplyOk(ctx);
    }
    else
        return GDB_ReplyEmpty(ctx);
}
#endif
