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
#include "dmntcht.h"

static Service g_dmntchtSrv;

NX_GENERATE_SERVICE_GUARD(dmntcht);

Result _dmntchtInitialize(void) {
    return smGetService(&g_dmntchtSrv, "dmnt:cht");
}

void _dmntchtCleanup(void) {
    serviceClose(&g_dmntchtSrv);
}

Service* dmntchtGetServiceSession(void) {
    return &g_dmntchtSrv;
}

Result dmntchtHasCheatProcess(bool *out) {
    u8 tmp;
    Result rc = serviceDispatchOut(&g_dmntchtSrv, 65000, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result dmntchtGetCheatProcessEvent(Event *event) {
    Handle evt_handle;
    Result rc = serviceDispatch(&g_dmntchtSrv, 65001,
        .out_handle_attrs = { SfOutHandleAttr_HipcCopy },
        .out_handles = &evt_handle,
    );

    if (R_SUCCEEDED(rc)) {
        eventLoadRemote(event, evt_handle, true);
    }

    return rc;
}

Result dmntchtGetCheatProcessMetadata(DmntCheatProcessMetadata *out_metadata) {
    return serviceDispatchOut(&g_dmntchtSrv, 65002, *out_metadata);
}

static Result _dmntchtCmdVoid(Service* srv, u32 cmd_id) {
    return serviceDispatch(srv, cmd_id);
}

Result dmntchtForceOpenCheatProcess(void) {
    return _dmntchtCmdVoid(&g_dmntchtSrv, 65003);
}

Result dmntchtPauseCheatProcess(void) {
    return _dmntchtCmdVoid(&g_dmntchtSrv, 65004);
}

Result dmntchtResumeCheatProcess(void) {
    return _dmntchtCmdVoid(&g_dmntchtSrv, 65005);
}

static Result _dmntchtGetCount(u64 *out_count, u32 cmd_id) {
    return serviceDispatchOut(&g_dmntchtSrv, cmd_id, *out_count);
}

static Result _dmntchtGetEntries(void *buffer, u64 buffer_size, u64 offset, u64 *out_count, u32 cmd_id) {
    return serviceDispatchInOut(&g_dmntchtSrv, cmd_id, offset, *out_count,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcMapAlias },
        .buffers = { { buffer, buffer_size } },
    );
}

static Result _dmntchtCmdInU32NoOut(u32 in, u32 cmd_id) {
    return serviceDispatchIn(&g_dmntchtSrv, cmd_id, in);
}

Result dmntchtGetCheatProcessMappingCount(u64 *out_count) {
    return _dmntchtGetCount(out_count, 65100);
}

Result dmntchtGetCheatProcessMappings(MemoryInfo *buffer, u64 max_count, u64 offset, u64 *out_count) {
    return _dmntchtGetEntries(buffer, sizeof(*buffer) * max_count, offset, out_count, 65101);
}

Result dmntchtReadCheatProcessMemory(u64 address, void *buffer, size_t size) {
    const struct {
        u64 address;
        u64 size;
    } in = { address, size };
    return serviceDispatchIn(&g_dmntchtSrv, 65102, in,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcMapAlias },
        .buffers = { { buffer, size } },
    );
}

Result dmntchtWriteCheatProcessMemory(u64 address, const void *buffer, size_t size) {
    const struct {
        u64 address;
        u64 size;
    } in = { address, size };
    return serviceDispatchIn(&g_dmntchtSrv, 65103, in,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias },
        .buffers = { { buffer, size } },
    );
}

Result dmntchtQueryCheatProcessMemory(MemoryInfo *mem_info, u64 address){
    return serviceDispatchInOut(&g_dmntchtSrv, 65104, address, *mem_info);
}

Result dmntchtGetCheatCount(u64 *out_count) {
    return _dmntchtGetCount(out_count, 65200);
}

Result dmntchtGetCheats(DmntCheatEntry *buffer, u64 max_count, u64 offset, u64 *out_count) {
    return _dmntchtGetEntries(buffer, sizeof(*buffer) * max_count, offset, out_count, 65201);
}

Result dmntchtGetCheatById(DmntCheatEntry *out, u32 cheat_id) {
    return serviceDispatchIn(&g_dmntchtSrv, 65202, cheat_id,
        .buffer_attrs = { SfBufferAttr_Out | SfBufferAttr_HipcMapAlias | SfBufferAttr_FixedSize },
        .buffers = { { out, sizeof(*out) } },
    );
}

Result dmntchtToggleCheat(u32 cheat_id) {
    return _dmntchtCmdInU32NoOut(cheat_id, 65203);
}

Result dmntchtAddCheat(DmntCheatDefinition *cheat_def, bool enabled, u32 *out_cheat_id) {
    const u8 in = enabled != 0;
    return serviceDispatchInOut(&g_dmntchtSrv, 65204, in, *out_cheat_id,
        .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcMapAlias | SfBufferAttr_FixedSize },
        .buffers = { { cheat_def, sizeof(*cheat_def) } },
    );
}

Result dmntchtRemoveCheat(u32 cheat_id) {
    return _dmntchtCmdInU32NoOut(cheat_id, 65205);
}

Result dmntchtReadStaticRegister(u64 *out, u8 which) {
    return serviceDispatchInOut(&g_dmntchtSrv, 65206, which, *out);
}

Result dmntchtWriteStaticRegister(u8 which, u64 value) {
    const struct {
        u64 which;
        u64 value;
    } in = { which, value };

    return serviceDispatchIn(&g_dmntchtSrv, 65207, in);
}

Result dmntchtResetStaticRegisters() {
    return _dmntchtCmdVoid(&g_dmntchtSrv, 65208);
}

Result dmntchtGetFrozenAddressCount(u64 *out_count) {
    return _dmntchtGetCount(out_count, 65300);
}

Result dmntchtGetFrozenAddresses(DmntFrozenAddressEntry *buffer, u64 max_count, u64 offset, u64 *out_count) {
    return _dmntchtGetEntries(buffer, sizeof(*buffer) * max_count, offset, out_count, 65301);
}

Result dmntchtGetFrozenAddress(DmntFrozenAddressEntry *out, u64 address) {
    return serviceDispatchInOut(&g_dmntchtSrv, 65302, address, *out);
}

Result dmntchtEnableFrozenAddress(u64 address, u64 width, u64 *out_value) {
    const struct {
        u64 address;
        u64 width;
    } in = { address, width };
    return serviceDispatchInOut(&g_dmntchtSrv, 65303, in, *out_value);
}

Result dmntchtDisableFrozenAddress(u64 address) {
    return serviceDispatchIn(&g_dmntchtSrv, 65304, address);
}
