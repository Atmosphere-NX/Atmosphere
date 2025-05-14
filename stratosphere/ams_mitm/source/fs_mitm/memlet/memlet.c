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
#include "service_guard.h"
#include "memlet.h"

static Service g_memletSrv;

NX_GENERATE_SERVICE_GUARD(memlet);

Result _memletInitialize(void) {
    return smGetService(&g_memletSrv, "memlet");
}

void _memletCleanup(void) {
    serviceClose(&g_memletSrv);
}

Service* memletGetServiceSession(void) {
    return &g_memletSrv;
}

Result memletCreateAppletSharedMemory(Handle *out_shmem_h, u64 *out_size, u64 desired_size) {
    return serviceDispatchInOut(&g_memletSrv, 65000, desired_size, *out_size,
        .out_handle_attrs = { SfOutHandleAttr_HipcMove },
        .out_handles = out_shmem_h,
    );
}
