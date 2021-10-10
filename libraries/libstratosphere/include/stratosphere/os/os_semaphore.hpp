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
#include <stratosphere/os/os_semaphore_types.hpp>
#include <stratosphere/os/os_semaphore_api.hpp>

namespace ams::os {

    class Semaphore {
        NON_COPYABLE(Semaphore);
        NON_MOVEABLE(Semaphore);
        private:
            SemaphoreType m_sema;
        public:
            explicit Semaphore(s32 count, s32 max_count) {
                InitializeSemaphore(std::addressof(m_sema), count, max_count);
            }

            ~Semaphore() { FinalizeSemaphore(std::addressof(m_sema)); }

            void Acquire() {
                return os::AcquireSemaphore(std::addressof(m_sema));
            }

            bool TryAcquire() {
                return os::TryAcquireSemaphore(std::addressof(m_sema));
            }

            bool TimedAcquire(TimeSpan timeout) {
                return os::TimedAcquireSemaphore(std::addressof(m_sema), timeout);
            }

            void Release() {
                return os::ReleaseSemaphore(std::addressof(m_sema));
            }

            void Release(s32 count) {
                return os::ReleaseSemaphore(std::addressof(m_sema), count);
            }

            s32 GetCurrentCount() const {
                return os::GetCurrentSemaphoreCount(std::addressof(m_sema));
            }

            operator SemaphoreType &() {
                return m_sema;
            }

            operator const SemaphoreType &() const {
                return m_sema;
            }

            SemaphoreType *GetBase() {
                return std::addressof(m_sema);
            }
    };

}
