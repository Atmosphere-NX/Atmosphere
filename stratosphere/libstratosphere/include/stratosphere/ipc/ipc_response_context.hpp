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

#pragma once
#include <switch.h>

#include "ipc_service_object.hpp"
#include "ipc_domain_object.hpp"

#include "ipc_special.hpp"

#include "ipc_session_manager_base.hpp"

struct IpcResponseContext {
    /* Request/Reply data. */
    IpcParsedCommand request;
    IpcCommand reply;
    u8 out_data[0x100];
    std::shared_ptr<IServiceObject> *out_objs[8];
    Handle out_object_server_handles[8];
    IpcHandle out_handles[8];
    u32 out_object_ids[8];
    IpcCommandType cmd_type;
    u64 cmd_id;
    Result rc;
    /* Context. */
    SessionManagerBase *manager;
    ServiceObjectHolder *obj_holder;
    unsigned char *pb;
    size_t pb_size;
};