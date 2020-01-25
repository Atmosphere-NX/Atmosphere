/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include <string.h>
#include <assert.h>

#include "../exceptions.h"
#include "../fpu.h"

#include "regs.h"
#include "net.h"

// GDB treats cpsr, fpsr, fpcr as 32-bit integers...

GDB_DECLARE_HANDLER(ReadRegisters)
{
    ENSURE(ctx->selectedThreadId == currentCoreCtx->coreId);

    const ExceptionStackFrame *frame = currentCoreCtx->guestFrame;
    const FpuRegisterCache *fpuRegCache = fpuReadRegisters();

    char *buf = ctx->buffer + 1;
    size_t n = 0;

    struct PACKED ALIGN(4) {
        u64 sp;
        u64 pc;
        u32 cpsr;
    } cpuSprs = {
        .sp = *exceptionGetSpPtr(frame),
        .pc = frame->elr_el2,
        .cpsr = (u32)frame->spsr_el2,
    };
    static_assert(sizeof(cpuSprs) == 12, "sizeof(cpuSprs) != 12");

    u32 fpuSprs[2] = {
        (u32)fpuRegCache->fpsr,
        (u32)fpuRegCache->fpcr,
    };


    n += GDB_EncodeHex(buf + n, frame->x, sizeof(frame->x));
    n += GDB_EncodeHex(buf + n, &cpuSprs, sizeof(cpuSprs));
    n += GDB_EncodeHex(buf + n, fpuRegCache->q, sizeof(fpuRegCache->q));
    n += GDB_EncodeHex(buf + n, fpuSprs, sizeof(fpuSprs));

    return GDB_SendPacket(ctx, buf, n);
}

GDB_DECLARE_HANDLER(WriteRegisters)
{
    ENSURE(ctx->selectedThreadId == currentCoreCtx->coreId);

    ExceptionStackFrame *frame = currentCoreCtx->guestFrame;
    FpuRegisterCache *fpuRegCache = fpuGetRegisterCache();

    char *buf = ctx->commandData;
    char *tmp = ctx->workBuffer;

    size_t n = 0;
    size_t m = 0;

    struct PACKED ALIGN(4) {
        u64 sp;
        u64 pc;
        u32 cpsr;
    } cpuSprs;
    static_assert(sizeof(cpuSprs) == 12, "sizeof(cpuSprs) != 12");

    u32 fpuSprs[2];

    struct {
        void *dst;
        size_t sz;
    } info[4] = {
        { frame->x,         sizeof(frame->x)        },
        { &cpuSprs,         sizeof(cpuSprs)         },
        { fpuRegCache->q,   sizeof(fpuRegCache->q)  },
        { fpuSprs,          sizeof(fpuSprs)         },
    };

    // Parse & return on error
    for (u32 i = 0; i < 4; i++) {
        if (GDB_DecodeHex(tmp + m, buf + n, info[i].sz) != info[i].sz) {
            return GDB_ReplyErrno(ctx, EPERM);
        }
        n += 2 * info[i].sz;
        m += info[i].sz;
    }

    // Copy. Note: we don't check if cpsr (spsr_el2) was modified to return to EL2...
    m = 0;
    for (u32 i = 0; i < 4; i++) {
        memcpy(info[i].dst, tmp + m, info[i].sz);
        m += info[i].sz;
    }
    *exceptionGetSpPtr(frame) = cpuSprs.sp;
    frame->elr_el2 = cpuSprs.pc;
    frame->spsr_el2 = cpuSprs.cpsr;
    fpuRegCache->fpsr = fpuSprs[0];
    fpuRegCache->fpcr = fpuSprs[1];
    fpuCommitRegisters();

    return GDB_ReplyOk(ctx);
}

static void GDB_GetRegisterPointerAndSize(size_t *outSz, void **outPtr, unsigned long id, ExceptionStackFrame *frame, FpuRegisterCache *fpuRegCache)
{
    switch (id) {
        case 0 ... 30:
            *outPtr = &frame->x[id];
            *outSz = 8;
            break;
        case 31:
            *outPtr = exceptionGetSpPtr(frame);
            *outSz = 8;
            break;
        case 32:
            *outPtr = &frame->spsr_el2;
            *outSz = 4;
            break;
        case 33 ... 64:
            *outPtr = &fpuRegCache->q[id - 33];
            *outSz = 16;
        case 65:
            *outPtr = &fpuRegCache->fpsr;
            *outSz = 4;
        case 66:
            *outPtr = &fpuRegCache->fpcr;
            *outSz = 4;
        default:
            __builtin_unreachable();
            return;
    }
}

GDB_DECLARE_HANDLER(ReadRegister)
{
    ENSURE(ctx->selectedThreadId == currentCoreCtx->coreId);

    const ExceptionStackFrame *frame = currentCoreCtx->guestFrame;
    const FpuRegisterCache *fpuRegCache = NULL;

    char *buf = ctx->buffer + 1;
    unsigned long gdbRegNum;

    if (GDB_ParseHexIntegerList(&gdbRegNum, ctx->commandData, 1, 0) == NULL) {
        return GDB_ReplyErrno(ctx, EILSEQ);
    }

    // Check the register number
    if (gdbRegNum >= 31 + 3 + 32 + 2) {
        return GDB_ReplyErrno(ctx, EINVAL);
    }

    if (gdbRegNum > 31 + 3) {
        // FPU register -- must read the FPU registers first
        fpuRegCache = fpuReadRegisters();
    }

    size_t sz;
    void *regPtr;
    GDB_GetRegisterPointerAndSize(&sz, &regPtr, gdbRegNum, frame, fpuRegCache);

    return GDB_SendHexPacket(ctx, regPtr, sz);
}

GDB_DECLARE_HANDLER(WriteRegister)
{
    ENSURE(ctx->selectedThreadId == currentCoreCtx->coreId);

    ExceptionStackFrame *frame = currentCoreCtx->guestFrame;
    FpuRegisterCache *fpuRegCache = fpuGetRegisterCache();

    char *tmp = ctx->workBuffer;
    unsigned long gdbRegNum;

    const char *valueStart = GDB_ParseHexIntegerList(&gdbRegNum, ctx->commandData, 1, '=');
    if(valueStart == NULL || *valueStart != '=') {
        return GDB_ReplyErrno(ctx, EILSEQ);
    }
    valueStart++;

    // Check the register number
    if (gdbRegNum >= 31 + 3 + 32 + 2) {
        return GDB_ReplyErrno(ctx, EINVAL);
    }

    size_t sz;
    void *regPtr;
    GDB_GetRegisterPointerAndSize(&sz, &regPtr, gdbRegNum, frame, fpuRegCache);

    // Check if we got 2 hex digits per byte
    if (strlen(valueStart) != 2 * sz) {
        return GDB_ReplyErrno(ctx, EILSEQ);
    }

    // Decode, check for errors
    if (GDB_DecodeHex(tmp, valueStart, sz) != sz) {
        return GDB_ReplyErrno(ctx, EILSEQ);
    }

    memcpy(regPtr, tmp, sz);

    if (gdbRegNum > 31 + 3) {
        // FPU register -- must commit the FPU registers
        fpuCommitRegisters();
    }

    else
        return GDB_ReplyOk(ctx);
}
