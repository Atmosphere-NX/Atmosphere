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
 
#include <switch.h>
#include <switch/arm/atomics.h>
#include <stratosphere/services/smm_ams.h>

static Service g_smManagerAmsSrv;
static u64 g_smManagerAmsRefcnt;

Result smManagerAmsInitialize(void) {
    atomicIncrement64(&g_smManagerAmsRefcnt);

    if (serviceIsActive(&g_smManagerAmsSrv))
        return 0;

    return smGetService(&g_smManagerAmsSrv, "sm:m");
}

void smManagerAmsExit(void) {
    if (atomicDecrement64(&g_smManagerAmsRefcnt) == 0)
        serviceClose(&g_smManagerAmsSrv);    
}

Result smManagerAmsEndInitialDefers(void) {
    IpcCommand c;
    ipcInitialize(&c);
    
    struct {
        u64 magic;
        u64 cmd_id;
    } *raw;

    raw = serviceIpcPrepareHeader(&g_smManagerAmsSrv, &c, sizeof(*raw));
    raw->magic = SFCI_MAGIC;
    raw->cmd_id = 65000;

    
    Result rc = serviceIpcDispatch(&g_smManagerAmsSrv);
    
    if (R_SUCCEEDED(rc)) {
        IpcParsedCommand r;
        struct {
            u64 magic;
            u64 result;
        } *resp;
        
        serviceIpcParse(&g_smManagerAmsSrv, &r, sizeof(*resp));
        resp = r.Raw;

        rc = resp->result;
    }
    
    return rc;

}