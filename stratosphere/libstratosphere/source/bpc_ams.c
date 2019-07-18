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
#include <stratosphere/services/bpc_ams.h>

static Service g_bpcAmsSrv;
static u64 g_bpcAmsAmsRefcnt;

Result bpcAmsInitialize(void) {
    atomicIncrement64(&g_bpcAmsAmsRefcnt);

    if (serviceIsActive(&g_bpcAmsSrv)) {
        return 0;
    }

    Handle h;
    Result rc = svcConnectToNamedPort(&h, "bpc:ams");
    if (R_SUCCEEDED(rc)) {
        serviceCreate(&g_bpcAmsSrv, h);
    }

    return rc;
}

void bpcAmsExit(void) {
    if (atomicDecrement64(&g_bpcAmsAmsRefcnt) == 0)
        serviceClose(&g_bpcAmsSrv);
}

Result bpcAmsRebootToFatalError(AtmosphereFatalErrorContext *ctx) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, ctx, sizeof(*ctx), BufferType_Normal);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_bpcAmsSrv, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65000;

    Result rc = serviceIpcDispatch(&g_bpcAmsSrv);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_bpcAmsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}
