/*
 * Copyright (c) Atmosphère-NX
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

static Result _smAtmosphereCmdHas(bool *out, SmServiceName name, u32 cmd_id) {
    u8 tmp;
    Result rc = tipcDispatchInOut(smGetServiceSessionTipc(), cmd_id, name, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

static Result _smAtmosphereCmdInServiceNameNoOut(SmServiceName name, TipcService *srv, u32 cmd_id) {
    return tipcDispatchIn(srv, cmd_id, name);
}

static Result _smAtmosphereDetachClient(TipcService *srv) {
    return tipcDispatch(srv, 4, .in_send_pid = true);
}

Result smAtmosphereHasService(bool *out, SmServiceName name) {
    return _smAtmosphereCmdHas(out, name, 65100);
}

Result smAtmosphereWaitService(SmServiceName name) {
    return _smAtmosphereCmdInServiceNameNoOut(name, smGetServiceSessionTipc(), 65101);
}

Result smAtmosphereHasMitm(bool *out, SmServiceName name) {
    return _smAtmosphereCmdHas(out, name, 65004);
}

Result smAtmosphereWaitMitm(SmServiceName name) {
    return _smAtmosphereCmdInServiceNameNoOut(name, smGetServiceSessionTipc(), 65005);
}

static TipcService g_smAtmosphereMitmSrv;

NX_GENERATE_SERVICE_GUARD(smAtmosphereMitm);

Result _smAtmosphereMitmInitialize(void) {
    return smAtmosphereOpenSession(&g_smAtmosphereMitmSrv);
}

void _smAtmosphereMitmCleanup(void) {
    smAtmosphereCloseSession(&g_smAtmosphereMitmSrv);
}

TipcService* smAtmosphereMitmGetServiceSession(void) {
    return &g_smAtmosphereMitmSrv;
}

Result smAtmosphereOpenSession(TipcService *out) {
    Handle sm_handle;
    Result rc = svcConnectToNamedPort(&sm_handle, "sm:");
    while (R_VALUE(rc) == KERNELRESULT(NotFound)) {
        svcSleepThread(50000000ul);
        rc = svcConnectToNamedPort(&sm_handle, "sm:");
    }

    if (R_SUCCEEDED(rc)) {
        tipcCreate(out, sm_handle);
    }

    if (R_SUCCEEDED(rc)) {
        rc = tipcDispatch(out, 0, .in_send_pid = true);
    }

    return rc;
}

void smAtmosphereCloseSession(TipcService *srv) {
    Result rc = _smAtmosphereDetachClient(srv);
    if (R_FAILED(rc)) {
        svcBreak(BreakReason_Panic, (uintptr_t)&rc, sizeof(rc));
    }
    tipcClose(srv);
}

Result smAtmosphereMitmInstall(TipcService *fwd_srv, Handle *handle_out, Handle *query_out, SmServiceName name) {
    Handle tmp_handles[2];
    Result rc = tipcDispatchIn(fwd_srv, 65000, name,
        .out_handle_attrs = { SfOutHandleAttr_HipcMove, SfOutHandleAttr_HipcMove },
        .out_handles = tmp_handles,
    );

    if (R_SUCCEEDED(rc)) {
        *handle_out = tmp_handles[0];
        *query_out  = tmp_handles[1];
    }

    return rc;
}

Result smAtmosphereMitmUninstall(SmServiceName name) {
    return _smAtmosphereCmdInServiceNameNoOut(name, smGetServiceSessionTipc(), 65001);
}

Result smAtmosphereMitmDeclareFuture(SmServiceName name) {
    return _smAtmosphereCmdInServiceNameNoOut(name, smGetServiceSessionTipc(), 65006);
}

Result smAtmosphereMitmClearFuture(SmServiceName name) {
    return _smAtmosphereCmdInServiceNameNoOut(name, smGetServiceSessionTipc(), 65007);
}

Result smAtmosphereMitmAcknowledgeSession(Service *srv_out, void *_out, SmServiceName name) {
    struct {
        u64 process_id;
        u64 program_id;
        u64 keys_held;
        u64 flags;
    } *out = _out;
    _Static_assert(sizeof(*out) == 0x20, "sizeof(*out) == 0x20");

    Handle tmp_handle;

    Result rc = tipcDispatchInOut(&g_smAtmosphereMitmSrv, 65003, name, *out,
        .out_handle_attrs = { SfOutHandleAttr_HipcMove },
        .out_handles = &tmp_handle,
    );

    if (R_SUCCEEDED(rc)) {
        serviceCreate(srv_out, tmp_handle);
    }

    return rc;
}
