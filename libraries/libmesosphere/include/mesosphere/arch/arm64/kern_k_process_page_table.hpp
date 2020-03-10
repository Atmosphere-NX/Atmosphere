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
#include <mesosphere/arch/arm64/kern_k_page_table.hpp>

namespace ams::kern::arch::arm64 {

    class KProcessPageTable {
        private:
            KPageTable page_table;
        public:
            constexpr KProcessPageTable() : page_table() { /* ... */ }

            void Activate(u64 id) {
                /* Activate the page table with the specified contextidr. */
                this->page_table.Activate(id);
            }

            Result Initialize(u32 id, ams::svc::CreateProcessFlag as_type, bool enable_aslr, bool from_back, KMemoryManager::Pool pool, KProcessAddress code_address, size_t code_size, KMemoryBlockSlabManager *mem_block_slab_manager, KBlockInfoManager *block_info_manager, KPageTableManager *pt_manager) {
                return this->page_table.InitializeForProcess(id, as_type, enable_aslr, from_back, pool, code_address, code_size, mem_block_slab_manager, block_info_manager, pt_manager);
            }

            void Finalize() { this->page_table.Finalize(); }

            Result SetMemoryPermission(KProcessAddress addr, size_t size, ams::svc::MemoryPermission perm) {
                return this->page_table.SetMemoryPermission(addr, size, perm);
            }

            Result SetProcessMemoryPermission(KProcessAddress addr, size_t size, ams::svc::MemoryPermission perm) {
                return this->page_table.SetProcessMemoryPermission(addr, size, perm);
            }

            Result SetHeapSize(KProcessAddress *out, size_t size) {
                return this->page_table.SetHeapSize(out, size);
            }

            Result SetMaxHeapSize(size_t size) {
                return this->page_table.SetMaxHeapSize(size);
            }

            Result QueryInfo(KMemoryInfo *out_info, ams::svc::PageInfo *out_page_info, KProcessAddress addr) const {
                return this->page_table.QueryInfo(out_info, out_page_info, addr);
            }

            Result MapIo(KPhysicalAddress phys_addr, size_t size, KMemoryPermission perm) {
                return this->page_table.MapIo(phys_addr, size, perm);
            }

            Result MapStatic(KPhysicalAddress phys_addr, size_t size, KMemoryPermission perm) {
                return this->page_table.MapStatic(phys_addr, size, perm);
            }

            Result MapRegion(KMemoryRegionType region_type, KMemoryPermission perm) {
                return this->page_table.MapRegion(region_type, perm);
            }

            Result MapPageGroup(KProcessAddress addr, const KPageGroup &pg, KMemoryState state, KMemoryPermission perm) {
                return this->page_table.MapPageGroup(addr, pg, state, perm);
            }

            Result MapPages(KProcessAddress *out_addr, size_t num_pages, size_t alignment, KPhysicalAddress phys_addr, KMemoryState state, KMemoryPermission perm) {
                return this->page_table.MapPages(out_addr, num_pages, alignment, phys_addr, state, perm);
            }

            Result MapPages(KProcessAddress *out_addr, size_t num_pages, KMemoryState state, KMemoryPermission perm) {
                return this->page_table.MapPages(out_addr, num_pages, state, perm);
            }

            Result UnmapPages(KProcessAddress addr, size_t num_pages, KMemoryState state) {
                return this->page_table.UnmapPages(addr, num_pages, state);
            }

            Result MakeAndOpenPageGroup(KPageGroup *out, KProcessAddress address, size_t num_pages, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) {
                return this->page_table.MakeAndOpenPageGroup(out, address, num_pages, state_mask, state, perm_mask, perm, attr_mask, attr);
            }

            bool GetPhysicalAddress(KPhysicalAddress *out, KProcessAddress address) const {
                return this->page_table.GetPhysicalAddress(out, address);
            }

            bool Contains(KProcessAddress addr, size_t size) const { return this->page_table.Contains(addr, size); }
            bool CanContain(KProcessAddress addr, size_t size, KMemoryState state) const { return this->page_table.CanContain(addr, size, state); }

            KProcessAddress GetAddressSpaceStart()    const { return this->page_table.GetAddressSpaceStart(); }
            KProcessAddress GetHeapRegionStart()      const { return this->page_table.GetHeapRegionStart(); }
            KProcessAddress GetAliasRegionStart()     const { return this->page_table.GetAliasRegionStart(); }
            KProcessAddress GetStackRegionStart()     const { return this->page_table.GetStackRegionStart(); }
            KProcessAddress GetKernelMapRegionStart() const { return this->page_table.GetKernelMapRegionStart(); }
            KProcessAddress GetAliasCodeRegionStart() const { return this->page_table.GetAliasCodeRegionStart(); }

            size_t GetAddressSpaceSize()    const { return this->page_table.GetAddressSpaceSize(); }
            size_t GetHeapRegionSize()      const { return this->page_table.GetHeapRegionSize(); }
            size_t GetAliasRegionSize()     const { return this->page_table.GetAliasRegionSize(); }
            size_t GetStackRegionSize()     const { return this->page_table.GetStackRegionSize(); }
            size_t GetKernelMapRegionSize() const { return this->page_table.GetKernelMapRegionSize(); }
            size_t GetAliasCodeRegionSize() const { return this->page_table.GetAliasCodeRegionSize(); }

            KPhysicalAddress GetHeapPhysicalAddress(KVirtualAddress address) const {
                /* TODO: Better way to convert address type? */
                return this->page_table.GetHeapPhysicalAddress(address);
            }

            KBlockInfoManager *GetBlockInfoManager() {
                return this->page_table.GetBlockInfoManager();
            }
    };

}
