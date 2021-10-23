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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_k_slab_heap.hpp>
#include <mesosphere/kern_k_auto_object_container.hpp>
#include <mesosphere/kern_k_unused_slab_memory.hpp>

namespace ams::kern {

    template<class Derived, bool SupportDynamicExpansion = false>
    class KSlabAllocated {
        private:
            static constinit inline KSlabHeap<Derived, SupportDynamicExpansion> s_slab_heap;
        public:
            constexpr KSlabAllocated() = default;

            size_t GetSlabIndex() const {
                return s_slab_heap.GetIndex(static_cast<const Derived *>(this));
            }
        public:
            static void InitializeSlabHeap(void *memory, size_t memory_size) {
                s_slab_heap.Initialize(memory, memory_size);
            }

            static Derived *Allocate() {
                return s_slab_heap.Allocate();
            }

            static void Free(Derived *obj) {
                s_slab_heap.Free(obj);
            }

            template<bool Enable = SupportDynamicExpansion, typename = typename std::enable_if<Enable>::type>
            static Derived *AllocateFromUnusedSlabMemory() {
                static_assert(Enable == SupportDynamicExpansion);

                Derived * const obj = GetPointer<Derived>(AllocateUnusedSlabMemory(sizeof(Derived), alignof(Derived)));
                if (AMS_LIKELY(obj != nullptr)) {
                    std::construct_at(obj);
                }
                return obj;
            }

            static size_t GetObjectSize() { return s_slab_heap.GetObjectSize(); }
            static size_t GetSlabHeapSize() { return s_slab_heap.GetSlabHeapSize(); }
            static size_t GetPeakIndex() { return s_slab_heap.GetPeakIndex(); }
            static uintptr_t GetSlabHeapAddress() { return s_slab_heap.GetSlabHeapAddress(); }

            static size_t GetNumRemaining() { return s_slab_heap.GetNumRemaining(); }
    };

    template<typename Derived, typename Base, bool SupportDynamicExpansion = false>
    class KAutoObjectWithSlabHeapAndContainer : public Base {
        static_assert(std::is_base_of<KAutoObjectWithList, Base>::value);
        private:
            static constinit inline KSlabHeap<Derived, SupportDynamicExpansion> s_slab_heap;
            static constinit inline KAutoObjectWithListContainer s_container;
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
            constexpr explicit KAutoObjectWithSlabHeapAndContainer(util::ConstantInitializeTag) : Base(util::ConstantInitialize) { /* ... */ }

            explicit KAutoObjectWithSlabHeapAndContainer() { /* ... */ }

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
                    KAutoObject::Create<Derived>(obj);
                }
                return obj;
            }

            template<bool Enable = SupportDynamicExpansion, typename = typename std::enable_if<Enable>::type>
            static Derived *CreateFromUnusedSlabMemory() {
                static_assert(Enable == SupportDynamicExpansion);

                Derived * const obj = GetPointer<Derived>(AllocateUnusedSlabMemory(sizeof(Derived), alignof(Derived)));
                if (AMS_LIKELY(obj != nullptr)) {
                    std::construct_at(obj);
                    KAutoObject::Create<Derived>(obj);
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
