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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>

namespace ams::util {

    template<typename _Mutex>
    class unique_lock {
        NON_COPYABLE(unique_lock);
        public:
            using mutex_type = _Mutex;
        private:
            mutex_type *m_mutex;
            bool m_owns;
        public:
            unique_lock() noexcept : m_mutex(nullptr), m_owns(false) { /* ... */ }

            explicit unique_lock(mutex_type &m) noexcept : m_mutex(std::addressof(m)), m_owns(false) {
                this->lock();
                m_owns = true;
            }

            unique_lock(mutex_type &m, std::defer_lock_t)  noexcept : m_mutex(std::addressof(m)), m_owns(false) { /* ... */ }
            unique_lock(mutex_type &m, std::try_to_lock_t) noexcept : m_mutex(std::addressof(m)), m_owns(m_mutex->try_lock()) { /* ... */ }
            unique_lock(mutex_type &m, std::adopt_lock_t)  noexcept : m_mutex(std::addressof(m)), m_owns(true) { /* ... */ }

            template<typename _Clock, typename _Duration>
            unique_lock(mutex_type &m, const std::chrono::time_point<_Clock, _Duration> &time) noexcept : m_mutex(std::addressof(m)), m_owns(m_mutex->try_lock_until(time)) { /* ... */ }

            template<typename _Rep, typename _Period>
            unique_lock(mutex_type &m, const std::chrono::duration<_Rep, _Period> &time) noexcept : m_mutex(std::addressof(m)), m_owns(m_mutex->try_lock_for(time)) { /* ... */ }

            ~unique_lock() noexcept {
                if (m_owns) {
                    this->unlock();
                }
            }

            unique_lock(unique_lock &&rhs) noexcept : m_mutex(rhs.m_mutex), m_owns(rhs.m_owns) {
                rhs.m_mutex = nullptr;
                rhs.m_owns  = false;
            }

            unique_lock &operator=(unique_lock &&rhs) noexcept {
                if (m_owns) {
                    this->unlock();
                }

                unique_lock(std::move(rhs)).swap(*this);

                rhs.m_mutex = nullptr;
                rhs.m_owns  = false;

                return *this;
            }

            void lock() noexcept {
                AMS_ABORT_UNLESS(m_mutex);
                AMS_ABORT_UNLESS(!m_owns);

                m_mutex->lock();
                m_owns = true;
            }

            bool try_lock() noexcept {
                AMS_ABORT_UNLESS(m_mutex);
                AMS_ABORT_UNLESS(!m_owns);

                m_owns = m_mutex->try_lock();
                return m_owns;
            }

            template<typename _Clock, typename _Duration>
            bool try_lock_until(const std::chrono::time_point<_Clock, _Duration> &time) noexcept {
                AMS_ABORT_UNLESS(m_mutex);
                AMS_ABORT_UNLESS(!m_owns);
                m_owns = m_mutex->try_lock_until(time);
                return m_owns;
            }

            template<typename _Rep, typename _Period>
            bool try_lock_for(const std::chrono::duration<_Rep, _Period> &time) noexcept {
                AMS_ABORT_UNLESS(m_mutex);
                AMS_ABORT_UNLESS(!m_owns);
                m_owns = m_mutex->try_lock_for(time);
                return m_owns;
            }


            void unlock() noexcept {
                AMS_ABORT_UNLESS(m_owns);
                if (m_mutex) {
                    m_mutex->unlock();
                    m_owns = false;
                }
            }

            void swap(unique_lock &rhs) noexcept {
                std::swap(m_mutex, rhs.m_mutex);
                std::swap(m_owns, rhs.m_owns);
            }

            mutex_type *release() noexcept {
                mutex_type *ret = m_mutex;
                m_mutex = nullptr;
                m_owns  = false;
                return ret;
            }

            bool owns_lock() const noexcept {
                return m_owns;
            }

            explicit operator bool() const noexcept {
                return this->owns_lock();
            }

            mutex_type *mutex() const noexcept {
                return m_mutex;
            }
    };

    template<typename _Mutex>
    inline void swap(unique_lock<_Mutex> &lhs, unique_lock<_Mutex> &rhs) noexcept { return lhs.swap(rhs); }

}
