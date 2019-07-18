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
#include <stratosphere/services/dmntcht.h>

static Service g_dmntchtService;
static u64 g_refCnt;

static Result _dmntchtGetCount(u64 cmd_id, u64 *out_count);
static Result _dmntchtGetEntries(u64 cmd_id, void *buffer, u64 buffer_size, u64 offset, u64 *out_count);

Result dmntchtInitialize(void) {
    atomicIncrement64(&g_refCnt);

    if (serviceIsActive(&g_dmntchtService)) {
        return 0;
    }

    return smGetService(&g_dmntchtService, "dmnt:cht");
}

void dmntchtExit(void) {
    if (atomicIncrement64(&g_refCnt) == 0)  {
        serviceClose(&g_dmntchtService);
    }
}

Service* dmntchtGetServiceSession(void) {
    return &g_dmntchtService;
}

Result dmntchtHasCheatProcess(bool *out) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65000;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            bool out;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            if (out) *out = resp->out;
        }
    }

    return rc;
}

Result dmntchtGetCheatProcessEvent(Event *event) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65001;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;

        if (R_SUCCEEDED(rc)) {
            eventLoadRemote(event, r.Handles[0], true);
        }
    }

    return rc;
}

Result dmntchtGetCheatProcessMetadata(DmntCheatProcessMetadata *out_metadata) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65002;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            DmntCheatProcessMetadata metadata;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            if (out_metadata) *out_metadata = resp->metadata;
        }
    }

    return rc;
}

Result dmntchtForceOpenCheatProcess(void) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65003;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

static Result _dmntchtGetCount(u64 cmd_id, u64 *out_count) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 count;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        *out_count = resp->count;
    }

    return rc;
}

static Result _dmntchtGetEntries(u64 cmd_id, void *buffer, u64 buffer_size, u64 offset, u64 *out_count) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buffer, buffer_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 offset;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = cmd_id;
    raw->offset = offset;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 count;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            if (out_count) *out_count = resp->count;
        }
    }

    return rc;
}

Result dmntchtGetCheatProcessMappingCount(u64 *out_count) {
    return _dmntchtGetCount(65100, out_count);
}

Result dmntchtGetCheatProcessMappings(MemoryInfo *buffer, u64 max_count, u64 offset, u64 *out_count) {
    return _dmntchtGetEntries(65101, buffer, sizeof(*buffer) * max_count, offset, out_count);
}

Result dmntchtReadCheatProcessMemory(u64 address, void *buffer, size_t size) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buffer, size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 address;
        u64 size;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65102;
    raw->address = address;
    raw->size = size;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result dmntchtWriteCheatProcessMemory(u64 address, const void *buffer, size_t size) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, buffer, size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 address;
        u64 size;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65103;
    raw->address = address;
    raw->size = size;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result dmntchtQueryCheatProcessMemory(MemoryInfo *mem_info, u64 address){
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 address;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65104;
    raw->address = address;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            MemoryInfo mem_info;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            if (mem_info) *mem_info = resp->mem_info;
        }
    }

    return rc;
}

Result dmntchtGetCheatCount(u64 *out_count) {
    return _dmntchtGetCount(65200, out_count);
}

Result dmntchtGetCheats(DmntCheatEntry *buffer, u64 max_count, u64 offset, u64 *out_count) {
    return _dmntchtGetEntries(65201, buffer, sizeof(*buffer) * max_count, offset, out_count);
}

Result dmntchtGetCheatById(DmntCheatEntry *buffer, u32 cheat_id) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvBuffer(&c, buffer, sizeof(*buffer), 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 cheat_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65202;
    raw->cheat_id = cheat_id;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result dmntchtToggleCheat(u32 cheat_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 cheat_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65203;
    raw->cheat_id = cheat_id;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result dmntchtAddCheat(DmntCheatDefinition *buffer, bool enabled, u32 *out_cheat_id) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, buffer, sizeof(*buffer), 0);

    struct {
        u64 magic;
        u64 cmd_id;
        u8 enabled;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65204;
    raw->enabled = enabled;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u32 cheat_id;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            if (out_cheat_id) *out_cheat_id = resp->cheat_id;
        }
    }

    return rc;
}

Result dmntchtRemoveCheat(u32 cheat_id) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u32 cheat_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65205;
    raw->cheat_id = cheat_id;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}


Result dmntchtGetFrozenAddressCount(u64 *out_count) {
    return _dmntchtGetCount(65300, out_count);
}

Result dmntchtGetFrozenAddresses(DmntFrozenAddressEntry *buffer, u64 max_count, u64 offset, u64 *out_count) {
    return _dmntchtGetEntries(65301, buffer, sizeof(*buffer) * max_count, offset, out_count);
}

Result dmntchtGetFrozenAddress(DmntFrozenAddressEntry *out, u64 address) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 address;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65302;
    raw->address = address;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            DmntFrozenAddressEntry entry;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            if (out) *out = resp->entry;
        }
    }

    return rc;
}

Result dmntchtEnableFrozenAddress(u64 address, u64 width, u64 *out_value) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 address;
        u64 width;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65303;
    raw->address = address;
    raw->width = width;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 value;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            if (out_value) *out_value = resp->value;
        }
    }

    return rc;
}

Result dmntchtDisableFrozenAddress(u64 address) {
    IpcCommand c;
    ipcInitialize(&c);

    struct {
        u64 magic;
        u64 cmd_id;
        u64 address;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_dmntchtService, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65304;
    raw->address = address;

    Result rc = serviceIpcDispatch(&g_dmntchtService);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 value;
        } *resp;

        serviceIpcParse(&g_dmntchtService, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}
