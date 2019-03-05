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
 
#pragma once
struct SmServiceName {
    char name[sizeof(u64)];
};

static_assert(__alignof__(SmServiceName) == 1, "SmServiceName definition!");

/* For Debug Monitor extensions. */
struct SmServiceRecord {
    u64 service_name;
    u64 owner_pid;
    u64 max_sessions;
    u64 mitm_pid;
    u64 mitm_waiting_ack_pid;
    bool is_light;
    bool mitm_waiting_ack;
};

static_assert(sizeof(SmServiceRecord) == 0x30, "SmServiceRecord definition!");