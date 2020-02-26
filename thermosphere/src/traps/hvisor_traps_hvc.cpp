/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "hvisor_traps_hvc.hpp"
#include "../hvisor_exception_dispatcher.hpp"

namespace ams::hvisor::traps {

    void HandleHvc(ExceptionStackFrame *frame, cpu::ExceptionSyndromeRegister esr)
    {
        u32 id = esr.iss;
        switch (id) {
            default:
                DEBUG("Unhandled hypercall: 0x%x.\n");
                DumpStackFrame(frame, false);
                break;
        }
    }

}
