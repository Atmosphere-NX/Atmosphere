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
#include "hid_shim.h"
#include <stratosphere/sf/sf_mitm_dispatch.h>

/* Command forwarders. */
Result hidSetSupportedNpadStyleSetFwd(Service* s, u64 process_id, u64 aruid, u32 style_set) {
    const struct {
        u32 style_set;
        u32 pad;
        u64 aruid;
    } in = { style_set, 0, aruid };
    return serviceMitmDispatchIn(s, 100, in, .in_send_pid = true, .override_pid = process_id);
}
