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

#include "hvisor_synchronization.hpp"
#include "core_ctx.h"

namespace ams::hvisor {

    void Spinlock::lock()
    {
        u32 tmp1;
        const u32 tmp2 = 1;
        __asm__ __volatile__(
            "prfm   pstl1keep, %[val]                   \n"
            "sevl                                       \n"
            "1:                                         \n"
            "   wfe                                     \n"
            "   2:                                      \n"
            "       ldaxr   %[tmp1], %[val]             \n"
            "       cbnz    %[tmp1], 1b                 \n"
            "       stxr    %[tmp1], %[tmp2], %[val]    \n"
            "       cbnz    %[tmp1], 2b                 \n"
            : [tmp1] "=&r"(tmp1), [val] "+Q" (m_val)
            : [tmp2] "r"(tmp2)
            : "cc", "memory"
        );
    }

    void Spinlock::unlock() noexcept
    {
        __asm__ __volatile__("stlr wzr, %[val]" : [val] "=Q" (m_val) :: "memory");
    }

    void Barrier::Join()
    {
        const u32 mask = BIT(currentCoreCtx->coreId);
        u32 newval, tmp;
        __asm__ __volatile__(
            "prfm   pstl1keep, %[val]                   \n"
            
            /* Fetch-and */
            "1:                                         \n"
            "   ldaxr   %[newval], %[val]               \n"
            "   bic     %[newval], %[newval], %[mask]   \n"
            "   stlxr   %[tmp], %[newval], %[val]       \n"
            "   cbnz    %[tmp], 1b                      \n"

            /* Check if now/already 0, wait if not */
            "cbz    %[newval], 3f                       \n"
            
            /* Event will be signaled if the stlxr succeeds for another core... */
            "2:                                         \n"
            "   wfe                                     \n"
            "   ldaxr   %[newval], %[val]               \n"
            "   cbnz    %[newval], 2b                   \n"
            "3:                                         \n"
            : [newval] "=&r"(newval), [tmp] "=&r" (tmp), [val] "+Q" (m_val)
            : [mask] "r"(mask)
            : "cc", "memory"
        );
    }

    void RecursiveSpinlock::lock()
    {
        u32 tag = currentCoreCtx->coreId + 1;
        if (AMS_LIKELY(tag != m_tag)) {
            m_spinlock.lock();
            m_tag = tag;
            m_count = 1;
        } else {
            ++m_count;
        }
    }

    void RecursiveSpinlock::unlock() noexcept
    {
        if (AMS_LIKELY(--m_count == 0)) {
            m_tag = 0;
            m_spinlock.unlock();
        }
    }
}
