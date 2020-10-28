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
#pragma once
#include <vapours/sdmmc/sdmmc_build_config.hpp>

namespace ams::sdmmc {

    constexpr inline size_t GcAsicOperationSize = 0x40;

    void PutGcAsicToSleep(Port port);
    Result AwakenGcAsic(Port port);
    Result WriteGcAsicOperation(Port port, const void *op_buf, size_t op_buf_size);
    Result FinishGcAsicOperation(Port port);
    Result AbortGcAsicOperation(Port port);
    Result SleepGcAsic(Port port);
    Result UpdateGcAsicKey(Port port);

    void SignalGcRemovedEvent(Port port);
    void ClearGcRemovedEvent(Port port);

}
