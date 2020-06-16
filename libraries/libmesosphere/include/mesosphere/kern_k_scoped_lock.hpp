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
#include <mesosphere/kern_common.hpp>

namespace ams::kern {

    template<typename T>
    concept KLockable = !std::is_reference<T>::value && requires (T &t) {
        { t.Lock()   } -> std::same_as<void>;
        { t.Unlock() } -> std::same_as<void>;
    };

    template<typename T> requires KLockable<T>
    class KScopedLock {
        NON_COPYABLE(KScopedLock);
        NON_MOVEABLE(KScopedLock);
        private:
            T *lock_ptr;
        public:
            explicit ALWAYS_INLINE KScopedLock(T *l) : lock_ptr(l) { this->lock_ptr->Lock(); }
            explicit ALWAYS_INLINE KScopedLock(T &l) : KScopedLock(std::addressof(l)) { /* ... */ }
            ALWAYS_INLINE ~KScopedLock() { this->lock_ptr->Unlock(); }
    };

}
