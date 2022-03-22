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

namespace ams::flow {

    void SetRegisterAddress(uintptr_t address);

    void ResetCpuRegisters(int core);

    void SetCpuCsr(int core, u32 enable_ext);
    void SetHaltCpuEvents(int core, bool resume_on_irq);
    void SetCc4Ctrl(int core, u32 value);
    void ClearL2FlushControl();

}
