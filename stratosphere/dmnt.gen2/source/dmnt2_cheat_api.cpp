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
#include <stratosphere.hpp>
#include "dmnt2_cheat_api.hpp"
// #include "dmnt_cheat_vm.hpp"
// #include "dmnt_cheat_debug_events_manager.hpp"

namespace ams::dmnt::cheat::impl {

    // namespace {

    //     /* Helper definitions. */
    //     constexpr size_t MaxCheatCount = 0x80;
    //     constexpr size_t MaxFrozenAddressCount = 0x80;

    //     class CheatProcessManager {
    //     private:

    //     };
    // }
    Result BreakPoint(u64 proc_addr, void* out_data, size_t size) {
        // std::scoped_lock lk(m_cheat_lock);
        typedef struct {
            u64 address;
            char name[200];
        } DmntBreakpointResult;
#define BP_Result (*(DmntBreakpointResult*) out_data)
#define BP_address proc_addr & 0xFFFFFFFFFF
#define BP_Token proc_addr >> 56  

        if (BP_Token != 0) {
            BP_Result.address = BP_address;
            sprintf(BP_Result.name, "Good Work Token=%lx Size=%ld", BP_Token, size);
            R_SUCCEED();
        }
        // else {

        //     R_TRY(this->EnsureCheatProcess());

        //     R_RETURN(this->BreakPointUnsafe(proc_addr, out_data, size));
        // };
        return 0;
    };


}
