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
#include <stratosphere.hpp>

namespace ams::sf {

    namespace {

        struct DefaultAllocatorImpl {
            os::SdkMutexType tls_lock;
            std::atomic_bool tls_allocated;
            os::TlsSlot current_mr_tls_slot;
            MemoryResource *default_mr;

            void EnsureCurrentMemoryResourceTlsSlotInitialized() {
                if (!tls_allocated.load(std::memory_order_acquire)) {
                    os::LockSdkMutex(std::addressof(tls_lock));
                    if (!tls_allocated.load(std::memory_order_relaxed)) {
                        R_ABORT_UNLESS(os::SdkAllocateTlsSlot(std::addressof(current_mr_tls_slot), nullptr));
                        tls_allocated.store(true, std::memory_order_release);
                    }
                    os::UnlockSdkMutex(std::addressof(tls_lock));
                }
            }

            MemoryResource *GetDefaultMemoryResource() {
                return default_mr;
            }

            MemoryResource *SetDefaultMemoryResource(MemoryResource *mr) {
                return util::Exchange(std::addressof(default_mr), mr);
            }

            MemoryResource *GetCurrentMemoryResource() {
                EnsureCurrentMemoryResourceTlsSlotInitialized();
                return reinterpret_cast<MemoryResource *>(os::GetTlsValue(current_mr_tls_slot));
            }

            MemoryResource *SetCurrentMemoryResource(MemoryResource *mr) {
                EnsureCurrentMemoryResourceTlsSlotInitialized();
                auto ret = reinterpret_cast<MemoryResource *>(os::GetTlsValue(current_mr_tls_slot));
                os::SetTlsValue(current_mr_tls_slot, reinterpret_cast<uintptr_t>(mr));
                return ret;
            }

            MemoryResource *GetCurrentEffectiveMemoryResourceImpl() {
                if (auto mr = GetCurrentMemoryResource(); mr != nullptr) {
                    return mr;
                }
                if (auto mr = GetGlobalDefaultMemoryResource(); mr != nullptr) {
                    return mr;
                }
                return nullptr;
            }
        };

        constinit DefaultAllocatorImpl g_default_allocator_impl = {};

        inline void *DefaultAllocate(size_t size, size_t align) {
            AMS_UNUSED(align);
            return ::operator new(size, std::nothrow);
        }

        inline void DefaultDeallocate(void *ptr, size_t size, size_t align) {
            AMS_UNUSED(size, align);
            return ::operator delete(ptr, std::nothrow);
        }

        class NewDeleteMemoryResource final : public MemoryResource {
            private:
                virtual void *AllocateImpl(size_t size, size_t alignment) override {
                    return DefaultAllocate(size, alignment);
                }

                virtual void DeallocateImpl(void *buffer, size_t size, size_t alignment) override {
                    return DefaultDeallocate(buffer, size, alignment);
                }

                virtual bool IsEqualImpl(const MemoryResource &resource) const {
                    return this == std::addressof(resource);
                }
        };

        constinit NewDeleteMemoryResource g_new_delete_memory_resource;

    }

    namespace impl {

        void *DefaultAllocateImpl(size_t size, size_t align, size_t offset) {
            auto mr = g_default_allocator_impl.GetCurrentEffectiveMemoryResourceImpl();
            auto h = mr != nullptr ? mr->allocate(size, align) : DefaultAllocate(size, align);
            if (h == nullptr) {
                return nullptr;
            }

            *static_cast<MemoryResource **>(h) = mr;
            return static_cast<u8 *>(h) + offset;
        }

        void DefaultDeallocateImpl(void *ptr, size_t size, size_t align, size_t offset) {
            if (ptr == nullptr) {
                return;
            }
            auto h = static_cast<u8 *>(ptr) - offset;
            if (auto mr = *reinterpret_cast<MemoryResource **>(h); mr != nullptr) {
                return mr->deallocate(h, size, align);
            } else {
                return DefaultDeallocate(h, size, align);
            }
        }

    }

    MemoryResource *GetGlobalDefaultMemoryResource() {
        return g_default_allocator_impl.GetDefaultMemoryResource();
    }

    MemoryResource *GetCurrentEffectiveMemoryResource() {
        if (auto mr = g_default_allocator_impl.GetCurrentEffectiveMemoryResourceImpl(); mr != nullptr) {
            return mr;
        }
        return GetNewDeleteMemoryResource();
    }

    MemoryResource *GetCurrentMemoryResource() {
        return g_default_allocator_impl.GetCurrentMemoryResource();
    }

    MemoryResource *GetNewDeleteMemoryResource() {
        return std::addressof(g_new_delete_memory_resource);
    }

    MemoryResource *SetGlobalDefaultMemoryResource(MemoryResource *mr) {
        return g_default_allocator_impl.SetDefaultMemoryResource(mr);
    }

    MemoryResource *SetCurrentMemoryResource(MemoryResource *mr) {
        return g_default_allocator_impl.SetCurrentMemoryResource(mr);
    }

    ScopedCurrentMemoryResourceSetter::ScopedCurrentMemoryResourceSetter(MemoryResource *mr) : m_prev(g_default_allocator_impl.GetCurrentMemoryResource()) {
        os::SetTlsValue(g_default_allocator_impl.current_mr_tls_slot, reinterpret_cast<uintptr_t>(mr));
    }

    ScopedCurrentMemoryResourceSetter::~ScopedCurrentMemoryResourceSetter() {
        os::SetTlsValue(g_default_allocator_impl.current_mr_tls_slot, reinterpret_cast<uintptr_t>(m_prev));
    }

}
