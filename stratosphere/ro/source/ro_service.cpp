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
 
#include <switch.h>
#include <cstdio>
#include <algorithm>
#include <stratosphere.hpp>

#include "ro_service.hpp"

RelocatableObjectsService::~RelocatableObjectsService() {
    /* TODO */
}

Result RelocatableObjectsService::LoadNro(Out<u64> load_address, PidDescriptor pid_desc, u64 nro_address, u64 nro_size, u64 bss_address, u64 bss_size) {
    /* TODO */
    return ResultKernelConnectionClosed;
}

Result RelocatableObjectsService::UnloadNro(PidDescriptor pid_desc, u64 nro_address) {
    /* TODO */
    return ResultKernelConnectionClosed;
}

Result RelocatableObjectsService::LoadNrr(PidDescriptor pid_desc, u64 nrr_address, u64 nrr_size) {
    /* TODO */
    return ResultKernelConnectionClosed;
}

Result RelocatableObjectsService::UnloadNrr(PidDescriptor pid_desc, u64 nrr_address) {
    /* TODO */
    return ResultKernelConnectionClosed;
}

Result RelocatableObjectsService::Initialize(PidDescriptor pid_desc, CopiedHandle process_h) {
    /* TODO */
    return ResultKernelConnectionClosed;
}

Result RelocatableObjectsService::LoadNrrEx(PidDescriptor pid_desc, u64 nrr_address, u64 nrr_size, CopiedHandle process_h) {
    /* TODO */
    return ResultKernelConnectionClosed;
}