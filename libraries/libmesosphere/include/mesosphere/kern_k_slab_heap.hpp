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
#include <mesosphere/kern_k_typed_address.hpp>

#if defined(ATMOSPHERE_ARCH_ARM64)

    #include <mesosphere/arch/arm64/kern_k_slab_heap_impl.hpp>
    namespace ams::kern {
        using ams::kern::arch::arm64::AllocateFromSlabAtomic;
        using ams::kern::arch::arm64::FreeToSlabAtomic;
    }

#else

    #error "Unknown architecture for KSlabHeapImpl"

#endif


namespace ams::kern {

    namespace impl {

        class KSlabHeapImpl {
            NON_COPYABLE(KSlabHeapImpl);
            NON_MOVEABLE(KSlabHeapImpl);
            public:
                struct Node {
                    Node *next;
                };
            private:
                Node * m_head;
                size_t m_obj_size;
            public:
                constexpr KSlabHeapImpl() : m_head(nullptr), m_obj_size(0) { MESOSPHERE_ASSERT_THIS(); }

                void Initialize(size_t size) {
                    MESOSPHERE_INIT_ABORT_UNLESS(m_head == nullptr);
                    m_obj_size = size;
                }

                Node *GetHead() const {
                    return m_head;
                }

                size_t GetObjectSize() const {
                    return m_obj_size;
                }

                void *Allocate() {
                    MESOSPHERE_ASSERT_THIS();

                    return AllocateFromSlabAtomic(std::addressof(m_head));
                }

                void Free(void *obj) {
                    MESOSPHERE_ASSERT_THIS();

                    Node *node = reinterpret_cast<Node *>(obj);

                    return FreeToSlabAtomic(std::addressof(m_head), node);
                }
        };

    }

    class KSlabHeapBase {
        NON_COPYABLE(KSlabHeapBase);
        NON_MOVEABLE(KSlabHeapBase);
        private:
            using Impl = impl::KSlabHeapImpl;
        private:
            Impl m_impl;
            uintptr_t m_peak;
            uintptr_t m_start;
            uintptr_t m_end;
        private:
            ALWAYS_INLINE Impl *GetImpl() {
                return std::addressof(m_impl);
            }
            ALWAYS_INLINE const Impl *GetImpl() const {
                return std::addressof(m_impl);
            }
        public:
            constexpr KSlabHeapBase() : m_impl(), m_peak(0), m_start(0), m_end(0) { MESOSPHERE_ASSERT_THIS(); }

            ALWAYS_INLINE bool Contains(uintptr_t address) const {
                return m_start <= address && address < m_end;
            }

            void InitializeImpl(size_t obj_size, void *memory, size_t memory_size) {
                MESOSPHERE_ASSERT_THIS();

                /* Ensure we don't initialize a slab using null memory. */
                MESOSPHERE_ABORT_UNLESS(memory != nullptr);

                /* Initialize the base allocator. */
                this->GetImpl()->Initialize(obj_size);

                /* Set our tracking variables. */
                const size_t num_obj = (memory_size / obj_size);
                m_start = reinterpret_cast<uintptr_t>(memory);
                m_end = m_start + num_obj * obj_size;
                m_peak = m_start;

                /* Free the objects. */
                u8 *cur = reinterpret_cast<u8 *>(m_end);

                for (size_t i = 0; i < num_obj; i++) {
                    cur -= obj_size;
                    this->GetImpl()->Free(cur);
                }
            }

            size_t GetSlabHeapSize() const {
                return (m_end - m_start) / this->GetObjectSize();
            }

            size_t GetObjectSize() const {
                return this->GetImpl()->GetObjectSize();
            }

            void *AllocateImpl() {
                MESOSPHERE_ASSERT_THIS();

                void *obj = this->GetImpl()->Allocate();

                /* Track the allocated peak. */
                #if defined(MESOSPHERE_BUILD_FOR_DEBUGGING)
                if (AMS_LIKELY(obj != nullptr)) {
                    static_assert(std::atomic_ref<uintptr_t>::is_always_lock_free);
                    std::atomic_ref<uintptr_t> peak_ref(m_peak);

                    const uintptr_t alloc_peak = reinterpret_cast<uintptr_t>(obj) + this->GetObjectSize();
                    uintptr_t cur_peak = m_peak;
                    do {
                        if (alloc_peak <= cur_peak) {
                            break;
                        }
                    } while (!peak_ref.compare_exchange_strong(cur_peak, alloc_peak));
                }
                #endif

                return obj;
            }

            void FreeImpl(void *obj) {
                MESOSPHERE_ASSERT_THIS();

                /* Don't allow freeing an object that wasn't allocated from this heap. */
                MESOSPHERE_ABORT_UNLESS(this->Contains(reinterpret_cast<uintptr_t>(obj)));

                this->GetImpl()->Free(obj);
            }

            size_t GetObjectIndexImpl(const void *obj) const {
                return (reinterpret_cast<uintptr_t>(obj) - m_start) / this->GetObjectSize();
            }

            size_t GetPeakIndex() const {
                return this->GetObjectIndexImpl(reinterpret_cast<const void *>(m_peak));
            }

            uintptr_t GetSlabHeapAddress() const {
                return m_start;
            }

            size_t GetNumRemaining() const {
                size_t remaining = 0;

                /* Only calculate the number of remaining objects under debug configuration. */
                #if defined(MESOSPHERE_BUILD_FOR_DEBUGGING)
                while (true) {
                    auto *cur = this->GetImpl()->GetHead();
                    remaining = 0;

                    while (this->Contains(reinterpret_cast<uintptr_t>(cur))) {
                        ++remaining;
                        cur = cur->next;
                    }

                    if (cur == nullptr) {
                        break;
                    }
                }
                #endif

                return remaining;
            }
    };

    template<typename T>
    class KSlabHeap : public KSlabHeapBase {
        public:
            constexpr KSlabHeap() : KSlabHeapBase() { /* ... */ }

            void Initialize(void *memory, size_t memory_size) {
                this->InitializeImpl(sizeof(T), memory, memory_size);
            }

            T *Allocate() {
                T *obj = reinterpret_cast<T *>(this->AllocateImpl());
                if (AMS_LIKELY(obj != nullptr)) {
                    new (obj) T();
                }
                return obj;
            }

            void Free(T *obj) {
                this->FreeImpl(obj);
            }

            size_t GetObjectIndex(const T *obj) const {
                return this->GetObjectIndexImpl(obj);
            }
    };

}
