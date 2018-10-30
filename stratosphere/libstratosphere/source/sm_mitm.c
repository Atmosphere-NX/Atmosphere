/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include <switch/arm/atomics.h>
#include <stratosphere/mitm/sm_mitm.h>

static Handle g_smMitmHandle = INVALID_HANDLE;
static u64 g_refCnt;

Result smMitMInitialize(void) {
    atomicIncrement64(&g_refCnt);

    if (g_smMitmHandle != INVALID_HANDLE)
        return 0;

    Result rc = svcConnectToNamedPort(&g_smMitmHandle, "sm:");

    if (R_SUCCEEDED(rc)) {
        IpcCommand c;
        ipcInitialize(&c);
        ipcSendPid(&c);

        struct {
            u64 magic;
            u64 cmd_id;
            u64 zero;
            u64 reserved[2];
        } *raw;

        raw = ipcPrepareHeader(&c, sizeof(*raw));

        raw->magic = SFCI_MAGIC;
        raw->cmd_id = 0;
        raw->zero = 0;

        rc = ipcDispatch(g_smMitmHandle);

        if (R_SUCCEEDED(rc)) {
            IpcParsedCommand r;
            ipcParse(&r);

            struct {
                u64 magic;
                u64 result;
            } *resp = r.Raw;

            rc = resp->result;
        }
    }

    if (R_FAILED(rc))
        smExit();

    return rc;
}

void smMitMExit(void) {
    if (atomicDecrement64(&g_refCnt) == 0) {
        svcCloseHandle(g_smMitmHandle);
        g_smMitmHandle = INVALID_HANDLE;
    }
}

Result smMitMGetService(Service* service_out, const char *name_str)
{
    u64 name = smEncodeName(name_str);
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
        u64 reserved[2];
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 1;
    raw->service_name = name;

    Result rc = ipcDispatch(g_smMitmHandle);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            service_out->type = ServiceType_Normal;
            service_out->handle = r.Handles[0];
        }
    }

    return rc;
}


Result smMitMInstall(Handle *handle_out, Handle *query_out, const char *name) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65000;
    raw->service_name = smEncodeName(name);

    Result rc = ipcDispatch(g_smMitmHandle);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
            *query_out = r.Handles[1];
        }
    }

    return rc;
}

Result smMitMUninstall(const char *name) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
        u64 reserved;
    } *raw;

    raw = ipcPrepareHeader(&c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65001;
    raw->service_name = smEncodeName(name);

    Result rc = ipcDispatch(g_smMitmHandle);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        ipcParse(&r);

        struct {
            u64 magic;
            u64 result;
        } *resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}