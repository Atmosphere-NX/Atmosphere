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

#include <string.h>
#include <switch.h>
#include "setsys_shim.h"

/* Command forwarders. */
Result setsysGetEdidFwd(Service* s, SetSysEdid* out) {
    IpcCommand c;
    ipcInitialize(&c);
    ipcAddRecvStatic(&c, out, sizeof(*out), 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 41;

    Result rc = serviceIpcDispatch(s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;

        serviceIpcParse(s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }

    return rc;
}

Result setsysGetSettingsItemValueFwd(Service *s, const char *name, const char *item_key, void *value_out, size_t value_out_size, u64 *size_out) {
    char send_name[SET_MAX_NAME_SIZE];
    char send_item_key[SET_MAX_NAME_SIZE];
    
    memset(send_name, 0, SET_MAX_NAME_SIZE);
    memset(send_item_key, 0, SET_MAX_NAME_SIZE);
    strncpy(send_name, name, SET_MAX_NAME_SIZE-1);
    strncpy(send_item_key, item_key, SET_MAX_NAME_SIZE-1);

    IpcCommand c;
    ipcInitialize(&c);
    ipcAddSendStatic(&c, send_name, SET_MAX_NAME_SIZE, 0);
    ipcAddSendStatic(&c, send_item_key, SET_MAX_NAME_SIZE, 0);
    ipcAddRecvBuffer(&c, value_out, value_out_size, 0);

    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(s, &c, sizeof(*raw));

    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 38;

    Result rc = serviceIpcDispatch(s);

    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
            u64 size_out;
        } *resp;

        serviceIpcParse(s, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
        if (R_SUCCEEDED(rc)) {
            *size_out = resp->size_out;
        }
    }

    return rc;
}
