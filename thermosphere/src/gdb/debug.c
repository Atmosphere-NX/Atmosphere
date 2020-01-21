/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#define _GNU_SOURCE // for strchrnul
#include "gdb/debug.h"
#include "gdb/server.h"
#include "gdb/verbose.h"
#include "gdb/net.h"
#include "gdb/thread.h"
#include "gdb/mem.h"
#include "gdb/hio.h"
#include "gdb/watchpoints.h"
#include "fmt.h"

#include <stdlib.h>
#include <signal.h>
#include "pmdbgext.h"

static void GDB_DetachImmediatelyExtended(GDBContext *ctx)
{
    // detach immediately
    RecursiveLock_Lock(&ctx->lock);
    ctx->state = GDB_STATE_DETACHING;

    svcClearEvent(ctx->processAttachedEvent);
    ctx->eventToWaitFor = ctx->processAttachedEvent;

    svcClearEvent(ctx->parent->statusUpdateReceived);
    svcSignalEvent(ctx->parent->statusUpdated);
    RecursiveLock_Unlock(&ctx->lock);

    svcWaitSynchronization(ctx->parent->statusUpdateReceived, -1LL);

    RecursiveLock_Lock(&ctx->lock);
    GDB_DetachFromProcess(ctx);
    ctx->flags &= GDB_FLAG_PROC_RESTART_MASK;
    RecursiveLock_Unlock(&ctx->lock);
}

GDB_DECLARE_VERBOSE_HANDLER(Run)
{
    // Note: only titleId [mediaType [launchFlags]] is supported, and the launched title shouldn't rely on APT
    // all 3 parameters should be hex-encoded.

    // Extended remote only
    if (!(ctx->flags & GDB_FLAG_EXTENDED_REMOTE))
        return GDB_ReplyErrno(ctx, EPERM);

    u64 titleId;
    u32 mediaType = MEDIATYPE_NAND;
    u32 launchFlags = PMLAUNCHFLAG_LOAD_DEPENDENCIES;

    char args[3][32] = {{0}};
    char *pos = ctx->commandData;
    for (u32 i = 0; i < 3 && *pos != 0; i++)
    {
        char *pos2 = strchrnul(pos, ';');
        u32 dist = pos2 - pos;
        if (dist < 2)
            return GDB_ReplyErrno(ctx, EILSEQ);
        if (dist % 2 == 1)
            return GDB_ReplyErrno(ctx, EILSEQ);

        if (dist / 2 > 16) // buffer overflow check
            return GDB_ReplyErrno(ctx, EINVAL);

        u32 n = GDB_DecodeHex(args[i], pos, dist / 2);
        if (n == 0)
            return GDB_ReplyErrno(ctx, EILSEQ);
        pos = *pos2 == 0 ? pos2 : pos2 + 1;
    }

    if (args[0][0] == 0)
        return GDB_ReplyErrno(ctx, EINVAL); // first arg mandatory

    if (GDB_ParseIntegerList64(&titleId, args[0], 1, 0, 0, 16, false) == NULL)
        return GDB_ReplyErrno(ctx, EINVAL);

    if (args[1][0] != 0 && (GDB_ParseIntegerList(&mediaType, args[1], 1, 0, 0, 16, true) == NULL || mediaType >= 0x100))
        return GDB_ReplyErrno(ctx, EINVAL);

    if (args[2][0] != 0 && GDB_ParseIntegerList(&launchFlags, args[2], 1, 0, 0, 16, true) == NULL)
        return GDB_ReplyErrno(ctx, EINVAL);

    FS_ProgramInfo progInfo;
    progInfo.mediaType = (FS_MediaType)mediaType;
    progInfo.programId = titleId;

    RecursiveLock_Lock(&ctx->lock);
    Result r = GDB_CreateProcess(ctx, &progInfo, launchFlags);

    if (R_FAILED(r))
    {
        if(ctx->debug != 0)
            GDB_DetachImmediatelyExtended(ctx);
        RecursiveLock_Unlock(&ctx->lock);
        return GDB_ReplyErrno(ctx, EPERM);
    }

    RecursiveLock_Unlock(&ctx->lock);
    return R_SUCCEEDED(r) ? GDB_SendStopReply(ctx, &ctx->latestDebugEvent) : GDB_ReplyErrno(ctx, EPERM);
}

GDB_DECLARE_HANDLER(Restart)
{
    // Note: removed from gdb
    // Extended remote only & process must have been created
    if (!(ctx->flags & GDB_FLAG_EXTENDED_REMOTE) || !(ctx->flags & GDB_FLAG_CREATED))
        return GDB_ReplyErrno(ctx, EPERM);

    FS_ProgramInfo progInfo = ctx->launchedProgramInfo;
    u32 launchFlags = ctx->launchedProgramLaunchFlags;

    ctx->flags |= GDB_FLAG_TERMINATE_PROCESS;
    if (ctx->flags & GDB_FLAG_EXTENDED_REMOTE)
        GDB_DetachImmediatelyExtended(ctx);
    
    RecursiveLock_Lock(&ctx->lock);
    Result r = GDB_CreateProcess(ctx, &progInfo, launchFlags);
    if (R_FAILED(r) && ctx->debug != 0)
        GDB_DetachImmediatelyExtended(ctx);
    RecursiveLock_Unlock(&ctx->lock);
    return 0;
}

GDB_DECLARE_VERBOSE_HANDLER(Attach)
{
    // Extended remote only
    if (!(ctx->flags & GDB_FLAG_EXTENDED_REMOTE))
        return GDB_ReplyErrno(ctx, EPERM);

    u32 pid;
    if(GDB_ParseHexIntegerList(&pid, ctx->commandData, 1, 0) == NULL)
        return GDB_ReplyErrno(ctx, EILSEQ);

    RecursiveLock_Lock(&ctx->lock);
    ctx->pid = pid;
    Result r = GDB_AttachToProcess(ctx);
    if(R_FAILED(r))
        GDB_DetachImmediatelyExtended(ctx);
    RecursiveLock_Unlock(&ctx->lock);
    return R_SUCCEEDED(r) ? GDB_SendStopReply(ctx, &ctx->latestDebugEvent) : GDB_ReplyErrno(ctx, EPERM);
}

/*
    Since we can't select particular threads to continue (and that's uncompliant behavior):
        - if we continue the current thread, continue all threads
        - otherwise, leaves all threads stopped but make the client believe it's continuing
*/

GDB_DECLARE_HANDLER(Detach)
{
    ctx->state = GDB_STATE_DETACHING;
    if (ctx->flags & GDB_FLAG_EXTENDED_REMOTE)
        GDB_DetachImmediatelyExtended(ctx);
    return GDB_ReplyOk(ctx);
}

GDB_DECLARE_HANDLER(Kill)
{
    ctx->state = GDB_STATE_DETACHING;
    ctx->flags |= GDB_FLAG_TERMINATE_PROCESS;
    if (ctx->flags & GDB_FLAG_EXTENDED_REMOTE)
        GDB_DetachImmediatelyExtended(ctx);

    return 0;
}

GDB_DECLARE_HANDLER(Break)
{
    if(!(ctx->flags & GDB_FLAG_PROCESS_CONTINUING))
        return GDB_SendPacket(ctx, "S02", 3);
    else
    {
        ctx->flags &= ~GDB_FLAG_PROCESS_CONTINUING;
        return 0;
    }
}

void GDB_ContinueExecution(GDBContext *ctx)
{
    ctx->selectedThreadId = ctx->selectedThreadIdForContinuing = 0;
    svcContinueDebugEvent(ctx->debug, ctx->continueFlags);
    ctx->flags |= GDB_FLAG_PROCESS_CONTINUING;
}

GDB_DECLARE_HANDLER(Continue)
{
    char *addrStart = NULL;
    u32 addr = 0;

    if(ctx->selectedThreadIdForContinuing != 0 && ctx->selectedThreadIdForContinuing != ctx->currentThreadId)
        return 0;

    if(ctx->commandData[-1] == 'C')
    {
        if(ctx->commandData[0] == 0 || ctx->commandData[1] == 0 || (ctx->commandData[2] != 0 && ctx->commandData[2] == ';'))
            return GDB_ReplyErrno(ctx, EILSEQ);

        // Signal ignored...

        if(ctx->commandData[2] == ';')
            addrStart = ctx->commandData + 3;
    }
    else
    {
        if(ctx->commandData[0] != 0)
            addrStart = ctx->commandData;
    }

    if(addrStart != NULL && ctx->currentThreadId != 0)
    {
        ThreadContext regs;
        if(GDB_ParseHexIntegerList(&addr, ctx->commandData + 3, 1, 0) == NULL)
            return GDB_ReplyErrno(ctx, EILSEQ);

        Result r = svcGetDebugThreadContext(&regs, ctx->debug, ctx->currentThreadId, THREADCONTEXT_CONTROL_CPU_SPRS);
        if(R_SUCCEEDED(r))
        {
            regs.cpu_registers.pc = addr;
            r = svcSetDebugThreadContext(ctx->debug, ctx->currentThreadId, &regs, THREADCONTEXT_CONTROL_CPU_SPRS);
        }
    }

    GDB_ContinueExecution(ctx);
    return 0;
}

GDB_DECLARE_VERBOSE_HANDLER(Continue)
{
    char *pos = ctx->commandData;
    bool currentThreadFound = false;
    while(pos != NULL && *pos != 0 && !currentThreadFound)
    {
        if(*pos != 'c' && *pos != 'C')
            return GDB_ReplyErrno(ctx, EPERM);

        pos += *pos == 'C' ? 3 : 1;

        if(*pos++ != ':') // default action found
        {
            currentThreadFound = true;
            break;
        }

        char *nextpos = (char *)strchr(pos, ';');
        if(strncmp(pos, "-1", 2) == 0)
            currentThreadFound = true;
        else
        {
            u32 threadId;
            if(GDB_ParseHexIntegerList(&threadId, pos, 1, ';') == NULL)
                return GDB_ReplyErrno(ctx, EILSEQ);
            currentThreadFound = currentThreadFound || threadId == ctx->currentThreadId;
        }

        pos = nextpos;
    }

    if(ctx->currentThreadId == 0 || currentThreadFound)
        GDB_ContinueExecution(ctx);

    return 0;
}

GDB_DECLARE_HANDLER(GetStopReason)
{
    if (ctx->processEnded && ctx->processExited) {
        return GDB_SendPacket(ctx, "W00", 3);
    } else if (ctx->processEnded && !ctx->processExited) {
        return GDB_SendPacket(ctx, "X0f", 3);
    } else if (ctx->debug == 0) {
        return GDB_SendPacket(ctx, "W00", 3);
    } else {
        return GDB_SendStopReply(ctx, &ctx->latestDebugEvent);
    }
}

static int GDB_ParseCommonThreadInfo(char *out, GDBContext *ctx, int sig)
{
    u32 threadId = ctx->currentThreadId;
    ThreadContext regs;
    s64 dummy;
    u32 core;
    Result r = svcGetDebugThreadContext(&regs, ctx->debug, threadId, THREADCONTEXT_CONTROL_ALL);
    int n = sprintf(out, "T%02xthread:%lx;", sig, threadId);

    if(R_FAILED(r))
        return n;

    r = svcGetDebugThreadParam(&dummy, &core, ctx->debug, ctx->currentThreadId, DBGTHREAD_PARAMETER_CPU_CREATOR); // Creator = "first ran, and running the thread"

    if(R_SUCCEEDED(r))
        n += sprintf(out + n, "core:%lx;", core);

    for(u32 i = 0; i <= 12; i++)
        n += sprintf(out + n, "%lx:%08lx;", i, __builtin_bswap32(regs.cpu_registers.r[i]));

    n += sprintf(out + n, "d:%08lx;e:%08lx;f:%08lx;19:%08lx;",
        __builtin_bswap32(regs.cpu_registers.sp), __builtin_bswap32(regs.cpu_registers.lr), __builtin_bswap32(regs.cpu_registers.pc),
        __builtin_bswap32(regs.cpu_registers.cpsr));

    for(u32 i = 0; i < 16; i++)
    {
        u64 val;
        memcpy(&val, &regs.fpu_registers.d[i], 8);
        n += sprintf(out + n, "%lx:%016llx;", 26 + i, __builtin_bswap64(val));
    }

    n += sprintf(out + n, "2a:%08lx;2b:%08lx;", __builtin_bswap32(regs.fpu_registers.fpscr), __builtin_bswap32(regs.fpu_registers.fpexc));

    return n;
}

void GDB_PreprocessDebugEvent(GDBContext *ctx, DebugEventInfo *info)
{
    switch(info->type)
    {
        case DBGEVENT_ATTACH_PROCESS:
        {
            ctx->pid = info->attach_process.process_id;
            break;
        }

        case DBGEVENT_ATTACH_THREAD:
        {
            if(ctx->nbThreads == MAX_DEBUG_THREAD)
                svcBreak(USERBREAK_ASSERT);
            else
            {
                ++ctx->totalNbCreatedThreads;
                ctx->threadInfos[ctx->nbThreads].id = info->thread_id;
                ctx->threadInfos[ctx->nbThreads++].tls = info->attach_thread.thread_local_storage;
            }

            break;
        }

        case DBGEVENT_EXIT_THREAD:
        {
            u32 i;
            for(i = 0; i < ctx->nbThreads && ctx->threadInfos[i].id != info->thread_id; i++);
            if(i == ctx->nbThreads ||  ctx->threadInfos[i].id != info->thread_id)
                svcBreak(USERBREAK_ASSERT);
            else
            {
                for(u32 j = i; j < ctx->nbThreads - 1; j++)
                    memcpy(ctx->threadInfos + j, ctx->threadInfos + j + 1, sizeof(ThreadInfo));
                memset(ctx->threadInfos + --ctx->nbThreads, 0, sizeof(ThreadInfo));
            }

            break;
        }

        case DBGEVENT_EXIT_PROCESS:
        {
            ctx->processEnded = true;
            ctx->processExited = info->exit_process.reason == EXITPROCESS_EVENT_EXIT;

            break;
        }

        case DBGEVENT_OUTPUT_STRING:
        {
            if(info->output_string.string_addr >= 0xFFFFFFFE)
            {
                u32 sz = info->output_string.string_size, addr = info->output_string.string_addr, threadId = info->thread_id;
                memset(info, 0, sizeof(DebugEventInfo));
                info->type = (addr == 0xFFFFFFFF) ? DBGEVENT_SYSCALL_OUT : DBGEVENT_SYSCALL_IN;
                info->thread_id = threadId;
                info->flags = 1;
                info->syscall.syscall = sz;
            }
            else if (info->output_string.string_size == 0)
                GDB_FetchPackedHioRequest(ctx, info->output_string.string_addr);

            break;
        }

        case DBGEVENT_EXCEPTION:
        {
            switch(info->exception.type)
            {
                case EXCEVENT_UNDEFINED_INSTRUCTION:
                {
                    // kernel bugfix for thumb mode
                    ThreadContext regs;
                    Result r = svcGetDebugThreadContext(&regs, ctx->debug, info->thread_id, THREADCONTEXT_CONTROL_CPU_SPRS);
                    if(R_SUCCEEDED(r) && (regs.cpu_registers.cpsr & 0x20) != 0)
                    {
                        regs.cpu_registers.pc += 2;
                        r = svcSetDebugThreadContext(ctx->debug, info->thread_id, &regs, THREADCONTEXT_CONTROL_CPU_SPRS);
                    }

                    break;
                }

                default:
                    break;
            }
        }
        default:
            break;
    }
}

int GDB_SendStopReply(GDBContext *ctx, const DebugEventInfo *info)
{
    char buffer[GDB_BUF_LEN + 1];

    switch(info->type)
    {
        case DBGEVENT_ATTACH_PROCESS:
            break; // Dismissed

        case DBGEVENT_ATTACH_THREAD:
        {
            if((ctx->flags & GDB_FLAG_ATTACHED_AT_START) && ctx->totalNbCreatedThreads == 1)
            {
                // Main thread created
                ctx->currentThreadId = info->thread_id;
                GDB_ParseCommonThreadInfo(buffer, ctx, SIGINT);
                return GDB_SendFormattedPacket(ctx, "%s", buffer);
            }
            else if(info->attach_thread.creator_thread_id == 0 || !ctx->catchThreadEvents)
                break; // Dismissed
            else
            {
                ctx->currentThreadId = info->thread_id;
                return GDB_SendPacket(ctx, "T05create:;", 10);
            }
        }

        case DBGEVENT_EXIT_THREAD:
        {
            if(ctx->catchThreadEvents && info->exit_thread.reason < EXITTHREAD_EVENT_EXIT_PROCESS)
            {
                // no signal, SIGTERM, SIGQUIT (process exited), SIGTERM (process terminated)
                static int threadExitRepliesSigs[] = { 0, SIGTERM, SIGQUIT, SIGTERM };
                return GDB_SendFormattedPacket(ctx, "w%02x;%lx", threadExitRepliesSigs[(u32)info->exit_thread.reason], info->thread_id);
            }
            break;
        }

        case DBGEVENT_EXIT_PROCESS:
        {
            // exited (no error / unhandled exception), SIGTERM (process terminated) * 2
            static const char *processExitReplies[] = { "W00", "X0f", "X0f" };
            return GDB_SendPacket(ctx, processExitReplies[(u32)info->exit_process.reason], 3);
        }

        case DBGEVENT_EXCEPTION:
        {
            ExceptionEvent exc = info->exception;

            switch(exc.type)
            {
                case EXCEVENT_UNDEFINED_INSTRUCTION:
                case EXCEVENT_PREFETCH_ABORT: // doesn't include hardware breakpoints
                case EXCEVENT_DATA_ABORT:     // doesn't include hardware watchpoints
                case EXCEVENT_UNALIGNED_DATA_ACCESS:
                case EXCEVENT_UNDEFINED_SYSCALL:
                {
                    u32 signum = exc.type == EXCEVENT_UNDEFINED_INSTRUCTION ? SIGILL :
                                (exc.type == EXCEVENT_UNDEFINED_SYSCALL ? SIGSYS : SIGSEGV);

                    ctx->currentThreadId = info->thread_id;
                    GDB_ParseCommonThreadInfo(buffer, ctx, signum);
                    return GDB_SendFormattedPacket(ctx, "%s", buffer);
                }

                case EXCEVENT_ATTACH_BREAK:
                    return GDB_SendPacket(ctx, "S00", 3);

                case EXCEVENT_STOP_POINT:
                {
                    ctx->currentThreadId = info->thread_id;

                    switch(exc.stop_point.type)
                    {
                        case STOPPOINT_SVC_FF:
                        {
                            GDB_ParseCommonThreadInfo(buffer, ctx, SIGTRAP);
                            return GDB_SendFormattedPacket(ctx, "%sswbreak:;", buffer);
                            break;
                        }

                        case STOPPOINT_BREAKPOINT:
                        {
                            // /!\ Not actually implemented (and will never be)
                            GDB_ParseCommonThreadInfo(buffer, ctx, SIGTRAP);
                            return GDB_SendFormattedPacket(ctx, "%shwbreak:;", buffer);
                            break;
                        }

                        case STOPPOINT_WATCHPOINT:
                        {
                            const char *kinds[] = { "", "r", "", "a" };
                            WatchpointKind kind = GDB_GetWatchpointKind(ctx, exc.stop_point.fault_information);
                            if(kind == WATCHPOINT_DISABLED)
                                GDB_SendDebugString(ctx, "Warning: unknown watchpoint encountered!\n");

                            GDB_ParseCommonThreadInfo(buffer, ctx, SIGTRAP);
                            return GDB_SendFormattedPacket(ctx, "%s%swatch:%08lx;", buffer, kinds[(u32)kind], exc.stop_point.fault_information);
                            break;
                        }

                        default:
                            break;
                    }
                    break;
                }

                case EXCEVENT_USER_BREAK:
                {
                    ctx->currentThreadId = info->thread_id;
                    GDB_ParseCommonThreadInfo(buffer, ctx, SIGINT);
                    return GDB_SendFormattedPacket(ctx, "%s", buffer);
                    //TODO
                }

                case EXCEVENT_DEBUGGER_BREAK:
                {
                    u32 threadIds[4];
                    u32 nbThreads = 0;

                    for(u32 i = 0; i < 4; i++)
                    {
                        if(exc.debugger_break.thread_ids[i] > 0)
                            threadIds[nbThreads++] = (u32)exc.debugger_break.thread_ids[i];
                    }

                    u32 currentThreadId = nbThreads > 0 ? GDB_GetCurrentThreadFromList(ctx, threadIds, nbThreads) : GDB_GetCurrentThread(ctx);
                    s64 dummy;
                    u32 mask = 0;

                    svcGetDebugThreadParam(&dummy, &mask, ctx->debug, currentThreadId, DBGTHREAD_PARAMETER_SCHEDULING_MASK_LOW);

                    if(mask == 1)
                    {
                        ctx->currentThreadId = currentThreadId;
                        GDB_ParseCommonThreadInfo(buffer, ctx, SIGINT);
                        return GDB_SendFormattedPacket(ctx, "%s", buffer);
                    }
                    else
                        return GDB_SendPacket(ctx, "S02", 3);
                }

                default:
                    break;
            }

            break;
        }

        case DBGEVENT_SYSCALL_IN:
        {
            ctx->currentThreadId = info->thread_id;
            GDB_ParseCommonThreadInfo(buffer, ctx, SIGTRAP);
            return GDB_SendFormattedPacket(ctx, "%ssyscall_entry:%02x;", buffer, info->syscall.syscall);
        }

        case DBGEVENT_SYSCALL_OUT:
        {
            ctx->currentThreadId = info->thread_id;
            GDB_ParseCommonThreadInfo(buffer, ctx, SIGTRAP);
            return GDB_SendFormattedPacket(ctx, "%ssyscall_return:%02x;", buffer, info->syscall.syscall);
        }

        case DBGEVENT_OUTPUT_STRING:
        {
            // Regular "output string"
            if (!GDB_IsHioInProgress(ctx))
            {
                u32 addr = info->output_string.string_addr;
                u32 remaining = info->output_string.string_size;
                u32 sent = 0;
                int total = 0;
                while(remaining > 0)
                {
                    u32 pending = (GDB_BUF_LEN - 1) / 2;
                    pending = pending < remaining ? pending : remaining;

                    int res = GDB_SendMemory(ctx, "O", 1, addr + sent, pending);
                    if(res < 0 || (u32) res != 5 + 2 * pending)
                        break;

                    sent += pending;
                    remaining -= pending;
                    total += res;
                }

                return total;
            }
            else // HIO
            {
                return GDB_SendCurrentHioRequest(ctx);
            }

        }
        default:
            break;
    }

    return 0;
}

/*
    Only 1 blocking event can be enqueued at a time: they preempt all the other threads.
    The only "non-blocking" event that is implemented is EXIT PROCESS (but it's a very special case)
*/
int GDB_HandleDebugEvents(GDBContext *ctx)
{
    if(ctx->state == GDB_STATE_DETACHING)
        return -1;

    DebugEventInfo info;
    Result rdbg = svcGetProcessDebugEvent(&info, ctx->debug);

    if(R_FAILED(rdbg))
        return -1;

    GDB_PreprocessDebugEvent(ctx, &info);

    int ret = 0;
    bool continueAutomatically = (info.type == DBGEVENT_OUTPUT_STRING  && !GDB_IsHioInProgress(ctx)) ||
                                info.type == DBGEVENT_ATTACH_PROCESS ||
                                (info.type == DBGEVENT_ATTACH_THREAD && (info.attach_thread.creator_thread_id == 0 || !ctx->catchThreadEvents)) ||
                                (info.type == DBGEVENT_EXIT_THREAD && (info.exit_thread.reason >= EXITTHREAD_EVENT_EXIT_PROCESS || !ctx->catchThreadEvents)) ||
                                info.type == DBGEVENT_EXIT_PROCESS || !(info.flags & 1);

    if(continueAutomatically)
    {
        Result r = 0;
        ret = GDB_SendStopReply(ctx, &info);
        if(info.flags & 1)
            r = svcContinueDebugEvent(ctx->debug, ctx->continueFlags);

        if(r == (Result)0xD8A02008) // process ended
            return -2;

        return -ret - 3;
    }
    else
    {
        int ret;

        if(ctx->processEnded)
            return -2;

        ctx->latestDebugEvent = info;
        ret = GDB_SendStopReply(ctx, &info);
        ctx->flags &= ~GDB_FLAG_PROCESS_CONTINUING;
        return ret;
    }
}
