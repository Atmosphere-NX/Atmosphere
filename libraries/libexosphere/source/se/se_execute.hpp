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
#include <exosphere.hpp>
#include "se_registers.hpp"

namespace ams::se {

    volatile SecurityEngineRegisters *GetRegisters();
    volatile SecurityEngineRegisters *GetRegisters2();

    void ExecuteOperation(volatile SecurityEngineRegisters *SE, SE_OPERATION_OP op, void *dst, size_t dst_size, const void *src, size_t src_size);
    void ExecuteOperationSingleBlock(volatile SecurityEngineRegisters *SE, void *dst, size_t dst_size, const void *src, size_t src_size);

    void StartInputOperation(volatile SecurityEngineRegisters *SE, const void *src, size_t src_size);
    void StartOperationRaw(volatile SecurityEngineRegisters *SE, SE_OPERATION_OP op, u32 out_ll_address, u32 in_ll_address);
    void SetDoneHandler(volatile SecurityEngineRegisters *SE, DoneHandler handler);

    void ValidateAesOperationResult(volatile SecurityEngineRegisters *SE);

}
