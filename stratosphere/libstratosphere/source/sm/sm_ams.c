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
#include <switch/arm/atomics.h>
#include "sm_ams.h"

static Service g_smMitmSrv;
static u64 g_mitmRefCnt;

Result smAtmosphereHasService(bool *out, const char *name) {
    IpcCommand c;
    ipcInitialize(&c);
    Service *srv = smGetServiceSession();

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65100;
    raw->service_name = smEncodeName(name);

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 has_service;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->has_service != 0;
        }
    }

    return rc;
}

Result smAtmosphereWaitService(const char *name) {
    IpcCommand c;
    ipcInitialize(&c);
    Service *srv = smGetServiceSession();

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65101;
    raw->service_name = smEncodeName(name);

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result smAtmosphereHasMitm(bool *out, const char *name) {
    IpcCommand c;
    ipcInitialize(&c);
    Service *srv = smGetServiceSession();

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65004;
    raw->service_name = smEncodeName(name);

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u8 has_mitm;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *out = resp->has_mitm != 0;
        }
    }

    return rc;
}

Result smAtmosphereWaitMitm(const char *name) {
    IpcCommand c;
    ipcInitialize(&c);
    Service *srv = smGetServiceSession();

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65005;
    raw->service_name = smEncodeName(name);

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result smAtmosphereMitmInitialize(void) {
    atomicIncrement64(&g_mitmRefCnt);

    if (serviceIsActive(&g_smMitmSrv))
        return 0;

    Handle sm_handle;
    Result rc = svcConnectToNamedPort(&sm_handle, "sm:");
    while (R_VALUE(rc) == KERNELRESULT(NotFound)) {
        svcSleepThread(50000000ul);
        rc = svcConnectToNamedPort(&sm_handle, "sm:");
    }

    if (R_SUCCEEDED(rc)) {
        serviceCreate(&g_smMitmSrv, sm_handle);
    }

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

        raw = serviceIpcPrepareHeader(&g_smMitmSrv, &c, sizeof(*raw));

        raw->magic = SFCI_MAGIC;
        raw->cmd_id = 0;
        raw->zero = 0;

        rc = serviceIpcDispatch(&g_smMitmSrv);

        if (R_SUCCEEDED(rc)) {
            IpcParsedCommand r;

            struct {
                u64 magic;
                u64 result;
            } *resp;
            serviceIpcParse(&g_smMitmSrv, &r, sizeof(*resp));

            resp = r.Raw;
            rc = resp->result;
        }
    }

    if (R_FAILED(rc))
        smAtmosphereMitmExit();

    return rc;
}

void smAtmosphereMitmExit(void) {
    if (atomicDecrement64(&g_mitmRefCnt) == 0) {
        serviceClose(&g_smMitmSrv);
    }
}

Result smAtmosphereMitmInstall(Handle *handle_out, Handle *query_out, const char *name) {
    IpcCommand c;
    ipcInitialize(&c);
    Service *srv = &g_smMitmSrv;

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65000;
    raw->service_name = smEncodeName(name);

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;
        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            *handle_out = r.Handles[0];
            *query_out = r.Handles[1];
        }
    }

    return rc;
}

Result smAtmosphereMitmUninstall(const char *name) {
    IpcCommand c;
    ipcInitialize(&c);
    Service *srv = &g_smMitmSrv;

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65001;
    raw->service_name = smEncodeName(name);

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;
        rc = resp->result;
    }

    return rc;
}

Result smAtmosphereMitmDeclareFuture(const char *name) {
    IpcCommand c;
    ipcInitialize(&c);
    Service *srv = &g_smMitmSrv;

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65006;
    raw->service_name = smEncodeName(name);

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;
        rc = resp->result;
    }

    return rc;
}

Result smAtmosphereMitmAcknowledgeSession(Service *srv_out, u64 *pid_out, u64 *tid_out, const char *name) {
    IpcCommand c;
    ipcInitialize(&c);
    Service *srv = &g_smMitmSrv;

    struct {
        u64 magic;
        u64 cmd_id;
        u64 service_name;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65003;
    raw->service_name = smEncodeName(name);

    Result rc = serviceIpcDispatch(srv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 pid;
            u64 tid;
        } *resp;

        serviceIpcParse(srv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            *pid_out = resp->pid;
            *tid_out = resp->tid;
            serviceCreate(srv_out, r.Handles[0]);
        }
    }

    return rc;
}
