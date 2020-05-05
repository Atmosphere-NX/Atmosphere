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

namespace ams::util {

    namespace {

        constinit uintptr_t g_timer_register_address = secmon::MemoryRegionPhysicalDeviceTimer.GetAddress();

        ALWAYS_INLINE uintptr_t GetCurrentTimeRegisterAddress() {
            return g_timer_register_address + 0x10;
        }

    }

    void SetRegisterAddress(uintptr_t address) {
        g_timer_register_address = address;
    }

    u32 GetMicroSeconds() {
        return reg::Read(GetCurrentTimeRegisterAddress());
    }

    void WaitMicroSeconds(int us) {
        const u32 start = reg::Read(GetCurrentTimeRegisterAddress());
        u32 cur = start;
        while ((cur - start) <= static_cast<u32>(us)) {
            cur = reg::Read(GetCurrentTimeRegisterAddress());
        }
    }

    void ClearMemory(void *ptr, size_t size) {
        std::memset(ptr, 0, size);
    }

}