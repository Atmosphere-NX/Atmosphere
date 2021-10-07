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
#include <stratosphere/fs/fs_memory_management.hpp>

namespace ams::fs::impl {

    class Newable {
        public:
            static ALWAYS_INLINE void *operator new(size_t size) noexcept {
                return ::ams::fs::impl::Allocate(size);
            }

            static ALWAYS_INLINE void *operator new(size_t size, Newable *placement) noexcept {
                AMS_UNUSED(size);
                return placement;
            }

            static ALWAYS_INLINE void *operator new[](size_t size) noexcept {
                return ::ams::fs::impl::Allocate(size);
            }

            static ALWAYS_INLINE void operator delete(void *ptr, size_t size) noexcept {
                return ::ams::fs::impl::Deallocate(ptr, size);
            }

            static ALWAYS_INLINE void operator delete[](void *ptr, size_t size) noexcept {
                return ::ams::fs::impl::Deallocate(ptr, size);
            }
    };

}
