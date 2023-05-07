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

    template<typename Derived, typename Base, bool SupportDynamicExpansion> requires std::derived_from<Base, KAutoObject>
    class KAutoObjectWithSlabHeapBase : public Base {
        private:
            template<typename, typename, bool> friend class KAutoObjectWithSlabHeap;
            template<typename, typename, bool> friend class KAutoObjectWithSlabHeapAndContainer;
        private:
            static constinit inline KSlabHeap<Derived, SupportDynamicExpansion> s_slab_heap;
        private:
            static ALWAYS_INLINE Derived *Allocate() {
                return s_slab_heap.Allocate();
            }

            static ALWAYS_INLINE void Free(Derived *obj) {
                s_slab_heap.Free(obj);
            }
        private:
            static ALWAYS_INLINE bool IsInitialized(const Derived *obj) {
                if constexpr (requires { { obj->IsInitialized() } -> std::same_as<bool>; }) {
                    return obj->IsInitialized();
                } else {
                    return true;
                }
            }

            static ALWAYS_INLINE uintptr_t GetPostDestroyArgument(const Derived *obj) {
                if constexpr (requires { { obj->GetPostDestroyArgument() } -> std::same_as<uintptr_t>; }) {
                    return obj->GetPostDestroyArgument();
                } else {
                    return 0;
                }
            }
        public:
            constexpr explicit KAutoObjectWithSlabHeapBase(util::ConstantInitializeTag) : Base(util::ConstantInitialize) { /* ... */ }

            explicit KAutoObjectWithSlabHeapBase() { /* ... */ }

            /* NOTE: IsInitialized() and GetPostDestroyArgument() are virtual functions declared in this class, */
            /* in Nintendo's kernel. We fully devirtualize them, as Destroy() is the only user of them. */
            /* We also devirtualize KAutoObject::Finalize(), which is only used by this function in Nintendo's kernel. */
            virtual void Destroy() override final {
                Derived * const derived = static_cast<Derived *>(this);

                if (KAutoObjectWithSlabHeapBase<Derived, Base, SupportDynamicExpansion>::IsInitialized(derived)) {
                    Derived::PreFinalize(derived);
                    const uintptr_t arg = KAutoObjectWithSlabHeapBase<Derived, Base, SupportDynamicExpansion>::GetPostDestroyArgument(derived);
                    derived->Finalize();
                    KAutoObjectWithSlabHeapBase<Derived, Base, SupportDynamicExpansion>::Free(derived);
                    Derived::PostDestroy(arg);
                } else {
                    KAutoObjectWithSlabHeapBase<Derived, Base, SupportDynamicExpansion>::Free(derived);
                }
            }

            size_t GetSlabIndex() const {
                return s_slab_heap.GetObjectIndex(static_cast<const Derived *>(this));
            }
        public:
            static void InitializeSlabHeap(void *memory, size_t memory_size) {
                s_slab_heap.Initialize(memory, memory_size);
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

            static size_t GetObjectSize() { return s_slab_heap.GetObjectSize(); }
            static size_t GetSlabHeapSize() { return s_slab_heap.GetSlabHeapSize(); }
            static size_t GetPeakIndex() { return s_slab_heap.GetPeakIndex(); }
            static uintptr_t GetSlabHeapAddress() { return s_slab_heap.GetSlabHeapAddress(); }

            static size_t GetNumRemaining() { return s_slab_heap.GetNumRemaining(); }
    };

    template<typename Derived, typename Base, bool SupportDynamicExpansion = false>
    class KAutoObjectWithSlabHeap : public KAutoObjectWithSlabHeapBase<Derived, Base, SupportDynamicExpansion> {
        public:
            constexpr explicit KAutoObjectWithSlabHeap(util::ConstantInitializeTag) : KAutoObjectWithSlabHeapBase<Derived, Base, SupportDynamicExpansion>(util::ConstantInitialize) { /* ... */ }

            explicit KAutoObjectWithSlabHeap() { /* ... */ }

            static ALWAYS_INLINE void PreFinalize(Derived *) { /* ... */ }
    };


    template<typename Derived, typename Base, bool SupportDynamicExpansion = false>
    class KAutoObjectWithSlabHeapAndContainer : public KAutoObjectWithSlabHeapBase<Derived, Base, SupportDynamicExpansion> {
        static_assert(std::derived_from<Base, KAutoObjectWithList>);
        private:
            static constinit inline KAutoObjectWithListContainer<Derived> s_container;
        public:
            class ListAccessor : public KAutoObjectWithListContainer<Derived>::ListAccessor {
                public:
                    ALWAYS_INLINE ListAccessor() : KAutoObjectWithListContainer<Derived>::ListAccessor(s_container) { /* ... */ }
                    ALWAYS_INLINE ~ListAccessor() { /* ... */ }
            };
        public:
            constexpr explicit KAutoObjectWithSlabHeapAndContainer(util::ConstantInitializeTag) : KAutoObjectWithSlabHeapBase<Derived, Base, SupportDynamicExpansion>(util::ConstantInitialize) { /* ... */ }

            explicit KAutoObjectWithSlabHeapAndContainer() { /* ... */ }
        public:
            static void InitializeSlabHeap(void *memory, size_t memory_size) {
                KAutoObjectWithSlabHeapBase<Derived, Base, SupportDynamicExpansion>::InitializeSlabHeap(memory, memory_size);
                s_container.Initialize();
            }

            static void Register(Derived *obj) {
                return s_container.Register(obj);
            }

            static ALWAYS_INLINE void PreFinalize(Derived *obj) { s_container.Unregister(obj); }
    };

}
