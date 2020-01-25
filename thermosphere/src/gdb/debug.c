/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#define _GNU_SOURCE // for strchrnul
#include <stdio.h>
#include <string.h>
#include "../exceptions.h"
#include "../watchpoints.h"

#include "debug.h"
#include "net.h"

#include "server.h"
#include "verbose.h"
#include "thread.h"
#include "mem.h"
#include "hio.h"
#include "watchpoints.h"

#include <stdlib.h>
#include <signal.h>

/*
    Since we can't select particular threads to continue (and that's uncompliant behavior):
        - if we continue the current thread, continue all threads
        - otherwise, leaves all threads stopped but make the client believe it's continuing
*/

GDB_DECLARE_HANDLER(Detach)
{
    ctx->state = GDB_STATE_DETACHING;
    return GDB_ReplyOk(ctx);
}

GDB_DECLARE_HANDLER(Kill)
{
    ctx->state = GDB_STATE_DETACHING;
    ctx->flags |= GDB_FLAG_TERMINATE;

    return 0;
}

GDB_DECLARE_HANDLER(Break)
{
    // TODO
    if(!(ctx->flags & GDB_FLAG_CONTINUING)) // Is this ever reached?
        return GDB_SendPacket(ctx, "S02", 3);
    else
    {
        ctx->flags &= ~GDB_FLAG_CONTINUING;
        return 0;
    }
}

void GDB_ContinueExecution(GDBContext *ctx)
{
    ctx->selectedThreadId = ctx->selectedThreadIdForContinuing = 0;
    svcContinueDebugEvent(ctx->debug, ctx->continueFlags);
    ctx->flags |= GDB_FLAG_CONTINUING;
}

GDB_DECLARE_HANDLER(Continue)
{
    // TODO
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
    // TODO
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
    } else if (!GDB_IsAttached(ctx)) {
        return GDB_SendPacket(ctx, "W00", 3);
    } else {
        // TODO //return GDB_SendStopReply(ctx, &ctx->latestDebugEvent);
    }
}

static int GDB_ParseExceptionFrame(char *out, const DebugEventInfo *info, int sig)
{
    u32 coreId = info->coreId;
    ExceptionStackFrame *frame = info->frame;

    int n = sprintf(out, "T%02xthread:%lx;core:%lx;", sig, 1 + coreId, coreId);

    // Dump the GPRs & sp & pc & cpsr (cpsr is 32-bit in the xml desc)
    // For performance reasons, we don't include the FPU registers here
    for (u32 i = 0; i < 31; i++) {
        n += sprintf(out + n, "%x:%016lx;", i, __builtin_bswap64(readFrameRegister(frame, i)));
    }

    n += sprintf(
        out + n,
        "1f:%016lx;20:%016lx;21:%08x",
        __builtin_bswap64(*exceptionGetSpPtr(frame)),
        __builitin_bswap32((u32)frame->spsr_el2)
    );

    return n;
}

int GDB_SendStopReply(GDBContext *ctx, const DebugEventInfo *info)
{
    char buffer[GDB_BUF_LEN + 1];
    int n;

    buffer[0] = 0;

    switch(info->type) {
        case DBGEVENT_DEBUGGER_BREAK: {
            strcpy(buffer, "S02");
        }
    
        case DBGEVENT_CORE_ON: {
            if (ctx->catchThreadEvents) {
                ctx->currentThreadId = info->coreId + 1; // FIXME?
                strcpy(buffer, "T05create:;");
            }
            break;
        }

        case DBGEVENT_CORE_OFF: {
            if(ctx->catchThreadEvents) {
                sprintf(buffer, "w0;%x", info->coreId + 1);
            }
            break;
        }

        case DBGEVENT_EXIT: {
            // exited (no error / unhandled exception), SIGTERM (process terminated) * 2
            static const char *processExitReplies[] = { "W00", "X0f" };
            strcpy(buffer, processExitReplies[ctx->processExited ? 0 : 1]);
            break;
        }

        case DBGEVENT_EXCEPTION: {
            ExceptionClass ec = info->frame->esr_el2.ec;
            ctx->currentThreadId = info->coreId + 1; // FIXME?

            // Aside from stage 2 translation faults and other pre-handled exceptions, 
            // the only notable exceptions we get are stop point/single step events from the debugee (basically classes 0x3x)
            switch(ec) {
                case Exception_BreakpointLowerEl: {
                    n = GDB_ParseExceptionFrame(buffer, ctx, SIGTRAP);
                    strcat(buffer + n, "hwbreak:;");
                }

                case Exception_WatchpointLowerEl: {
                    static const char *kinds[] = { "", "r", "", "a" };
                    // Note: exception info doesn't provide us with the access size. Use 1.
                    bool wnr = (info->frame->esr_el2.iss & BIT(6)) != 0;
                    WatchpointLoadStoreControl dr = wnr ? WatchpointLoadStoreControl_Store : WatchpointLoadStoreControl_Load;
                    DebugControlRegister cr = retrieveSplitWatchpointConfig(info->frame->far_el2, 1, dr, false);
                    if (!cr.enabled) {
                        DEBUG("GDB: oops, unhandled watchpoint for core id %u, far=%016lx\n", info->coreId, info->frame->far_el2);
                    } else {
                        n = GDB_ParseExceptionFrame(buffer, ctx, SIGTRAP);
                        sprintf(buffer + n, "%swatch:%016lx;", kinds[cr.lsc], info->frame->far_el2);
                    }
                }

                // Note: we don't really support 32-bit sw breakpoints, we'll still report them
                // if the guest has inserted some of them manually...
                case Exception_SoftwareBreakpointA64:
                case Exception_SoftwareBreakpointA32: {
                    n = GDB_ParseExceptionFrame(buffer, ctx, SIGTRAP);
                    strcat(buffer + n, "swbreak:;");
                }
                
                default: {
                    DEBUG("GDB: oops, unhandled exception for core id %u\n", info->coreId);
                    break;
                }
            }
            break;
        }

        case DBGEVENT_OUTPUT_STRING: {
            uintptr_t addr = info->outputString.address;
            size_t remaining = info->outputString.size;
            size_t sent = 0;
            size_t total = 0;
            while (remaining > 0) {
                size_t pending = (GDB_BUF_LEN - 1) / 2;
                pending = pending < remaining ? pending : remaining;

                int res = GDB_SendMemory(ctx, "O", 1, addr + sent, pending);
                if(res < 0 || res != 5 + 2 * pending)
                    break;

                sent += pending;
                remaining -= pending;
                total += res;
            }

            return (int)total;
        }

        // TODO: HIO

        default: {
            DEBUG("GDB: unknown exception type %u, core id %u\n", (u32)info->type, info->coreId);
            break;
        }
    }

    if (buffer[0] == 0) {
        return 0;
    } else {
        return GDB_SendPacket(ctx, buffer, strlen(buffer));
    }
}
