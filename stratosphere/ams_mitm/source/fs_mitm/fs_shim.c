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
#include "fs_shim.h"

/* Missing fsp-srv commands. */
Result fsOpenBisStorageFwd(Service* s, FsStorage* out, FsBisPartitionId partition_id) {
    const u32 tmp = partition_id;
    return serviceDispatchIn(s, 12, tmp,
        .out_num_objects = 1,
        .out_objects = &out->s,
    );
}
