/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include "gdb/thread.h"
#include "gdb/net.h"
#include "fmt.h"
#include <stdlib.h>

static s32 GDB_GetDynamicThreadPriority(GDBContext *ctx, u32 threadId)
{
    Handle process, thread;
    Result r;
    s32 prio = 65;

    r = svcOpenProcess(&process, ctx->pid);
    if(R_FAILED(r))
        return 65;

    r = svcOpenThread(&thread, process, threadId);
    if(R_FAILED(r))
        goto cleanup;

    r = svcGetThreadPriority(&prio, thread);

cleanup:
    svcCloseHandle(thread);
    svcCloseHandle(process);

    return prio;
}

struct ThreadIdWithCtx
{
    GDBContext *ctx;
    u32 id;
};

static int thread_compare_func(const void *a_, const void *b_)
{
    const struct ThreadIdWithCtx *a = (const struct ThreadIdWithCtx *)a_;
    const struct ThreadIdWithCtx *b = (const struct ThreadIdWithCtx *)b_;

    u32 maskA = 2, maskB = 2;
    s32 prioAStatic = 65, prioBStatic = 65;
    s32 prioADynamic = GDB_GetDynamicThreadPriority(a->ctx, a->id);
    s32 prioBDynamic = GDB_GetDynamicThreadPriority(b->ctx, b->id);
    s64 dummy;

    svcGetDebugThreadParam(&dummy, &maskA, a->ctx->debug, a->id, DBGTHREAD_PARAMETER_SCHEDULING_MASK_LOW);
    svcGetDebugThreadParam(&dummy, &maskB, b->ctx->debug, b->id, DBGTHREAD_PARAMETER_SCHEDULING_MASK_LOW);
    svcGetDebugThreadParam(&dummy, (u32 *)&prioAStatic, a->ctx->debug, a->id, DBGTHREAD_PARAMETER_PRIORITY);
    svcGetDebugThreadParam(&dummy, (u32 *)&prioBStatic, b->ctx->debug, b->id, DBGTHREAD_PARAMETER_PRIORITY);

    if(maskA == 1 && maskB != 1)
        return -1;
    else if(maskA != 1 && maskB == 1)
        return 1;
    else if(prioADynamic != prioBDynamic)
        return prioADynamic - prioBDynamic;
    else
        return prioAStatic - prioBStatic;
}

u32 GDB_GetCurrentThreadFromList(GDBContext *ctx, u32 *threadIds, u32 nbThreads)
{
    struct ThreadIdWithCtx lst[MAX_DEBUG_THREAD];
    for(u32 i = 0; i < nbThreads; i++)
    {
        lst[i].ctx = ctx;
        lst[i].id = threadIds[i];
    }

    qsort(lst, nbThreads, sizeof(struct ThreadIdWithCtx), thread_compare_func);
    return lst[0].id;
}

u32 GDB_GetCurrentThread(GDBContext *ctx)
{
    u32 threadIds[MAX_DEBUG_THREAD];

    if(ctx->nbThreads == 0)
        return 0;

    for(u32 i = 0; i < ctx->nbThreads; i++)
        threadIds[i] = ctx->threadInfos[i].id;

    return GDB_GetCurrentThreadFromList(ctx, threadIds, ctx->nbThreads);
}

GDB_DECLARE_HANDLER(SetThreadId)
{
    if(ctx->commandData[0] == 'g')
    {
        if(strncmp(ctx->commandData + 1, "-1", 2) == 0)
            return GDB_ReplyErrno(ctx, EILSEQ); // a thread must be specified

        u32 id;
        if(GDB_ParseHexIntegerList(&id, ctx->commandData + 1, 1, 0) == NULL)
            return GDB_ReplyErrno(ctx, EILSEQ);
        ctx->selectedThreadId = id;
        return GDB_ReplyOk(ctx);
    }
    else if(ctx->commandData[0] == 'c')
    {
        // We can't stop/continue particular threads (uncompliant behavior)
        if(strncmp(ctx->commandData + 1, "-1", 2) == 0)
            ctx->selectedThreadIdForContinuing = 0;
        else
        {
            u32 id;
            if(GDB_ParseHexIntegerList(&id, ctx->commandData + 1, 1, 0) == NULL)
                return GDB_ReplyErrno(ctx, EILSEQ);
            ctx->selectedThreadIdForContinuing = id;
        }

        return GDB_ReplyOk(ctx);
    }
    else
        return GDB_ReplyErrno(ctx, EPERM);
}

GDB_DECLARE_HANDLER(IsThreadAlive)
{
    u32 threadId;
    s64 dummy;
    u32 mask;

    if(GDB_ParseHexIntegerList(&threadId, ctx->commandData, 1, 0) == NULL)
        return GDB_ReplyErrno(ctx, EILSEQ);

    Result r = svcGetDebugThreadParam(&dummy, &mask, ctx->debug, threadId, DBGTHREAD_PARAMETER_SCHEDULING_MASK_LOW);
    if(R_SUCCEEDED(r) && mask != 2)
        return GDB_ReplyOk(ctx);
    else
        return GDB_ReplyErrno(ctx, EPERM);
}

GDB_DECLARE_QUERY_HANDLER(CurrentThreadId)
{
    if(ctx->currentThreadId == 0)
        ctx->currentThreadId = GDB_GetCurrentThread(ctx);

    return ctx->currentThreadId != 0 ? GDB_SendFormattedPacket(ctx, "QC%lx", ctx->currentThreadId) : GDB_ReplyErrno(ctx, EPERM);
}

static void GDB_GenerateThreadListData(GDBContext *ctx)
{
    u32 aliveThreadIds[MAX_DEBUG_THREAD];
    u32 nbAliveThreads = 0; // just in case. This is probably redundant

    for(u32 i = 0; i < ctx->nbThreads; i++)
    {
        s64 dummy;
        u32 mask;

        Result r = svcGetDebugThreadParam(&dummy, &mask, ctx->debug, ctx->threadInfos[i].id, DBGTHREAD_PARAMETER_SCHEDULING_MASK_LOW);
        if(R_SUCCEEDED(r) && mask != 2)
            aliveThreadIds[nbAliveThreads++] = ctx->threadInfos[i].id;
    }

    if(nbAliveThreads == 0)
        ctx->threadListData[0] = 0;

    char *bufptr = ctx->threadListData;

    for(u32 i = 0; i < nbAliveThreads; i++)
        bufptr += sprintf(bufptr, i == (nbAliveThreads - 1) ? "%lx" : "%lx,", aliveThreadIds[i]);
}

static int GDB_SendThreadData(GDBContext *ctx)
{
    u32 sz = strlen(ctx->threadListData);
    u32 len;
    if(ctx->threadListDataPos >= sz)
        len = 0;
    else if(sz - ctx->threadListDataPos <= GDB_BUF_LEN - 1)
        len = sz - ctx->threadListDataPos;
    else
    {
        for(len = GDB_BUF_LEN - 1; ctx->threadListData[ctx->threadListDataPos + len] != ',' && len > 0; len--);
        if(len > 0)
            len--;
    }

    int n = GDB_SendStreamData(ctx, ctx->threadListData, ctx->threadListDataPos, len, sz, true);

    if(ctx->threadListDataPos >= sz)
    {
        ctx->threadListDataPos = 0;
        ctx->threadListData[0] = 0;
    }
    else
        ctx->threadListDataPos += len;

    return n;
}

GDB_DECLARE_QUERY_HANDLER(fThreadInfo)
{
    if(ctx->threadListData[0] == 0)
        GDB_GenerateThreadListData(ctx);

    return GDB_SendThreadData(ctx);
}

GDB_DECLARE_QUERY_HANDLER(sThreadInfo)
{
    return GDB_SendThreadData(ctx);
}

GDB_DECLARE_QUERY_HANDLER(ThreadEvents)
{
    switch(ctx->commandData[0])
    {
        case '0':
            ctx->catchThreadEvents = false;
            return GDB_ReplyOk(ctx);
        case '1':
            ctx->catchThreadEvents = true;
            return GDB_ReplyOk(ctx);
        default:
            return GDB_ReplyErrno(ctx, EILSEQ);
    }
}

GDB_DECLARE_QUERY_HANDLER(ThreadExtraInfo)
{
    u32 id;
    s64 dummy;
    u32 val;
    Result r;
    int n;

    const char *sStatus;
    char sThreadDynamicPriority[64], sThreadStaticPriority[64];
    char sCoreIdeal[64], sCoreCreator[64];
    char buf[512];

    u32 tls = 0;

    if(GDB_ParseHexIntegerList(&id, ctx->commandData, 1, 0) == NULL)
        return GDB_ReplyErrno(ctx, EILSEQ);

    for(u32 i = 0; i < MAX_DEBUG_THREAD; i++)
    {
        if(ctx->threadInfos[i].id == id)
            tls = ctx->threadInfos[i].tls;
    }

    r = svcGetDebugThreadParam(&dummy, &val, ctx->debug, id, DBGTHREAD_PARAMETER_SCHEDULING_MASK_LOW);
    sStatus = R_SUCCEEDED(r) ? (val == 1 ? ", running, " : ", idle, ") : "";

    val = (u32)GDB_GetDynamicThreadPriority(ctx, id);
    if(val == 65)
        sThreadDynamicPriority[0] = 0;
    else
        sprintf(sThreadDynamicPriority, "dynamic prio.: %ld, ", (s32)val);

    r = svcGetDebugThreadParam(&dummy, &val, ctx->debug, id, DBGTHREAD_PARAMETER_PRIORITY);
    if(R_FAILED(r))
        sThreadStaticPriority[0] = 0;
    else
        sprintf(sThreadStaticPriority, "static prio.: %ld, ", (s32)val);

    r = svcGetDebugThreadParam(&dummy, &val, ctx->debug, id, DBGTHREAD_PARAMETER_CPU_IDEAL);
    if(R_FAILED(r))
        sCoreIdeal[0] = 0;
    else
        sprintf(sCoreIdeal, "ideal core: %lu, ", val);

    r = svcGetDebugThreadParam(&dummy, &val, ctx->debug, id, DBGTHREAD_PARAMETER_CPU_CREATOR); // Creator = "first ran, and running the thread"
    if(R_FAILED(r))
        sCoreCreator[0] = 0;
    else
        sprintf(sCoreCreator, "running on core %lu", val);

    n = sprintf(buf, "TLS: 0x%08lx%s%s%s%s%s", tls, sStatus, sThreadDynamicPriority, sThreadStaticPriority,
                sCoreIdeal, sCoreCreator);

    return GDB_SendHexPacket(ctx, buf, (u32)n);
}

GDB_DECLARE_QUERY_HANDLER(GetTLSAddr)
{
    u32 lst[3];
    if(GDB_ParseHexIntegerList(lst, ctx->commandData, 3, 0) == NULL)
        return GDB_ReplyErrno(ctx, EILSEQ);

    // We don't care about the 'lm' parameter...
    u32 id = lst[0];
    u32 offset = lst[1];

    u32 tls = 0;

    for(u32 i = 0; i < MAX_DEBUG_THREAD; i++)
    {
        if(ctx->threadInfos[i].id == id)
            tls = ctx->threadInfos[i].tls;
    }

    if(tls == 0)
        return GDB_ReplyErrno(ctx, EINVAL);

    return GDB_SendFormattedPacket(ctx, "%08lx", tls + offset);
}
