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

#include "hvisor_watchpoint_manager.hpp"
#include "cpu/hvisor_cpu_instructions.hpp"
#include <mutex>

#define _REENT_ONLY
#include <cerrno>

namespace {

    constexpr bool IsRangeMaskWatchpoint(uintptr_t addr, size_t size)
    {
        // size needs to be a power of 2, at least 8 (we'll only allow 16+ though), addr needs to be aligned.
        bool ret = (size & (size - 1)) == 0 && size >= 16 && (addr & (size - 1)) == 0;
        return ret;
    }

    constexpr bool CheckWatchpointAddressAndSizeParams(uintptr_t addr, size_t size)
    {
        if (size == 0) {
            return false;
        } else if (size > 8) {
            return IsRangeMaskWatchpoint(addr, size);
        } else {
            return ((addr + size) & ~7ul) == (addr & ~7ul);
        }
    }

}

namespace ams::hvisor {

    WatchpointManager WatchpointManager::instance{};

    void WatchpointManager::Reload() const
    {
        // TODO
    }

    bool WatchpointManager::FindPredicate(const cpu::DebugRegisterPair &pair, uintptr_t addr, size_t size, cpu::DebugRegisterPair::LoadStoreControl direction) const
    {
        size_t off;
        size_t sz;
        size_t nmask;
    
        if (pair.cr.mask != 0) {
            off = 0;
            sz = MASK(pair.cr.mask);
            nmask = ~sz;
        } else {
            off = __builtin_ffs(pair.cr.bas) - 1;
            sz = __builtin_popcount(pair.cr.bas);
            nmask = ~7ul;
        }

        if (size != 0) {
            // Strict watchpoint check
            if (addr == pair.vr + off && direction == pair.cr.lsc && sz == size) {
                return true;
            }
        } else {
            // Return first wp that could have triggered the exception
            if ((addr & nmask) == pair.vr && (direction & pair.cr.lsc) != 0) {
                return true;
            }
        }

        return false;
    }

    cpu::DebugRegisterPair WatchpointManager::RetrieveWatchpointConfig(uintptr_t addr, cpu::DebugRegisterPair::LoadStoreControl direction) const
    {
        std::scoped_lock lk{m_lock};
        const cpu::DebugRegisterPair *p = Find(addr, 0, direction);
        return p != nullptr ? *p : cpu::DebugRegisterPair{};
    }

    int WatchpointManager::Add(uintptr_t addr, size_t size, cpu::DebugRegisterPair::LoadStoreControl direction)
    {
        if (!CheckWatchpointAddressAndSizeParams(addr, size)) {
            return -EINVAL;
        }

        cpu::DebugRegisterPair wp{};
        wp.cr.lsc = direction;
        if (IsRangeMaskWatchpoint(addr, size)) {
            wp.vr = addr;
            wp.cr.bas = 0xFF; // TRM-mandated
            wp.cr.mask = static_cast<u32>(__builtin_ffsl(size) - 1);
        } else {
            size_t off = addr & 7ull;
            wp.vr = addr & ~7ul;
            wp.cr.bas = MASK2(off + size, off);
        }

        return AddImpl(addr, size, wp);
    }

    int WatchpointManager::Remove(uintptr_t addr, size_t size, cpu::DebugRegisterPair::LoadStoreControl direction)
    {
        if (!CheckWatchpointAddressAndSizeParams(addr, size)) {
            return -EINVAL;
        }

        return RemoveImpl(addr, size, direction);
    }
}
