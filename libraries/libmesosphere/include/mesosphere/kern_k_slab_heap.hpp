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
#include <mesosphere/kern_k_typed_address.hpp>
#include <mesosphere/kern_k_memory_layout.hpp>

#if defined(ATMOSPHERE_ARCH_ARM64)

    #include <mesosphere/arch/arm64/kern_k_slab_heap_impl.hpp>
    namespace ams::kern {
        using ams::kern::arch::arm64::IsSlabAtomicValid;
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
                Node *m_head{nullptr};
            public:
                constexpr KSlabHeapImpl() = default;

                void Initialize() {
                    MESOSPHERE_ABORT_UNLESS(m_head == nullptr);
                    MESOSPHERE_ABORT_UNLESS(IsSlabAtomicValid());
                }

                ALWAYS_INLINE Node *GetHead() const {
                    return m_head;
                }

                ALWAYS_INLINE void *Allocate() {
                    return AllocateFromSlabAtomic(std::addressof(m_head));
                }

                ALWAYS_INLINE void Free(void *obj) {
                    return FreeToSlabAtomic(std::addressof(m_head), static_cast<Node *>(obj));
                }
        };

    }

    template<bool SupportDynamicExpansion>
    class KSlabHeapBase : protected impl::KSlabHeapImpl {
        NON_COPYABLE(KSlabHeapBase);
        NON_MOVEABLE(KSlabHeapBase);
        private:
            size_t m_obj_size{};
            uintptr_t m_peak{};
            uintptr_t m_start{};
            uintptr_t m_end{};
        private:
            ALWAYS_INLINE void UpdatePeakImpl(uintptr_t obj) {
                const util::AtomicRef<uintptr_t> peak_ref(m_peak);

                const uintptr_t alloc_peak = obj + this->GetObjectSize();
                uintptr_t cur_peak = m_peak;
                do {
                    if (alloc_peak <= cur_peak) {
                        break;
                    }
                } while (!peak_ref.CompareExchangeStrong(cur_peak, alloc_peak));
            }
        public:
            constexpr KSlabHeapBase() = default;

            ALWAYS_INLINE bool Contains(uintptr_t address) const {
                return m_start <= address && address < m_end;
            }

            void Initialize(size_t obj_size, void *memory, size_t memory_size) {
                /* Ensure we don't initialize a slab using null memory. */
                MESOSPHERE_ABORT_UNLESS(memory != nullptr);

                /* Set our object size. */
                m_obj_size = obj_size;

                /* Initialize the base allocator. */
                KSlabHeapImpl::Initialize();

                /* Set our tracking variables. */
                const size_t num_obj = (memory_size / obj_size);
                m_start = reinterpret_cast<uintptr_t>(memory);
                m_end   = m_start + num_obj * obj_size;
                m_peak  = m_start;

                /* Free the objects. */
                u8 *cur = reinterpret_cast<u8 *>(m_end);

                for (size_t i = 0; i < num_obj; i++) {
                    cur -= obj_size;
                    KSlabHeapImpl::Free(cur);
                }
            }

            ALWAYS_INLINE size_t GetSlabHeapSize() const {
                return (m_end - m_start) / this->GetObjectSize();
            }

            ALWAYS_INLINE size_t GetObjectSize() const {
                return m_obj_size;
            }

            ALWAYS_INLINE void *Allocate() {
                void *obj = KSlabHeapImpl::Allocate();

                /* Track the allocated peak. */
                #if defined(MESOSPHERE_BUILD_FOR_DEBUGGING)
                if (AMS_LIKELY(obj != nullptr)) {
                    if constexpr (SupportDynamicExpansion) {
                        if (this->Contains(reinterpret_cast<uintptr_t>(obj))) {
                            this->UpdatePeakImpl(reinterpret_cast<uintptr_t>(obj));
                        } else {
                            this->UpdatePeakImpl(reinterpret_cast<uintptr_t>(m_end) - this->GetObjectSize());
                        }
                    } else {
                        this->UpdatePeakImpl(reinterpret_cast<uintptr_t>(obj));
                    }
                }
                #endif

                return obj;
            }

            ALWAYS_INLINE void Free(void *obj) {
                /* Don't allow freeing an object that wasn't allocated from this heap. */
                const bool contained = this->Contains(reinterpret_cast<uintptr_t>(obj));
                if constexpr (SupportDynamicExpansion) {
                    const bool is_slab = KMemoryLayout::GetSlabRegion().Contains(reinterpret_cast<uintptr_t>(obj));
                    MESOSPHERE_ABORT_UNLESS(contained || is_slab);
                } else {
                    MESOSPHERE_ABORT_UNLESS(contained);
                }

                KSlabHeapImpl::Free(obj);
            }

            ALWAYS_INLINE size_t GetObjectIndex(const void *obj) const {
                if constexpr (SupportDynamicExpansion) {
                    if (!this->Contains(reinterpret_cast<uintptr_t>(obj))) {
                        return std::numeric_limits<size_t>::max();
                    }
                }

                return (reinterpret_cast<uintptr_t>(obj) - m_start) / this->GetObjectSize();
            }

            ALWAYS_INLINE size_t GetPeakIndex() const {
                return this->GetObjectIndex(reinterpret_cast<const void *>(m_peak));
            }

            ALWAYS_INLINE uintptr_t GetSlabHeapAddress() const {
                return m_start;
            }

            ALWAYS_INLINE size_t GetNumRemaining() const {
                size_t remaining = 0;

                /* Only calculate the number of remaining objects under debug configuration. */
                #if defined(MESOSPHERE_BUILD_FOR_DEBUGGING)
                while (true) {
                    auto *cur = this->GetHead();
                    remaining = 0;

                    if constexpr (SupportDynamicExpansion) {
                        const auto &slab_region = KMemoryLayout::GetSlabRegion();

                        while (this->Contains(reinterpret_cast<uintptr_t>(cur)) || slab_region.Contains(reinterpret_cast<uintptr_t>(cur))) {
                            ++remaining;
                            cur = cur->next;
                        }
                    } else {
                        while (this->Contains(reinterpret_cast<uintptr_t>(cur))) {
                            ++remaining;
                            cur = cur->next;
                        }
                    }

                    if (cur == nullptr) {
                        break;
                    }
                }
                #endif

                return remaining;
            }
    };

    template<typename T, bool SupportDynamicExpansion>
    class KSlabHeap : public KSlabHeapBase<SupportDynamicExpansion> {
        private:
            using BaseHeap = KSlabHeapBase<SupportDynamicExpansion>;
        public:
            constexpr KSlabHeap() = default;

            void Initialize(void *memory, size_t memory_size) {
                BaseHeap::Initialize(sizeof(T), memory, memory_size);
            }

            ALWAYS_INLINE T *Allocate() {
                T *obj = static_cast<T *>(BaseHeap::Allocate());
                if (AMS_LIKELY(obj != nullptr)) {
                    std::construct_at(obj);
                }
                return obj;
            }

            ALWAYS_INLINE void Free(T *obj) {
                BaseHeap::Free(obj);
            }

            ALWAYS_INLINE size_t GetObjectIndex(const T *obj) const {
                return BaseHeap::GetObjectIndex(obj);
            }
    };

}
