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
#pragma once
#include <vapours.hpp>
#include <stratosphere/htcs/htcs_types.hpp>

namespace ams::htcs {

    bool IsInitialized();

    size_t GetWorkingMemorySize(int max_socket_count);

    void Initialize(AllocateFunction allocate, DeallocateFunction deallocate, int num_sessions = SessionCountMax);
    void Initialize(void *buffer, size_t buffer_size);

    void InitializeForDisableDisconnectionEmulation(AllocateFunction allocate, DeallocateFunction deallocate, int num_sessions = SessionCountMax);
    void InitializeForDisableDisconnectionEmulation(void *buffer, size_t buffer_size);

    void InitializeForSystem(void *buffer, size_t buffer_size, int num_sessions);

    void Finalize();

}
