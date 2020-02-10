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

#pragma once

#include "defines.hpp"


namespace ams::hvisor {

    class Spinlock final {
        NON_COPYABLE(Spinlock);
        NON_MOVEABLE(Spinlock);
        private:
            u32 m_val = 0;
        public:
            constexpr Spinlock() = default;
            void lock();
            void unlock() noexcept;
    };

    class Barrier final {
        NON_COPYABLE(Barrier);
        NON_MOVEABLE(Barrier);
        private:
            u32 m_val = 0;
        public:
            constexpr Barrier() = default;
            void Join();

            constexpr void Reset(u32 val)
            {
                m_val = val;
            }
    };

    class RecursiveSpinlock final {
        NON_COPYABLE(RecursiveSpinlock);
        NON_MOVEABLE(RecursiveSpinlock);
        private:
            Spinlock m_spinlock{};
            u32 m_tag = 0;
            u32 m_count = 0;
        public:
            constexpr RecursiveSpinlock() = default;
            void lock();
            void unlock() noexcept;
    };

}
