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
#include <mesosphere/kern_k_memory_region_type.hpp>

namespace ams::kern {

    class KMemoryRegionTree;

    class KMemoryRegion : public util::IntrusiveRedBlackTreeBaseNode<KMemoryRegion> {
        NON_COPYABLE(KMemoryRegion);
        NON_MOVEABLE(KMemoryRegion);
        private:
            friend class KMemoryRegionTree;
        private:
            uintptr_t address;
            uintptr_t pair_address;
            uintptr_t last_address;
            u32 attributes;
            u32 type_id;
        public:
            static constexpr ALWAYS_INLINE int Compare(const KMemoryRegion &lhs, const KMemoryRegion &rhs) {
                if (lhs.GetAddress() < rhs.GetAddress()) {
                    return -1;
                } else if (lhs.GetAddress() <= rhs.GetLastAddress()) {
                    return 0;
                } else {
                    return 1;
                }
            }
        public:
            constexpr ALWAYS_INLINE KMemoryRegion() : address(0), pair_address(0), last_address(0), attributes(0), type_id(0) { /* ... */ }
            constexpr ALWAYS_INLINE KMemoryRegion(uintptr_t a, size_t la, uintptr_t p, u32 r, u32 t) :
                address(a), pair_address(p), last_address(la), attributes(r), type_id(t)
            {
                /* ... */
            }
            constexpr ALWAYS_INLINE KMemoryRegion(uintptr_t a, size_t la, u32 r, u32 t) : KMemoryRegion(a, la, std::numeric_limits<uintptr_t>::max(), r, t) { /* ... */ }
        private:
            constexpr ALWAYS_INLINE void Reset(uintptr_t a, uintptr_t la, uintptr_t p, u32 r, u32 t) {
                this->address      = a;
                this->pair_address = p;
                this->last_address  = la;
                this->attributes   = r;
                this->type_id      = t;
            }
        public:
            constexpr ALWAYS_INLINE uintptr_t GetAddress() const {
                return this->address;
            }

            constexpr ALWAYS_INLINE uintptr_t GetPairAddress() const {
                return this->pair_address;
            }

            constexpr ALWAYS_INLINE uintptr_t GetLastAddress() const {
                return this->last_address;
            }

            constexpr ALWAYS_INLINE uintptr_t GetEndAddress() const {
                return this->GetLastAddress() + 1;
            }

            constexpr ALWAYS_INLINE size_t GetSize() const {
                return this->GetEndAddress() - this->GetAddress();
            }

            constexpr ALWAYS_INLINE u32 GetAttributes() const {
                return this->attributes;
            }

            constexpr ALWAYS_INLINE u32 GetType() const {
                return this->type_id;
            }

            constexpr ALWAYS_INLINE void SetType(u32 type) {
                MESOSPHERE_INIT_ABORT_UNLESS(this->CanDerive(type));
                this->type_id = type;
            }

            constexpr ALWAYS_INLINE bool Contains(uintptr_t address) const {
                MESOSPHERE_INIT_ABORT_UNLESS(this->GetEndAddress() != 0);
                return this->GetAddress() <= address && address <= this->GetLastAddress();
            }

            constexpr ALWAYS_INLINE bool IsDerivedFrom(u32 type) const {
                return (this->GetType() | type) == this->GetType();
            }

            constexpr ALWAYS_INLINE bool HasTypeAttribute(KMemoryRegionAttr attr) const {
                return (this->GetType() | attr) == this->GetType();
            }

            constexpr ALWAYS_INLINE bool CanDerive(u32 type) const {
                return (this->GetType() | type) == type;
            }

            constexpr ALWAYS_INLINE void SetPairAddress(uintptr_t a) {
                this->pair_address = a;
            }

            constexpr ALWAYS_INLINE void SetTypeAttribute(KMemoryRegionAttr attr) {
                this->type_id |= attr;
            }
    };
    static_assert(std::is_trivially_destructible<KMemoryRegion>::value);

    class KMemoryRegionTree {
        public:
            struct DerivedRegionExtents {
                const KMemoryRegion *first_region;
                const KMemoryRegion *last_region;

                constexpr DerivedRegionExtents() : first_region(nullptr), last_region(nullptr) { /* ... */ }

                constexpr ALWAYS_INLINE uintptr_t GetAddress() const {
                    return this->first_region->GetAddress();
                }

                constexpr ALWAYS_INLINE uintptr_t GetLastAddress() const {
                    return this->last_region->GetLastAddress();
                }

                constexpr ALWAYS_INLINE uintptr_t GetEndAddress() const {
                    return this->GetLastAddress() + 1;
                }

                constexpr ALWAYS_INLINE size_t GetSize() const {
                    return this->GetEndAddress() - this->GetAddress();
                }
            };
        private:
            using TreeType = util::IntrusiveRedBlackTreeBaseTraits<KMemoryRegion>::TreeType<KMemoryRegion>;
        public:
            using value_type        = TreeType::value_type;
            using size_type         = TreeType::size_type;
            using difference_type   = TreeType::difference_type;
            using pointer           = TreeType::pointer;
            using const_pointer     = TreeType::const_pointer;
            using reference         = TreeType::reference;
            using const_reference   = TreeType::const_reference;
            using iterator          = TreeType::iterator;
            using const_iterator    = TreeType::const_iterator;
        private:
            TreeType tree;
        public:
            constexpr ALWAYS_INLINE KMemoryRegionTree() : tree() { /* ... */ }
        public:
            KMemoryRegion *FindModifiable(uintptr_t address) {
                if (auto it = this->find(KMemoryRegion(address, address, 0, 0)); it != this->end()) {
                    return std::addressof(*it);
                } else {
                    return nullptr;
                }
            }

            const KMemoryRegion *Find(uintptr_t address) const {
                if (auto it = this->find(KMemoryRegion(address, address, 0, 0)); it != this->cend()) {
                    return std::addressof(*it);
                } else {
                    return nullptr;
                }
            }

            const KMemoryRegion *FindByType(u32 type_id) const {
                for (auto it = this->cbegin(); it != this->cend(); ++it) {
                    if (it->GetType() == type_id) {
                        return std::addressof(*it);
                    }
                }
                return nullptr;
            }

            const KMemoryRegion *FindByTypeAndAttribute(u32 type_id, u32 attr) const {
                for (auto it = this->cbegin(); it != this->cend(); ++it) {
                    if (it->GetType() == type_id && it->GetAttributes() == attr) {
                        return std::addressof(*it);
                    }
                }
                return nullptr;
            }

            const KMemoryRegion *FindFirstDerived(u32 type_id) const {
                for (auto it = this->cbegin(); it != this->cend(); it++) {
                    if (it->IsDerivedFrom(type_id)) {
                        return std::addressof(*it);
                    }
                }
                return nullptr;
            }

            const KMemoryRegion *FindLastDerived(u32 type_id) const {
                const KMemoryRegion *region = nullptr;
                for (auto it = this->begin(); it != this->end(); it++) {
                    if (it->IsDerivedFrom(type_id)) {
                        region = std::addressof(*it);
                    }
                }
                return region;
            }


            DerivedRegionExtents GetDerivedRegionExtents(u32 type_id) const {
                DerivedRegionExtents extents;

                MESOSPHERE_INIT_ABORT_UNLESS(extents.first_region == nullptr);
                MESOSPHERE_INIT_ABORT_UNLESS(extents.last_region  == nullptr);

                for (auto it = this->cbegin(); it != this->cend(); it++) {
                    if (it->IsDerivedFrom(type_id)) {
                        if (extents.first_region == nullptr) {
                            extents.first_region = std::addressof(*it);
                        }
                        extents.last_region = std::addressof(*it);
                    }
                }

                MESOSPHERE_INIT_ABORT_UNLESS(extents.first_region != nullptr);
                MESOSPHERE_INIT_ABORT_UNLESS(extents.last_region  != nullptr);

                return extents;
            }
        public:
            NOINLINE void InsertDirectly(uintptr_t address, uintptr_t last_address, u32 attr = 0, u32 type_id = 0);
            NOINLINE bool Insert(uintptr_t address, size_t size, u32 type_id, u32 new_attr = 0, u32 old_attr = 0);

            NOINLINE KVirtualAddress GetRandomAlignedRegion(size_t size, size_t alignment, u32 type_id);

            ALWAYS_INLINE KVirtualAddress GetRandomAlignedRegionWithGuard(size_t size, size_t alignment, u32 type_id, size_t guard_size) {
                return this->GetRandomAlignedRegion(size + 2 * guard_size, alignment, type_id) + guard_size;
            }
        public:
            /* Iterator accessors. */
            iterator begin() {
                return this->tree.begin();
            }

            const_iterator begin() const {
                return this->tree.begin();
            }

            iterator end() {
                return this->tree.end();
            }

            const_iterator end() const {
                return this->tree.end();
            }

            const_iterator cbegin() const {
                return this->begin();
            }

            const_iterator cend() const {
                return this->end();
            }

            iterator iterator_to(reference ref) {
                return this->tree.iterator_to(ref);
            }

            const_iterator iterator_to(const_reference ref) const {
                return this->tree.iterator_to(ref);
            }

            /* Content management. */
            bool empty() const {
                return this->tree.empty();
            }

            reference back() {
                return this->tree.back();
            }

            const_reference back() const {
                return this->tree.back();
            }

            reference front() {
                return this->tree.front();
            }

            const_reference front() const {
                return this->tree.front();
            }

            /* GCC over-eagerly inlines this operation. */
            NOINLINE iterator insert(reference ref) {
                return this->tree.insert(ref);
            }

            NOINLINE iterator erase(iterator it) {
                return this->tree.erase(it);
            }

            iterator find(const_reference ref) const {
                return this->tree.find(ref);
            }

            iterator nfind(const_reference ref) const {
                return this->tree.nfind(ref);
            }
    };

}
