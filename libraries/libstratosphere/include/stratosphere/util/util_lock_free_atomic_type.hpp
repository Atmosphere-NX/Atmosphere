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
#include <stratosphere/os/os_memory_fence_api.hpp>

namespace ams::util {

    template<typename T>
    struct LockFreeAtomicType {
        volatile u32 _counter;
        T _value[2];
    };

    template<typename T>
    void StoreToLockFreeAtomicType(LockFreeAtomicType<T> *p, const T &value) {
        /* Get the current counter. */
        auto counter = p->_counter;

        /* Increment the counter. */
        ++counter;

        /* Store the updated value. */
        p->_value[counter % 2] = value;

        /* Fence memory. */
        os::FenceMemoryStoreStore();

        /* Set the updated counter. */
        p->_counter = counter;
    }

    template<typename T>
    T LoadFromLockFreeAtomicType(const LockFreeAtomicType<T> *p) {
        while (true) {
            /* Get the counter. */
            auto counter = p->_counter;

            /* Get the value. */
            auto value = p->_value[counter % 2];

            /* Fence memory. */
            os::FenceMemoryLoadLoad();

            /* Check that the counter matches. */
            if (counter == p->_counter) {
                return value;
            }
        }
    }

}
