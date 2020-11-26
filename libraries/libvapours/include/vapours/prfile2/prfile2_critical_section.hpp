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
#include <vapours/prfile2/pf/prfile2_pf_config.hpp>

namespace ams::prfile2 {

    struct CriticalSection {
        enum State {
            State_NotInitialized = 0,
            State_Initialized    = 1,
        };

        struct Resource;

        u8 state;
        int lock_count;
        Resource *resource;
        u64 owner;
    };

    void InitializeCriticalSection(CriticalSection *cs);
    void FinalizeCriticalSection(CriticalSection *cs);

    void EnterCriticalSection(CriticalSection *cs);
    void ExitCriticalSection(CriticalSection *cs);

    class ScopedCriticalSection {
        private:
            CriticalSection *cs;
        public:
            ALWAYS_INLINE ScopedCriticalSection(CriticalSection *c) : cs(c) { EnterCriticalSection(this->cs); }
            ALWAYS_INLINE ScopedCriticalSection(CriticalSection &c) : ScopedCriticalSection(std::addressof(c)) { /* ... */ }

            ALWAYS_INLINE ~ScopedCriticalSection() { ExitCriticalSection(this->cs); }
    };

}
