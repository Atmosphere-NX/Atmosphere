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
#include "smm_ams.h"

Result smManagerAtmosphereEndInitialDefers(void) {
    return tipcDispatch(smManagerTipcGetServiceSession(), 65000);
}

Result smManagerAtmosphereRegisterProcess(u64 pid, u64 tid, const CfgOverrideStatus *status, const void *acid_sac, size_t acid_sac_size, const void *aci_sac, size_t aci_sac_size) {
    const struct {
        u64 pid;
        u64 tid;
        CfgOverrideStatus status;
    } in = { pid, tid, *status };
    return tipcDispatchIn(smManagerTipcGetServiceSession(), 65002, in,
        .buffer_attrs = {
            SfBufferAttr_In | SfBufferAttr_HipcMapAlias,
            SfBufferAttr_In | SfBufferAttr_HipcMapAlias,
        },
        .buffers = {
            { acid_sac, acid_sac_size },
            { aci_sac, aci_sac_size },
        },
    );
}

static Result _smManagerAtmosphereCmdHas(bool *out, SmServiceName name, u32 cmd_id) {
    u8 tmp;
    Result rc = tipcDispatchInOut(smManagerTipcGetServiceSession(), cmd_id, name, tmp);
    if (R_SUCCEEDED(rc) && out) *out = tmp & 1;
    return rc;
}

Result smManagerAtmosphereHasMitm(bool *out, SmServiceName name) {
    return _smManagerAtmosphereCmdHas(out, name, 65001);
}
