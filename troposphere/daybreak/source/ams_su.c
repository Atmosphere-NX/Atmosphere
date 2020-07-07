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
#include <switch.h>
#include <string.h>
#include "ams_su.h"
#include "service_guard.h"

static Service g_amssuSrv;
static TransferMemory g_tmem;

NX_GENERATE_SERVICE_GUARD(amssu);

Result _amssuInitialize(void) {
    return smGetService(&g_amssuSrv, "ams:su");
}

void _amssuCleanup(void) {
    serviceClose(&g_amssuSrv);
    tmemClose(&g_tmem);
}

Service *amssuGetServiceSession(void) {
    return &g_amssuSrv;
}

Result amssuGetUpdateInformation(AmsSuUpdateInformation *out, const char *path) {
    char send_path[FS_MAX_PATH] = {0};
    strncpy(send_path, path, FS_MAX_PATH-1);
    send_path[FS_MAX_PATH-1] = 0;

    return serviceDispatchOut(&g_amssuSrv, 0, *out,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcPointer | SfBufferAttr_FixedSize },
        .buffers      = { { send_path, FS_MAX_PATH } },
    );
}

Result amssuValidateUpdate(AmsSuUpdateValidationInfo *out, const char *path) {
    char send_path[FS_MAX_PATH] = {0};
    strncpy(send_path, path, FS_MAX_PATH-1);
    send_path[FS_MAX_PATH-1] = 0;

    return serviceDispatchOut(&g_amssuSrv, 1, *out,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcPointer | SfBufferAttr_FixedSize },
        .buffers      = { { send_path, FS_MAX_PATH } },
    );
}

Result amssuSetupUpdate(void *buffer, size_t size, const char *path, bool exfat) {
    Result rc = 0;

    if (buffer == NULL) {
        rc = tmemCreate(&g_tmem, size, Perm_None);
    } else {
        rc = tmemCreateFromMemory(&g_tmem, buffer, size, Perm_None);
    }
    if (R_FAILED(rc)) return rc;

    char send_path[FS_MAX_PATH] = {0};
    strncpy(send_path, path, FS_MAX_PATH-1);
    send_path[FS_MAX_PATH-1] = 0;

    const struct {
        u8 exfat;
        u64 size;
    } in = { exfat, g_tmem.size };

    rc = serviceDispatchIn(&g_amssuSrv, 2, in,
        .in_num_handles = 1,
        .in_handles = { g_tmem.handle },
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcPointer | SfBufferAttr_FixedSize },
        .buffers      = { { send_path, FS_MAX_PATH } },
    );
    if (R_FAILED((rc))) {
        tmemClose(&g_tmem);
    }

    return rc;
}

Result amssuSetupUpdateWithVariation(void *buffer, size_t size, const char *path, bool exfat, u32 variation) {
    Result rc = 0;

    if (buffer == NULL) {
        rc = tmemCreate(&g_tmem, size, Perm_None);
    } else {
        rc = tmemCreateFromMemory(&g_tmem, buffer, size, Perm_None);
    }
    if (R_FAILED(rc)) return rc;

    char send_path[FS_MAX_PATH] = {0};
    strncpy(send_path, path, FS_MAX_PATH-1);
    send_path[FS_MAX_PATH-1] = 0;

    const struct {
        u8 exfat;
        u32 variation;
        u64 size;
    } in = { exfat, variation, g_tmem.size };

    rc = serviceDispatchIn(&g_amssuSrv, 3, in,
        .in_num_handles = 1,
        .in_handles = { g_tmem.handle },
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcPointer | SfBufferAttr_FixedSize },
        .buffers      = { { send_path, FS_MAX_PATH } },
    );
    if (R_FAILED((rc))) {
        tmemClose(&g_tmem);
    }

    return rc;
}

Result amssuRequestPrepareUpdate(AsyncResult *a) {
    memset(a, 0, sizeof(*a));

    Handle event = INVALID_HANDLE;
    Result rc = serviceDispatch(&g_amssuSrv, 4,
        .out_num_objects = 1,
        .out_objects = &a->s,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &event,
    );

    if (R_SUCCEEDED(rc))
        eventLoadRemote(&a->event, event, false);

    return rc;
}

Result amssuGetPrepareUpdateProgress(NsSystemUpdateProgress *out) {
    return serviceDispatchOut(&g_amssuSrv, 5, *out);
}

Result amssuHasPreparedUpdate(bool *out) {
    u8 outval = 0;
    Result rc = serviceDispatchOut(&g_amssuSrv, 6, outval);
    if (R_SUCCEEDED(rc)) {
        if (out) *out = outval & 1;
    }
    return rc;
}

Result amssuApplyPreparedUpdate() {
    return serviceDispatch(&g_amssuSrv, 7);
}