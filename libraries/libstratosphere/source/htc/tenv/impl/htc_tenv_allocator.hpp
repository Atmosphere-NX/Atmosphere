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
#include <stratosphere.hpp>

namespace ams::htc::tenv::impl {

    void InitializeAllocator(AllocateFunction allocate, DeallocateFunction deallocate);

    void *Allocate(size_t size);
    void Deallocate(void *p, size_t size);

    class SfAllocator {
        public:
            constexpr ALWAYS_INLINE SfAllocator() { /* ... */ }

            void *Allocate(size_t size) {
                return ::ams::htc::tenv::impl::Allocate(size);
            }

            void Deallocate(void *p, size_t size) {
                return ::ams::htc::tenv::impl::Deallocate(p, size);
            }
    };

    using SfPolicy = sf::StatelessAllocationPolicy<SfAllocator>;
    using SfObjectFactory = sf::ObjectFactory<SfPolicy>;

}
