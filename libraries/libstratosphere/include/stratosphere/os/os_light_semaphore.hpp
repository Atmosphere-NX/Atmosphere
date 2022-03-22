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
#include <stratosphere/os/os_light_semaphore_types.hpp>
#include <stratosphere/os/os_light_semaphore_api.hpp>

namespace ams::os {

    class LightSemaphore {
        NON_COPYABLE(LightSemaphore);
        NON_MOVEABLE(LightSemaphore);
        private:
            LightSemaphoreType m_sema;
        public:
            explicit LightSemaphore(s32 count, s32 max_count) {
                InitializeLightSemaphore(std::addressof(m_sema), count, max_count);
            }

            ~LightSemaphore() { FinalizeLightSemaphore(std::addressof(m_sema)); }

            void Acquire() {
                return os::AcquireLightSemaphore(std::addressof(m_sema));
            }

            bool TryAcquire() {
                return os::TryAcquireLightSemaphore(std::addressof(m_sema));
            }

            bool TimedAcquire(TimeSpan timeout) {
                return os::TimedAcquireLightSemaphore(std::addressof(m_sema), timeout);
            }

            void Release() {
                return os::ReleaseLightSemaphore(std::addressof(m_sema));
            }

            void Release(s32 count) {
                return os::ReleaseLightSemaphore(std::addressof(m_sema), count);
            }

            s32 GetCurrentCount() const {
                return os::GetCurrentLightSemaphoreCount(std::addressof(m_sema));
            }

            operator LightSemaphoreType &() {
                return m_sema;
            }

            operator const LightSemaphoreType &() const {
                return m_sema;
            }

            LightSemaphoreType *GetBase() {
                return std::addressof(m_sema);
            }
    };

}
