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
#include <stratosphere/os/os_barrier_types.hpp>
#include <stratosphere/os/os_barrier_api.hpp>

namespace ams::os {

    class Barrier {
        NON_COPYABLE(Barrier);
        NON_MOVEABLE(Barrier);
        private:
            BarrierType m_barrier;
        public:
            explicit Barrier(int num_threads) {
                InitializeBarrier(std::addressof(m_barrier), num_threads);
            }

            ~Barrier() {
                FinalizeBarrier(std::addressof(m_barrier));
            }

            void Await() {
                return AwaitBarrier(std::addressof(m_barrier));
            }

            operator BarrierType &() {
                return m_barrier;
            }

            operator const BarrierType &() const {
                return m_barrier;
            }

            BarrierType *GetBase() {
                return std::addressof(m_barrier);
            }
    };

}
