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
#define NX_SERVICE_ASSUME_NON_DOMAIN
#include "../service_guard.h"
#include "sm_ams.h"

static Result _smAtmosphereCmdHas(bool *out, u64 service_name, u32 cmd_id) {
    u8 tmp;
    Result rc = serviceDispatchInOut(smGetServiceSession(), cmd_id, service_name, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _smAtmosphereCmdInServiceNameNoOut(u64 service_name, Service *srv, u32 cmd_id) {
    return serviceDispatchIn(srv, cmd_id, service_name);
}

Result smAtmosphereHasService(bool *out, const char *name) {
    return _smAtmosphereCmdHas(out, smEncodeName(name), 65100);
}

Result smAtmosphereWaitService(const char *name) {
    return _smAtmosphereCmdInServiceNameNoOut(smEncodeName(name), smGetServiceSession(), 65101);
}

Result smAtmosphereHasMitm(bool *out, const char *name) {
    return _smAtmosphereCmdHas(out, smEncodeName(name), 65004);
}

Result smAtmosphereWaitMitm(const char *name) {
    return _smAtmosphereCmdInServiceNameNoOut(smEncodeName(name), smGetServiceSession(), 65005);
}

static Service g_smAtmosphereMitmSrv;

NX_GENERATE_SERVICE_GUARD(smAtmosphereMitm);

Result _smAtmosphereMitmInitialize(void) {
    Handle sm_handle;
    Result rc = svcConnectToNamedPort(&sm_handle, "sm:");
    while (R_VALUE(rc) == KERNELRESULT(NotFound)) {
        svcSleepThread(50000000ul);
        rc = svcConnectToNamedPort(&sm_handle, "sm:");
    }

    if (R_SUCCEEDED(rc)) {
        serviceCreate(&g_smAtmosphereMitmSrv, sm_handle);
    }

    if (R_SUCCEEDED(rc)) {
        const u64 pid_placeholder = 0;
        rc = serviceDispatchIn(&g_smAtmosphereMitmSrv, 0, pid_placeholder, .in_send_pid = true);
    }

    return rc;
}

void _smAtmosphereMitmCleanup(void) {
    serviceClose(&g_smAtmosphereMitmSrv);
}

Service* smAtmosphereMitmGetServiceSession(void) {
    return &g_smAtmosphereMitmSrv;
}

Result smAtmosphereMitmInstall(Handle *handle_out, Handle *query_out, const char *name) {
    const u64 in = smEncodeName(name);
    Handle tmp_handles[2];
    Result rc = serviceDispatchIn(&g_smAtmosphereMitmSrv, 65000, in,
        .out_handle_attrs = { SfOutHandleAttr_HipcMove, SfOutHandleAttr_HipcMove },
        .out_handles = tmp_handles,
    );

    if (R_SUCCEEDED(rc)) {
        *handle_out = tmp_handles[0];
        *query_out  = tmp_handles[1];
    }

    return rc;
}

Result smAtmosphereMitmUninstall(const char *name) {
    return _smAtmosphereCmdInServiceNameNoOut(smEncodeName(name), &g_smAtmosphereMitmSrv, 65001);
}

Result smAtmosphereMitmDeclareFuture(const char *name) {
    return _smAtmosphereCmdInServiceNameNoOut(smEncodeName(name), &g_smAtmosphereMitmSrv, 65006);
}

Result smAtmosphereMitmAcknowledgeSession(Service *srv_out, u64 *pid_out, u64 *tid_out, const char *name) {
    const u64 in = smEncodeName(name);
    struct {
        u64 pid;
        u64 tid;
    } out;

    Result rc = serviceDispatchInOut(&g_smAtmosphereMitmSrv, 65003, in, out,
        .out_num_objects = 1,
        .out_objects = srv_out,
    );

    if (R_SUCCEEDED(rc)) {
        if (pid_out) *pid_out = out.pid;
        if (tid_out) *tid_out = out.tid;
    }

    return rc;
}
