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
#include "smm_ams.h"

Result smManagerAtmosphereEndInitialDefers(void) {
    IpcCommand c;
    ipcInitialize(&c);
    Service *srv = smManagerGetServiceSession();

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65000;


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

Result smManagerAtmosphereRegisterProcess(u64 pid, u64 tid, const void *acid_sac, size_t acid_sac_size, const void *aci_sac, size_t aci_sac_size) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendBuffer(&c, acid_sac, acid_sac_size, BufferType_Normal);
    ipcAddSendBuffer(&c, aci_sac,  aci_sac_size,  BufferType_Normal);
    Service *srv = smManagerGetServiceSession();

    struct {
        u64 magic;
        u64 cmd_id;
        u64 pid;
        u64 tid;
    } *raw;

    raw = serviceIpcPrepareHeader(srv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65002;
    raw->pid = pid;
    raw->tid = tid;

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

Result smManagerAtmosphereHasMitm(bool *out, const char* name) {
    IpcCommand c;
    ipcInitialize(&c);
    Service *srv = smManagerGetServiceSession();

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
