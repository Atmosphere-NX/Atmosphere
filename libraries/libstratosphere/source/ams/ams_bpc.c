/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "ams_bpc.h"

static Service g_amsBpcSrv;

NX_GENERATE_SERVICE_GUARD(amsBpc);

Result _amsBpcInitialize(void) {
    Handle h;
    Result rc = svcConnectToNamedPort(&h, "bpc:ams"); /* TODO: ams:bpc */
    while (R_VALUE(rc) == KERNELRESULT(NotFound)) {
        svcSleepThread(50000000ul);
        rc = svcConnectToNamedPort(&h, "bpc:ams");
    }

    if (R_SUCCEEDED(rc)) serviceCreate(&g_amsBpcSrv, h);
    return rc;
}

void _amsBpcCleanup(void) {
    serviceClose(&g_amsBpcSrv);
}

Service *amsBpcGetServiceSession(void) {
    return &g_amsBpcSrv;
}

Result amsBpcRebootToFatalError(void *ctx) {
    /* Note: this takes in an sts::ams::FatalErrorContext. */
    /* static_assert(sizeof() == 0x450) is done at type definition. */
    return serviceDispatch(&g_amsBpcSrv, 65000,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias | SfBufferAttr_FixedSize },
        .buffers = { { ctx, 0x450 } },
    );
}


Result amsBpcSetRebootPayload(const void *src, size_t src_size) {
    return serviceDispatch(&g_amsBpcSrv, 65001,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { src, src_size } },
    );
}
