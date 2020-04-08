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
#include <stratosphere/os/os_semaphore_types.hpp>
#include <stratosphere/os/os_semaphore_api.hpp>

namespace ams::os {

    class Semaphore {
        NON_COPYABLE(Semaphore);
        NON_MOVEABLE(Semaphore);
        private:
            SemaphoreType sema;
        public:
            explicit Semaphore(s32 count, s32 max_count) {
                InitializeSemaphore(std::addressof(this->sema), count, max_count);
            }

            ~Semaphore() { FinalizeSemaphore(std::addressof(this->sema)); }

            void Acquire() {
                return os::AcquireSemaphore(std::addressof(this->sema));
            }

            bool TryAcquire() {
                return os::TryAcquireSemaphore(std::addressof(this->sema));
            }

            bool TimedAcquire(TimeSpan timeout) {
                return os::TimedAcquireSemaphore(std::addressof(this->sema), timeout);
            }

            void Release() {
                return os::ReleaseSemaphore(std::addressof(this->sema));
            }

            void Release(s32 count) {
                return os::ReleaseSemaphore(std::addressof(this->sema), count);
            }

            s32 GetCurrentCount() const {
                return os::GetCurrentSemaphoreCount(std::addressof(this->sema));
            }

            operator SemaphoreType &() {
                return this->sema;
            }

            operator const SemaphoreType &() const {
                return this->sema;
            }

            SemaphoreType *GetBase() {
                return std::addressof(this->sema);
            }
    };

}
