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
            KPageTable m_page_table;
        public:
            constexpr KProcessPageTable() : m_page_table() { /* ... */ }

            void Activate(u64 id) {
                /* Activate the page table with the specified contextidr. */
                m_page_table.Activate(id);
            }

            Result Initialize(u32 id, ams::svc::CreateProcessFlag as_type, bool enable_aslr, bool enable_das_merge, bool from_back, KMemoryManager::Pool pool, KProcessAddress code_address, size_t code_size, KMemoryBlockSlabManager *mem_block_slab_manager, KBlockInfoManager *block_info_manager, KPageTableManager *pt_manager) {
                return m_page_table.InitializeForProcess(id, as_type, enable_aslr, enable_das_merge, from_back, pool, code_address, code_size, mem_block_slab_manager, block_info_manager, pt_manager);
            }

            void Finalize() { m_page_table.Finalize(); }

            Result SetMemoryPermission(KProcessAddress addr, size_t size, ams::svc::MemoryPermission perm) {
                return m_page_table.SetMemoryPermission(addr, size, perm);
            }

            Result SetProcessMemoryPermission(KProcessAddress addr, size_t size, ams::svc::MemoryPermission perm) {
                return m_page_table.SetProcessMemoryPermission(addr, size, perm);
            }

            Result SetMemoryAttribute(KProcessAddress addr, size_t size, u32 mask, u32 attr) {
                return m_page_table.SetMemoryAttribute(addr, size, mask, attr);
            }

            Result SetHeapSize(KProcessAddress *out, size_t size) {
                return m_page_table.SetHeapSize(out, size);
            }

            Result SetMaxHeapSize(size_t size) {
                return m_page_table.SetMaxHeapSize(size);
            }

            Result QueryInfo(KMemoryInfo *out_info, ams::svc::PageInfo *out_page_info, KProcessAddress addr) const {
                return m_page_table.QueryInfo(out_info, out_page_info, addr);
            }

            Result QueryPhysicalAddress(ams::svc::PhysicalMemoryInfo *out, KProcessAddress address) const {
                return m_page_table.QueryPhysicalAddress(out, address);
            }

            Result QueryStaticMapping(KProcessAddress *out, KPhysicalAddress address, size_t size) const {
                return m_page_table.QueryStaticMapping(out, address, size);
            }

            Result QueryIoMapping(KProcessAddress *out, KPhysicalAddress address, size_t size) const {
                return m_page_table.QueryIoMapping(out, address, size);
            }

            Result MapMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size) {
                return m_page_table.MapMemory(dst_address, src_address, size);
            }

            Result UnmapMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size) {
                return m_page_table.UnmapMemory(dst_address, src_address, size);
            }

            Result MapCodeMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size) {
                return m_page_table.MapCodeMemory(dst_address, src_address, size);
            }

            Result UnmapCodeMemory(KProcessAddress dst_address, KProcessAddress src_address, size_t size) {
                return m_page_table.UnmapCodeMemory(dst_address, src_address, size);
            }

            Result MapIo(KPhysicalAddress phys_addr, size_t size, KMemoryPermission perm) {
                return m_page_table.MapIo(phys_addr, size, perm);
            }

            Result MapStatic(KPhysicalAddress phys_addr, size_t size, KMemoryPermission perm) {
                return m_page_table.MapStatic(phys_addr, size, perm);
            }

            Result MapRegion(KMemoryRegionType region_type, KMemoryPermission perm) {
                return m_page_table.MapRegion(region_type, perm);
            }

            Result MapPageGroup(KProcessAddress addr, const KPageGroup &pg, KMemoryState state, KMemoryPermission perm) {
                return m_page_table.MapPageGroup(addr, pg, state, perm);
            }

            Result UnmapPageGroup(KProcessAddress address, const KPageGroup &pg, KMemoryState state) {
                return m_page_table.UnmapPageGroup(address, pg, state);
            }

            Result MapPages(KProcessAddress *out_addr, size_t num_pages, size_t alignment, KPhysicalAddress phys_addr, KMemoryState state, KMemoryPermission perm) {
                return m_page_table.MapPages(out_addr, num_pages, alignment, phys_addr, state, perm);
            }

            Result MapPages(KProcessAddress *out_addr, size_t num_pages, KMemoryState state, KMemoryPermission perm) {
                return m_page_table.MapPages(out_addr, num_pages, state, perm);
            }

            Result MapPages(KProcessAddress address, size_t num_pages, KMemoryState state, KMemoryPermission perm) {
                return m_page_table.MapPages(address, num_pages, state, perm);
            }

            Result UnmapPages(KProcessAddress addr, size_t num_pages, KMemoryState state) {
                return m_page_table.UnmapPages(addr, num_pages, state);
            }

            Result MakeAndOpenPageGroup(KPageGroup *out, KProcessAddress address, size_t num_pages, u32 state_mask, u32 state, u32 perm_mask, u32 perm, u32 attr_mask, u32 attr) {
                return m_page_table.MakeAndOpenPageGroup(out, address, num_pages, state_mask, state, perm_mask, perm, attr_mask, attr);
            }

            Result InvalidateProcessDataCache(KProcessAddress address, size_t size) {
                return m_page_table.InvalidateProcessDataCache(address, size);
            }

            Result ReadDebugMemory(void *buffer, KProcessAddress address, size_t size) {
                return m_page_table.ReadDebugMemory(buffer, address, size);
            }

            Result WriteDebugMemory(KProcessAddress address, const void *buffer, size_t size) {
                return m_page_table.WriteDebugMemory(address, buffer, size);
            }

            Result LockForDeviceAddressSpace(KPageGroup *out, KProcessAddress address, size_t size, KMemoryPermission perm, bool is_aligned) {
                return m_page_table.LockForDeviceAddressSpace(out, address, size, perm, is_aligned);
            }

            Result UnlockForDeviceAddressSpace(KProcessAddress address, size_t size) {
                return m_page_table.UnlockForDeviceAddressSpace(address, size);
            }

            Result MakePageGroupForUnmapDeviceAddressSpace(KPageGroup *out, KProcessAddress address, size_t size) {
                return m_page_table.MakePageGroupForUnmapDeviceAddressSpace(out, address, size);
            }

            Result UnlockForDeviceAddressSpacePartialMap(KProcessAddress address, size_t size, size_t mapped_size) {
                return m_page_table.UnlockForDeviceAddressSpacePartialMap(address, size, mapped_size);
            }

            Result LockForIpcUserBuffer(KPhysicalAddress *out, KProcessAddress address, size_t size) {
                return m_page_table.LockForIpcUserBuffer(out, address, size);
            }

            Result UnlockForIpcUserBuffer(KProcessAddress address, size_t size) {
                return m_page_table.UnlockForIpcUserBuffer(address, size);
            }

            Result LockForTransferMemory(KPageGroup *out, KProcessAddress address, size_t size, KMemoryPermission perm) {
                return m_page_table.LockForTransferMemory(out, address, size, perm);
            }

            Result UnlockForTransferMemory(KProcessAddress address, size_t size, const KPageGroup &pg) {
                return m_page_table.UnlockForTransferMemory(address, size, pg);
            }

            Result LockForCodeMemory(KPageGroup *out, KProcessAddress address, size_t size) {
                return m_page_table.LockForCodeMemory(out, address, size);
            }

            Result UnlockForCodeMemory(KProcessAddress address, size_t size, const KPageGroup &pg) {
                return m_page_table.UnlockForCodeMemory(address, size, pg);
            }

            Result CopyMemoryFromLinearToUser(KProcessAddress dst_addr, size_t size, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr) {
                return m_page_table.CopyMemoryFromLinearToUser(dst_addr, size, src_addr, src_state_mask, src_state, src_test_perm, src_attr_mask, src_attr);
            }

            Result CopyMemoryFromLinearToKernel(KProcessAddress dst_addr, size_t size, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr) {
                return m_page_table.CopyMemoryFromLinearToKernel(dst_addr, size, src_addr, src_state_mask, src_state, src_test_perm, src_attr_mask, src_attr);
            }

            Result CopyMemoryFromUserToLinear(KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr) {
                return m_page_table.CopyMemoryFromUserToLinear(dst_addr, size, dst_state_mask, dst_state, dst_test_perm, dst_attr_mask, dst_attr, src_addr);
            }

            Result CopyMemoryFromKernelToLinear(KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr) {
                return m_page_table.CopyMemoryFromKernelToLinear(dst_addr, size, dst_state_mask, dst_state, dst_test_perm, dst_attr_mask, dst_attr, src_addr);
            }

            Result CopyMemoryFromHeapToHeap(KProcessPageTable &dst_page_table, KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr) {
                return m_page_table.CopyMemoryFromHeapToHeap(dst_page_table.m_page_table, dst_addr, size, dst_state_mask, dst_state, dst_test_perm, dst_attr_mask, dst_attr, src_addr, src_state_mask, src_state, src_test_perm, src_attr_mask, src_attr);
            }

            Result CopyMemoryFromHeapToHeapWithoutCheckDestination(KProcessPageTable &dst_page_table, KProcessAddress dst_addr, size_t size, u32 dst_state_mask, u32 dst_state, KMemoryPermission dst_test_perm, u32 dst_attr_mask, u32 dst_attr, KProcessAddress src_addr, u32 src_state_mask, u32 src_state, KMemoryPermission src_test_perm, u32 src_attr_mask, u32 src_attr) {
                return m_page_table.CopyMemoryFromHeapToHeapWithoutCheckDestination(dst_page_table.m_page_table, dst_addr, size, dst_state_mask, dst_state, dst_test_perm, dst_attr_mask, dst_attr, src_addr, src_state_mask, src_state, src_test_perm, src_attr_mask, src_attr);
            }

            Result SetupForIpc(KProcessAddress *out_dst_addr, size_t size, KProcessAddress src_addr, KProcessPageTable &src_page_table, KMemoryPermission test_perm, KMemoryState dst_state, bool send) {
                return m_page_table.SetupForIpc(out_dst_addr, size, src_addr, src_page_table.m_page_table, test_perm, dst_state, send);
            }

            Result CleanupForIpcServer(KProcessAddress address, size_t size, KMemoryState dst_state, KProcess *server_process) {
                return m_page_table.CleanupForIpcServer(address, size, dst_state, server_process);
            }

            Result CleanupForIpcClient(KProcessAddress address, size_t size, KMemoryState dst_state) {
                return m_page_table.CleanupForIpcClient(address, size, dst_state);
            }

            Result MapPhysicalMemory(KProcessAddress address, size_t size) {
                return m_page_table.MapPhysicalMemory(address, size);
            }

            Result UnmapPhysicalMemory(KProcessAddress address, size_t size) {
                return m_page_table.UnmapPhysicalMemory(address, size);
            }

            Result MapPhysicalMemoryUnsafe(KProcessAddress address, size_t size) {
                return m_page_table.MapPhysicalMemoryUnsafe(address, size);
            }

            Result UnmapPhysicalMemoryUnsafe(KProcessAddress address, size_t size) {
                return m_page_table.UnmapPhysicalMemoryUnsafe(address, size);
            }

            void DumpMemoryBlocks() const {
                return m_page_table.DumpMemoryBlocks();
            }

            void DumpPageTable() const {
                return m_page_table.DumpPageTable();
            }

            size_t CountPageTables() const {
                return m_page_table.CountPageTables();
            }

            bool GetPhysicalAddress(KPhysicalAddress *out, KProcessAddress address) const {
                return m_page_table.GetPhysicalAddress(out, address);
            }

            bool Contains(KProcessAddress addr, size_t size) const { return m_page_table.Contains(addr, size); }

            bool IsInAliasRegion(KProcessAddress addr, size_t size) const { return m_page_table.IsInAliasRegion(addr, size); }
            bool IsInUnsafeAliasRegion(KProcessAddress addr, size_t size) const { return m_page_table.IsInUnsafeAliasRegion(addr, size); }

            bool CanContain(KProcessAddress addr, size_t size, KMemoryState state) const { return m_page_table.CanContain(addr, size, state); }

            KProcessAddress GetAddressSpaceStart()    const { return m_page_table.GetAddressSpaceStart(); }
            KProcessAddress GetHeapRegionStart()      const { return m_page_table.GetHeapRegionStart(); }
            KProcessAddress GetAliasRegionStart()     const { return m_page_table.GetAliasRegionStart(); }
            KProcessAddress GetStackRegionStart()     const { return m_page_table.GetStackRegionStart(); }
            KProcessAddress GetKernelMapRegionStart() const { return m_page_table.GetKernelMapRegionStart(); }
            KProcessAddress GetAliasCodeRegionStart() const { return m_page_table.GetAliasCodeRegionStart(); }

            size_t GetAddressSpaceSize()    const { return m_page_table.GetAddressSpaceSize(); }
            size_t GetHeapRegionSize()      const { return m_page_table.GetHeapRegionSize(); }
            size_t GetAliasRegionSize()     const { return m_page_table.GetAliasRegionSize(); }
            size_t GetStackRegionSize()     const { return m_page_table.GetStackRegionSize(); }
            size_t GetKernelMapRegionSize() const { return m_page_table.GetKernelMapRegionSize(); }
            size_t GetAliasCodeRegionSize() const { return m_page_table.GetAliasCodeRegionSize(); }

            size_t GetNormalMemorySize() const { return m_page_table.GetNormalMemorySize(); }

            size_t GetCodeSize() const { return m_page_table.GetCodeSize(); }
            size_t GetCodeDataSize() const { return m_page_table.GetCodeDataSize(); }

            size_t GetAliasCodeSize() const { return m_page_table.GetAliasCodeSize(); }
            size_t GetAliasCodeDataSize() const { return m_page_table.GetAliasCodeDataSize(); }

            u32 GetAllocateOption() const { return m_page_table.GetAllocateOption(); }

            KPhysicalAddress GetHeapPhysicalAddress(KVirtualAddress address) const {
                return m_page_table.GetHeapPhysicalAddress(address);
            }

            KVirtualAddress GetHeapVirtualAddress(KPhysicalAddress address) const {
                return m_page_table.GetHeapVirtualAddress(address);
            }

            KBlockInfoManager *GetBlockInfoManager() {
                return m_page_table.GetBlockInfoManager();
            }
    };

}
