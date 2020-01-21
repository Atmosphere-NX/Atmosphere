/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include "gdb/mem.h"
#include "gdb/net.h"
#include "utils.h"

static void *k_memcpy_no_interrupt(void *dst, const void *src, u32 len)
{
    __asm__ volatile("cpsid aif");
    return memcpy(dst, src, len);
}

Result GDB_ReadTargetMemoryInPage(void *out, GDBContext *ctx, u32 addr, u32 len)
{
    s64 TTBCR;
    svcGetSystemInfo(&TTBCR, 0x10002, 0);

    if(addr < (1u << (32 - (u32)TTBCR))) // Note: UB with user-mapped MMIO (uses memcpy).
        return svcReadProcessMemory(out, ctx->debug, addr, len);
    else if(!ctx->enableExternalMemoryAccess)
        return -1;
    else if(addr >= 0x80000000 && addr < 0xB0000000)
    {
        if(addr >= 0x90000000 && addr < 0x98000000) // IO
        {
            for(u32 off = 0; off < len; )
            {
                if((addr + off) & 1)
                {
                    *((u8 *)out + off) = *(vu8 *)(addr + off);
                    off += 1;
                }
                else if((addr + off) & 3)
                {
                    *((u16 *)out + off) = *(vu16 *)(addr + off);
                    off += 2;
                }
                else
                {
                    *((u32 *)out + off) = *(vu32 *)(addr + off);
                    off += 4;
                }
            }
        }
        else memcpy(out, (const void *)addr, len);
        return 0;
    }
    else
    {
        u32 PA = svcConvertVAToPA((const void *)addr, false);

        if(PA == 0)
            return -1;
        else
        {
            svcCustomBackdoor(k_memcpy_no_interrupt, out, (const void *)addr, len);
            return 0;
        }
    }
}

Result GDB_WriteTargetMemoryInPage(GDBContext *ctx, const void *in, u32 addr, u32 len)
{
    s64 TTBCR;
    svcGetSystemInfo(&TTBCR, 0x10002, 0);

    if(addr < (1u << (32 - (u32)TTBCR)))
        return svcWriteProcessMemory(ctx->debug, in, addr, len); // not sure if it checks if it's IO or not. It probably does
    else if(!ctx->enableExternalMemoryAccess)
        return -1;
    else if(addr >= 0x80000000 && addr < 0xB0000000)
    {
        if(addr >= 0x90000000 && addr < 0x98000000) // IO
        {
            for(u32 off = 0; off < len; )
            {
                if((addr + off) & 1)
                {
                    *(vu8 *)(addr + off) = *((u8 *)in + off);
                    off += 1;
                }
                else if((addr + off) & 3)
                {
                    *(vu16 *)(addr + off) = *((u16 *)in + off);
                    off += 2;
                }
                else
                {
                    *(vu32 *)(addr + off) = *((u32 *)in + off);
                    off += 4;
                }
            }
        }
        else memcpy((void *)addr, in, len);
        return 0;
    }
    else
    {
        u32 PA = svcConvertVAToPA((const void *)addr, true);

        if(PA != 0)
        {
            svcCustomBackdoor(k_memcpy_no_interrupt, (void *)addr, in, len);
            return 0;
        }
        else
        {
            // Unreliable, use at your own risk

            svcFlushEntireDataCache();
            svcInvalidateEntireInstructionCache();
            Result ret = GDB_WriteTargetMemoryInPage(ctx, PA_FROM_VA_PTR(in), addr, len);
            svcFlushEntireDataCache();
            svcInvalidateEntireInstructionCache();
            return ret;
        }
    }
}

u32 GDB_ReadTargetMemory(void *out, GDBContext *ctx, u32 addr, u32 len)
{
    Result r = 0;
    u32 remaining = len, total = 0;
    u8 *out8 = (u8 *)out;
    do
    {
        u32 nb = (remaining > 0x1000 - (addr & 0xFFF)) ? 0x1000 - (addr & 0xFFF) : remaining;
        r = GDB_ReadTargetMemoryInPage(out8 + total, ctx, addr, nb);
        if(R_SUCCEEDED(r))
        {
            addr += nb;
            total += nb;
            remaining -= nb;
        }
    }
    while(remaining > 0 && R_SUCCEEDED(r));

    return total;
}

u32 GDB_WriteTargetMemory(GDBContext *ctx, const void *in, u32 addr, u32 len)
{
    Result r = 0;
    u32 remaining = len, total = 0;
    do
    {
        u32 nb = (remaining > 0x1000 - (addr & 0xFFF)) ? 0x1000 - (addr & 0xFFF) : remaining;
        r = GDB_WriteTargetMemoryInPage(ctx, (u8 *)in + total, addr, nb);
        if(R_SUCCEEDED(r))
        {
            addr += nb;
            total += nb;
            remaining -= nb;
        }
    }
    while(remaining > 0 && R_SUCCEEDED(r));

    return total;
}

int GDB_SendMemory(GDBContext *ctx, const char *prefix, u32 prefixLen, u32 addr, u32 len)
{
    char buf[GDB_BUF_LEN];
    u8 membuf[GDB_BUF_LEN / 2];

    if(prefix != NULL)
        memcpy(buf, prefix, prefixLen);
    else
        prefixLen = 0;

    if(prefixLen + 2 * len > GDB_BUF_LEN) // gdb shouldn't send requests which responses don't fit in a packet
        return prefix == NULL ? GDB_ReplyErrno(ctx, ENOMEM) : -1;

    u32 total = GDB_ReadTargetMemory(membuf, ctx, addr, len);
    if(total == 0)
        return prefix == NULL ? GDB_ReplyErrno(ctx, EFAULT) : -EFAULT;
    else
    {
        GDB_EncodeHex(buf + prefixLen, membuf, total);
        return GDB_SendPacket(ctx, buf, prefixLen + 2 * total);
    }
}

int GDB_WriteMemory(GDBContext *ctx, const void *buf, u32 addr, u32 len)
{
    u32 total = GDB_WriteTargetMemory(ctx, buf, addr, len);
    if(total != len)
        return GDB_ReplyErrno(ctx, EFAULT);
    else
        return GDB_ReplyOk(ctx);
}

u32 GDB_SearchMemory(bool *found, GDBContext *ctx, u32 addr, u32 len, const void *pattern, u32 patternLen)
{
    u8 buf[0x1000 + 0x1000 * ((GDB_BUF_LEN + 0xFFF) / 0x1000)];
    u32 maxNbPages = 1 + ((GDB_BUF_LEN + 0xFFF) / 0x1000);
    u32 curAddr = addr;

    s64 TTBCR;
    svcGetSystemInfo(&TTBCR, 0x10002, 0);
    while(curAddr < addr + len)
    {
        u32 nbPages;
        u32 addrBase = curAddr & ~0xFFF, addrDispl = curAddr & 0xFFF;

        for(nbPages = 0; nbPages < maxNbPages; nbPages++)
        {
            if(addr >= (1u << (32 - (u32)TTBCR)))
            {
                u32 PA = svcConvertVAToPA((const void *)addr, false);
                if(PA == 0 || (PA >= 0x10000000 && PA <= 0x18000000))
                    break;
            }

            if(R_FAILED(GDB_ReadTargetMemoryInPage(buf + 0x1000 * nbPages, ctx, addrBase + nbPages * 0x1000, 0x1000)))
                break;
        }

        u8 *pos = NULL;
        if(addrDispl + patternLen <= 0x1000 * nbPages)
            pos = memsearch(buf + addrDispl, pattern, 0x1000 * nbPages - addrDispl, patternLen);

        if(pos != NULL)
        {
            *found = true;
            return addrBase + (pos - buf);
        }

        curAddr = addrBase + 0x1000;
    }

    *found = false;
    return 0;
}

GDB_DECLARE_HANDLER(ReadMemory)
{
    u32 lst[2];
    if(GDB_ParseHexIntegerList(lst, ctx->commandData, 2, 0) == NULL)
        return GDB_ReplyErrno(ctx, EILSEQ);

    u32 addr = lst[0];
    u32 len = lst[1];

    return GDB_SendMemory(ctx, NULL, 0, addr, len);
}

GDB_DECLARE_HANDLER(WriteMemory)
{
    u32 lst[2];
    const char *dataStart = GDB_ParseHexIntegerList(lst, ctx->commandData, 2, ':');
    if(dataStart == NULL || *dataStart != ':')
        return GDB_ReplyErrno(ctx, EILSEQ);

    dataStart++;
    u32 addr = lst[0];
    u32 len = lst[1];

    if(dataStart + 2 * len >= ctx->buffer + 4 + GDB_BUF_LEN)
        return GDB_ReplyErrno(ctx, ENOMEM);

    u8 data[GDB_BUF_LEN / 2];
    u32 n = GDB_DecodeHex(data, dataStart, len);

    if(n != len)
        return GDB_ReplyErrno(ctx, EILSEQ);

    return GDB_WriteMemory(ctx, data, addr, len);
}

GDB_DECLARE_HANDLER(WriteMemoryRaw)
{
    u32 lst[2];
    const char *dataStart = GDB_ParseHexIntegerList(lst, ctx->commandData, 2, ':');
    if(dataStart == NULL || *dataStart != ':')
        return GDB_ReplyErrno(ctx, EILSEQ);

    dataStart++;
    u32 addr = lst[0];
    u32 len = lst[1];

    if(dataStart + len >= ctx->buffer + 4 + GDB_BUF_LEN)
        return GDB_ReplyErrno(ctx, ENOMEM);

    u8 data[GDB_BUF_LEN];
    u32 n = GDB_UnescapeBinaryData(data, dataStart, len);

    if(n != len)
        return GDB_ReplyErrno(ctx, n);

    return GDB_WriteMemory(ctx, data, addr, len);
}

GDB_DECLARE_QUERY_HANDLER(SearchMemory)
{
    u32 lst[2];
    u32 addr, len;
    u8 pattern[GDB_BUF_LEN];
    const char *patternStart;
    u32 patternLen;
    bool found;
    u32 foundAddr;

    if(strncmp(ctx->commandData, "memory:", 7) != 0)
        return GDB_ReplyErrno(ctx, EILSEQ);

    ctx->commandData += 7;
    patternStart = GDB_ParseIntegerList(lst, ctx->commandData, 2, ';', ';', 16, false);
    if(patternStart == NULL || *patternStart != ';')
        return GDB_ReplyErrno(ctx, EILSEQ);

    addr = lst[0];
    len = lst[1];

    patternStart++;
    patternLen = ctx->commandEnd - patternStart;

    patternLen = GDB_UnescapeBinaryData(pattern, patternStart, patternLen);

    foundAddr = GDB_SearchMemory(&found, ctx, addr, len, patternStart, patternLen);

    if(found)
        return GDB_SendFormattedPacket(ctx, "1,%x", foundAddr);
    else
        return GDB_SendPacket(ctx, "0", 1);
}
