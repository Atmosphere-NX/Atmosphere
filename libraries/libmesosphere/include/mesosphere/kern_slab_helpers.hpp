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
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_k_slab_heap.hpp>
#include <mesosphere/kern_k_auto_object_container.hpp>

namespace ams::kern {

    template<class Derived>
    class KSlabAllocated {
        private:
            static inline KSlabHeap<Derived> s_slab_heap;
        public:
            constexpr KSlabAllocated() { /* ... */ }

            size_t GetSlabIndex() const {
                return s_slab_heap.GetIndex(static_cast<const Derived *>(this));
            }
        public:
            static void InitializeSlabHeap(void *memory, size_t memory_size) {
                s_slab_heap.Initialize(memory, memory_size);
            }

            static ALWAYS_INLINE Derived *Allocate() {
                return s_slab_heap.Allocate();
            }

            static ALWAYS_INLINE void Free(Derived *obj) {
                s_slab_heap.Free(obj);
            }

            static size_t GetObjectSize() { return s_slab_heap.GetObjectSize(); }
            static size_t GetSlabHeapSize() { return s_slab_heap.GetSlabHeapSize(); }
            static size_t GetPeakIndex() { return s_slab_heap.GetPeakIndex(); }
            static uintptr_t GetSlabHeapAddress() { return s_slab_heap.GetSlabHeapAddress(); }

            static size_t GetNumRemaining() { return s_slab_heap.GetNumRemaining(); }
    };

    template<typename Derived, typename Base>
    class KAutoObjectWithSlabHeapAndContainer : public Base {
        static_assert(std::is_base_of<KAutoObjectWithList, Base>::value);
        private:
            static inline KSlabHeap<Derived> s_slab_heap;
            static inline KAutoObjectWithListContainer s_container;
        private:
            static ALWAYS_INLINE Derived *Allocate() {
                return s_slab_heap.Allocate();
            }

            static ALWAYS_INLINE void Free(Derived *obj) {
                s_slab_heap.Free(obj);
            }
        public:
            class ListAccessor : public KAutoObjectWithListContainer::ListAccessor {
                public:
                    ALWAYS_INLINE ListAccessor() : KAutoObjectWithListContainer::ListAccessor(s_container) { /* ... */ }
                    ALWAYS_INLINE ~ListAccessor() { /* ... */ }
            };
        public:
            constexpr KAutoObjectWithSlabHeapAndContainer() : Base() { /* ... */ }
            virtual ~KAutoObjectWithSlabHeapAndContainer() { /* ... */ }

            virtual void Destroy() override {
                const bool is_initialized = this->IsInitialized();
                uintptr_t arg = 0;
                if (is_initialized) {
                    s_container.Unregister(this);
                    arg = this->GetPostDestroyArgument();
                    this->Finalize();
                }
                Free(static_cast<Derived *>(this));
                if (is_initialized) {
                    Derived::PostDestroy(arg);
                }
            }

            virtual bool IsInitialized() const { return true; }
            virtual uintptr_t GetPostDestroyArgument() const { return 0; }

            size_t GetSlabIndex() const {
                return s_slab_heap.GetObjectIndex(static_cast<const Derived *>(this));
            }
        public:
            static void InitializeSlabHeap(void *memory, size_t memory_size) {
                s_slab_heap.Initialize(memory, memory_size);
                s_container.Initialize();
            }

            static Derived *Create() {
                Derived *obj = Allocate();
                if (AMS_LIKELY(obj != nullptr)) {
                    KAutoObject::Create(obj);
                }
                return obj;
            }

            static void Register(Derived *obj) {
                return s_container.Register(obj);
            }

            static size_t GetObjectSize() { return s_slab_heap.GetObjectSize(); }
            static size_t GetSlabHeapSize() { return s_slab_heap.GetSlabHeapSize(); }
            static size_t GetPeakIndex() { return s_slab_heap.GetPeakIndex(); }
            static uintptr_t GetSlabHeapAddress() { return s_slab_heap.GetSlabHeapAddress(); }

            static size_t GetNumRemaining() { return s_slab_heap.GetNumRemaining(); }
    };

}
