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
#include <mesosphere/kern_slab_helpers.hpp>
#include <mesosphere/kern_k_memory_manager.hpp>
#include <mesosphere/kern_k_dynamic_resource_manager.hpp>
#include <mesosphere/kern_k_page_table_manager.hpp>
#include <mesosphere/kern_k_resource_limit.hpp>

namespace ams::kern {

    /* NOTE: Nintendo's implementation does not have the "is_secure_resource" field, and instead uses virtual IsSecureResource(). */

    class KSystemResource : public KAutoObject {
        MESOSPHERE_AUTOOBJECT_TRAITS(KSystemResource, KAutoObject);
        private:
            KMemoryBlockSlabManager *m_p_memory_block_slab_manager{};
            KBlockInfoManager       *m_p_block_info_manager{};
            KPageTableManager       *m_p_page_table_manager{};
            bool m_is_secure_resource{false};
        public:
            explicit KSystemResource() : KAutoObject() { /* ... */ }

            constexpr explicit KSystemResource(util::ConstantInitializeTag) : KAutoObject(util::ConstantInitialize) { /* ... */ }
        protected:
            ALWAYS_INLINE void SetSecureResource() { m_is_secure_resource = true; }
        public:
            virtual void Destroy() override { MESOSPHERE_PANIC("KSystemResource::Destroy() was called"); }

            ALWAYS_INLINE bool IsSecureResource() const { return m_is_secure_resource; }

            void SetManagers(KMemoryBlockSlabManager &mb, KBlockInfoManager &bi, KPageTableManager &pt) {
                MESOSPHERE_ASSERT(m_p_memory_block_slab_manager == nullptr);
                MESOSPHERE_ASSERT(m_p_block_info_manager == nullptr);
                MESOSPHERE_ASSERT(m_p_page_table_manager == nullptr);

                m_p_memory_block_slab_manager = std::addressof(mb);
                m_p_block_info_manager        = std::addressof(bi);
                m_p_page_table_manager        = std::addressof(pt);
            }

            const KMemoryBlockSlabManager &GetMemoryBlockSlabManager() const { return *m_p_memory_block_slab_manager; }
            const KBlockInfoManager &GetBlockInfoManager() const { return *m_p_block_info_manager; }
            const KPageTableManager &GetPageTableManager() const { return *m_p_page_table_manager; }

            KMemoryBlockSlabManager &GetMemoryBlockSlabManager() { return *m_p_memory_block_slab_manager; }
            KBlockInfoManager &GetBlockInfoManager() { return *m_p_block_info_manager; }
            KPageTableManager &GetPageTableManager() { return *m_p_page_table_manager; }

            KMemoryBlockSlabManager *GetMemoryBlockSlabManagerPointer() { return m_p_memory_block_slab_manager; }
            KBlockInfoManager *GetBlockInfoManagerPointer() { return m_p_block_info_manager; }
            KPageTableManager *GetPageTableManagerPointer() { return m_p_page_table_manager; }
    };

    class KSecureSystemResource final : public KAutoObjectWithSlabHeap<KSecureSystemResource, KSystemResource> {
        private:
            bool                        m_is_initialized;
            KMemoryManager::Pool        m_resource_pool;
            KDynamicPageManager         m_dynamic_page_manager;
            KMemoryBlockSlabManager     m_memory_block_slab_manager;
            KBlockInfoManager           m_block_info_manager;
            KPageTableManager           m_page_table_manager;
            KMemoryBlockSlabHeap        m_memory_block_heap;
            KBlockInfoSlabHeap          m_block_info_heap;
            KPageTableSlabHeap          m_page_table_heap;
            KResourceLimit             *m_resource_limit;
            KVirtualAddress             m_resource_address;
            size_t                      m_resource_size;
        public:
            explicit KSecureSystemResource() : m_is_initialized(false), m_resource_limit(nullptr) {
                /* Mark ourselves as being a secure resource. */
                this->SetSecureResource();
            }

            Result Initialize(size_t size, KResourceLimit *resource_limit, KMemoryManager::Pool pool);
            void Finalize();

            bool IsInitialized() const { return m_is_initialized; }
            static void PostDestroy(uintptr_t arg) { MESOSPHERE_UNUSED(arg); /* ... */ }

            ALWAYS_INLINE size_t CalculateRequiredSecureMemorySize() const {
                if (m_resource_limit != nullptr) {
                    return CalculateRequiredSecureMemorySize(m_resource_size, m_resource_pool);
                } else {
                    return 0;
                }
            }

            ALWAYS_INLINE size_t GetSize() const { return m_resource_size; }
            ALWAYS_INLINE size_t GetUsedSize() const { return m_dynamic_page_manager.GetUsed() * PageSize; }

            const KDynamicPageManager &GetDynamicPageManager() const { return m_dynamic_page_manager; }
        public:
            static size_t CalculateRequiredSecureMemorySize(size_t size, KMemoryManager::Pool pool);
    };

}
