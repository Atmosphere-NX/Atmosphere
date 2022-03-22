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
#include "sfdnsres_shim.h"
#include <stratosphere/sf/sf_mitm_dispatch.h>

/* Command forwarders. */
Result sfdnsresGetHostByNameRequestWithOptionsFwd(Service *s, u64 process_id, const void *name, size_t name_size, void *out_hostent, size_t out_hostent_size, u32 *out_size, u32 options_version, const void *option, size_t option_size, u32 num_options, s32 *out_host_error, s32 *out_errno) {
    const struct {
        u32 options_version;
        u32 num_options;
        u64 process_id;
    } in = { options_version, num_options, process_id };
    struct {
        u32 size;
        s32 host_error;
        s32 errno;
    } out;

    Result rc = serviceMitmDispatchInOut(s, 10, in, out,
        .buffer_attrs = {
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In
        },
        .buffers = {
            { name, name_size },
            { out_hostent, out_hostent_size },
            { option, option_size }
        },
        .in_send_pid = true,
        .override_pid = process_id,
    );

    if (R_SUCCEEDED(rc)) {
        if (out_size) *out_size = out.size;
        if (out_host_error) *out_host_error = out.host_error;
        if (out_errno) *out_errno = out.errno;
    }

    return rc;
}

Result sfdnsresGetAddrInfoRequestWithOptionsFwd(Service *s, u64 process_id, const void *node, size_t node_size, const void *srv, size_t srv_size, const void *hint, size_t hint_size, void *out_ai, size_t out_ai_size, u32 *out_size, s32 *out_rv, u32 options_version, const void *option, size_t option_size, u32 num_options, s32 *out_host_error, s32 *out_errno) {
    const struct {
        u32 options_version;
        u32 num_options;
        u64 process_id;
    } in = { options_version, num_options, process_id };
    struct {
        u32 size;
        s32 rv;
        s32 host_error;
        s32 errno;
    } out;

    Result rc = serviceMitmDispatchInOut(s, 12, in, out,
        .buffer_attrs = {
            SfBufferAttr_HipcMapAlias   | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias   | SfBufferAttr_In,
            SfBufferAttr_HipcMapAlias   | SfBufferAttr_In,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_Out,
            SfBufferAttr_HipcAutoSelect | SfBufferAttr_In
        },
        .buffers = {
            { node, node_size },
            { srv, srv_size },
            { hint, hint_size },
            { out_ai, out_ai_size },
            { option, option_size }
        },
        .in_send_pid = true,
        .override_pid = process_id,
    );

    if (R_SUCCEEDED(rc)) {
        if (out_size) *out_size = out.size;
        if (out_rv) *out_rv = out.rv;
        if (out_host_error) *out_host_error = out.host_error;
        if (out_errno) *out_errno = out.errno;
    }

    return rc;
}

