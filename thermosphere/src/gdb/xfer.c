/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include <3ds/os.h>

#include "gdb/xfer.h"
#include "gdb/net.h"
#include "fmt.h"

#include "osdata_cfw_version_template_xml.h"
#include "osdata_memory_template_xml.h"
#include "osdata_xml.h"
#include "target_xml.h"

struct
{
    const char *name;
    int (*handler)(GDBContext *ctx, bool write, const char *annex, u32 offset, u32 length);
} xferCommandHandlers[] =
{
    { "features", GDB_XFER_HANDLER(Features) },
    { "osdata",   GDB_XFER_HANDLER(OsData) },
};

GDB_DECLARE_XFER_HANDLER(Features)
{
    if(strcmp(annex, "target.xml") != 0 || write)
        return GDB_ReplyEmpty(ctx);
    else
        return GDB_SendStreamData(ctx, (const char *)target_xml, offset, length, target_xml_size, false);
}

struct
{
    const char *name;
    int (*handler)(GDBContext *ctx, bool write, u32 offset, u32 length);
} xferOsDataCommandHandlers[] =
{
    { "cfwversion", GDB_XFER_OSDATA_HANDLER(CfwVersion) },
    { "memory",     GDB_XFER_OSDATA_HANDLER(Memory) },
    { "processes",  GDB_XFER_OSDATA_HANDLER(Processes) },
};

GDB_DECLARE_XFER_OSDATA_HANDLER(CfwVersion)
{
    if(write)
        return GDB_HandleUnsupported(ctx);
    else
    {
        char buf[512]; // Make sure this doesn't overflow
        char versionString[16];
        s64 out;
        u32 version, commitHash;
        bool isRelease;
        u32 sz;

        svcGetSystemInfo(&out, 0x10000, 0);
        version = (u32)out;

        svcGetSystemInfo(&out, 0x10000, 1);
        commitHash = (u32)out;

        svcGetSystemInfo(&out, 0x10000, 0x200);
        isRelease = (bool)out;

        if(GET_VERSION_REVISION(version) == 0)
            sprintf(versionString, "v%lu.%lu", GET_VERSION_MAJOR(version), GET_VERSION_MINOR(version));
        else
            sprintf(versionString, "v%lu.%lu.%lu", GET_VERSION_MAJOR(version), GET_VERSION_MINOR(version), GET_VERSION_REVISION(version));

        sz = (u32)sprintf(buf, (const char *)osdata_cfw_version_template_xml, versionString, commitHash, isRelease ? "Yes" : "No");

        return GDB_SendStreamData(ctx, buf, offset, length, sz, false);
    }
}

GDB_DECLARE_XFER_OSDATA_HANDLER(Memory)
{
    if(write)
        return GDB_HandleUnsupported(ctx);
    else
    {
        if(ctx->memoryOsInfoXmlData[0] == 0)
        {
            s64 out;
            u32 applicationUsed, systemUsed, baseUsed;
            u32 applicationTotal = *(vu32 *)0x1FF80040, systemTotal = *(vu32 *)0x1FF80044, baseTotal = *(vu32 *)0x1FF80048;

            svcGetSystemInfo(&out, 0, 1);
            applicationUsed = (u32)out;

            svcGetSystemInfo(&out, 0, 2);
            systemUsed = (u32)out;

            svcGetSystemInfo(&out, 0, 3);
            baseUsed = (u32)out;

            sprintf(ctx->memoryOsInfoXmlData, (const char *)osdata_memory_template_xml,
                applicationUsed, applicationTotal - applicationUsed, applicationTotal, (u32)((5ULL + ((1000ULL * applicationUsed) / applicationTotal)) / 10ULL),
                systemUsed, systemTotal - systemUsed, systemTotal, (u32)((5ULL + ((1000ULL * systemUsed) / systemTotal)) / 10ULL),
                baseUsed, baseTotal - baseUsed, baseTotal, (u32)((5ULL + ((1000ULL * baseUsed) / baseTotal)) / 10ULL)
            );
        }

        u32 size = strlen(ctx->memoryOsInfoXmlData);
        int n = GDB_SendStreamData(ctx, ctx->memoryOsInfoXmlData, offset, length, size, false);

        if(offset + length >= size)
            ctx->memoryOsInfoXmlData[0] = 0; // we're done, invalidate

        return n;
    }
}

GDB_DECLARE_XFER_OSDATA_HANDLER(Processes)
{
    if(write)
        return GDB_HandleUnsupported(ctx);
    else
    {
        if(ctx->processesOsInfoXmlData[0] == 0)
        {
            static const char header[] =
            /*"<?xml version=\"1.0\"?>"
            "<!DOCTYPE target SYSTEM \"osdata.dtd\">" IDA rejects the xml header*/
            "<osdata type=\"processes\">";

            static const char item[] =
            "<item>"
            "<column name=\"pid\">%lu</column>"
            "<column name=\"command\">%s</column>"
            "</item>";

            static const char footer[] = "</osdata>";

            int n;
            u32 pos = 0;

            u32 pidList[0x40];
            s32 processAmount;

            strcpy(ctx->processesOsInfoXmlData, header);
            pos = sizeof(header) - 1;
            svcGetProcessList(&processAmount, pidList, 0x40);

            for(s32 i = 0; i < processAmount; i++)
            {
                u32 pid = pidList[i];
                char name[9] = { 0 };
                s64 out;
                Handle processHandle;
                Result res = svcOpenProcess(&processHandle, pidList[i]);
                if(R_FAILED(res))
                    continue;

                svcGetProcessInfo(&out, processHandle, 0x10000);
                memcpy(name, &out, 8);
                svcCloseHandle(processHandle);

                n = sprintf(ctx->processesOsInfoXmlData + pos, item, pid, name);
                pos += (u32)n;
            }

            strcpy(ctx->processesOsInfoXmlData + pos, footer);
            pos = sizeof(footer) - 1;
        }

        u32 size = strlen(ctx->processesOsInfoXmlData);
        int n = GDB_SendStreamData(ctx, ctx->processesOsInfoXmlData, offset, length, size, false);

        if(offset + length >= size)
            ctx->processesOsInfoXmlData[0] = 0; // we're done, invalidate

        return n;
    }
}

GDB_DECLARE_XFER_HANDLER(OsData)
{
    if(strcmp(annex, "") == 0 && !write)
        return GDB_SendStreamData(ctx, (const char *)osdata_xml, offset, length, osdata_xml_size, false);
    else
    {
        for(u32 i = 0; i < sizeof(xferOsDataCommandHandlers) / sizeof(xferOsDataCommandHandlers[0]); i++)
        {
            if(strcmp(annex, xferOsDataCommandHandlers[i].name) == 0)
                return xferOsDataCommandHandlers[i].handler(ctx, write, offset, length);
        }
    }

    return GDB_HandleUnsupported(ctx);
}

GDB_DECLARE_QUERY_HANDLER(Xfer)
{
    const char *objectStart = ctx->commandData;
    char *objectEnd = (char*)strchr(objectStart, ':');
    if(objectEnd == NULL) return -1;
    *objectEnd = 0;

    char *opStart = objectEnd + 1;
    char *opEnd = (char*)strchr(opStart, ':');
    if(opEnd == NULL) return -1;
    *opEnd = 0;

    char *annexStart = opEnd + 1;
    char *annexEnd = (char*)strchr(annexStart, ':');
    if(annexEnd == NULL) return -1;
    *annexEnd = 0;

    const char *offStart = annexEnd + 1;
    u32 offset, length;

    bool write;
    const char *pos;
    if(strcmp(opStart, "read") == 0)
    {
        u32 lst[2];
        if(GDB_ParseHexIntegerList(lst, offStart, 2, 0) == NULL)
            return GDB_ReplyErrno(ctx, EILSEQ);

        offset = lst[0];
        length = lst[1];
        write = false;
    }
    else if(strcmp(opStart, "write") == 0)
    {
        pos = GDB_ParseHexIntegerList(&offset, offStart, 1, ':');
        if(pos == NULL || *pos++ != ':')
            return GDB_ReplyErrno(ctx, EILSEQ);

        u32 len = strlen(pos);
        if(len == 0 || (len % 2) != 0)
            return GDB_ReplyErrno(ctx, EILSEQ);
        length = len / 2;
        write = true;
    }
    else
        return GDB_ReplyErrno(ctx, EILSEQ);

    for(u32 i = 0; i < sizeof(xferCommandHandlers) / sizeof(xferCommandHandlers[0]); i++)
    {
        if(strcmp(objectStart, xferCommandHandlers[i].name) == 0)
        {
            if(write)
                ctx->commandData = (char *)pos;

            return xferCommandHandlers[i].handler(ctx, write, annexStart, offset, length);
        }
    }

    return GDB_HandleUnsupported(ctx);
}
