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
 
#include "tma_conn_result.hpp"

/* This is just python's hash function, but official TMA code uses it. */
static constexpr u32 HashServiceName(const char *name) {
    u32 h = *name;
    u32 len = 0;
    
    while (*name) {
        h = (1000003 * h) ^ *name;
        name++;
        len++;
    }
    
    return h ^ len;
}

enum class TmaService : u32 {
    Invalid = 0,
    
    /* Special nodes, for facilitating connection over USB. */
    UsbQueryTarget = HashServiceName("USBQueryTarget"),
    UsbSendHostInfo = HashServiceName("USBSendHostInfo"),
    UsbConnect = HashServiceName("USBConnect"),
    UsbDisconnect = HashServiceName("USBDisconnect"),
    
    
    TestService = HashServiceName("AtmosphereTestService"), /* Temporary service, will be used to debug communications. */
};
