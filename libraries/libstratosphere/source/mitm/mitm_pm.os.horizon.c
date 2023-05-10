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
#include "mitm_pm.os.horizon.h"

static Service g_amsMitmPmSrv;

NX_GENERATE_SERVICE_GUARD(amsMitmPm);

Result _amsMitmPmInitialize(void) {
    return smGetService(&g_amsMitmPmSrv, "mitm:pm");
}

void _amsMitmPmCleanup(void) {
    serviceClose(&g_amsMitmPmSrv);
}

Service *amsMitmPmGetServiceSession(void) {
    return &g_amsMitmPmSrv;
}

Result amsMitmPmPrepareLaunchProgram(u64 *out, u64 program_id, const CfgOverrideStatus *status, bool is_application) {
    const struct {
        u8 is_application;
        u64 program_id;
        CfgOverrideStatus status;
    } in = { is_application ? 1 : 0, program_id, *status };

    return serviceDispatchInOut(&g_amsMitmPmSrv, 65000, in, *out);
}
