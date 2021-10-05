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
#include <stratosphere.hpp>

namespace ams::ro::impl {

    constexpr inline auto RetrySearchCount = 512;

    Result SearchFreeRegion(uintptr_t *out, size_t mapping_size);

    class ProcessRegionInfo {
        NON_COPYABLE(ProcessRegionInfo);
        NON_MOVEABLE(ProcessRegionInfo);
        private:
            static constexpr size_t StackGuardSize = 4 * os::MemoryPageSize;
        private:
            u64 m_heap_start;
            u64 m_heap_size;
            u64 m_alias_start;
            u64 m_alias_size;
            u64 m_aslr_start;
            u64 m_aslr_size;
        public:
            ProcessRegionInfo(os::NativeHandle process) {
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(m_heap_start),  svc::InfoType_HeapRegionAddress,  process, 0));
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(m_heap_size),   svc::InfoType_HeapRegionSize,     process, 0));
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(m_alias_start), svc::InfoType_AliasRegionAddress, process, 0));
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(m_alias_size),  svc::InfoType_AliasRegionSize,    process, 0));
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(m_aslr_start),  svc::InfoType_AslrRegionAddress,  process, 0));
                R_ABORT_UNLESS(svc::GetInfo(std::addressof(m_aslr_size),   svc::InfoType_AslrRegionSize,     process, 0));
            }

            u64 GetAslrRegion(u64 mapping_size) const {
                /* If we can, look for a region. */
                if (mapping_size <= m_aslr_size) {
                    for (auto i = 0; i < RetrySearchCount; ++i) {
                        /* Get a random address. */
                        const u64 address = m_aslr_start + os::GenerateRandomU64((m_aslr_size - mapping_size) / os::MemoryPageSize) * os::MemoryPageSize;

                        /* Check that it's not contained within heap. */
                        if (m_heap_size != 0 && !(address + mapping_size - 1 < m_heap_start || m_heap_start + m_heap_size - 1 < address)) {
                            continue;
                        }

                        /* Check that it's not contained within alias. */
                        if (m_alias_size != 0 && !(address + mapping_size - 1 < m_alias_start || m_alias_start + m_alias_size - 1 < address)) {
                            continue;
                        }

                        /* Return the address. */
                        return address;
                    }
                }

                /* We failed to find a region. */
                return 0;
            }

            bool CanEmplaceGuardSpaces(os::NativeHandle process, u64 address, u64 size) {
                /* NOTE: Nintendo does not check the results of these svc calls. */
                svc::MemoryInfo mem_info;
                svc::PageInfo page_info;

                /* Check for guard availability before the region. */
                R_ABORT_UNLESS(svc::QueryProcessMemory(std::addressof(mem_info), std::addressof(page_info), process, address - 1));
                if (!(mem_info.state == svc::MemoryState_Free && mem_info.base_address <= address - StackGuardSize)) {
                    return false;
                }

                /* Check for guard availability after the region. */
                R_ABORT_UNLESS(svc::QueryProcessMemory(std::addressof(mem_info), std::addressof(page_info), process, address + size));
                if (!(mem_info.state == svc::MemoryState_Free && address + size + StackGuardSize <= mem_info.base_address + mem_info.size)) {
                    return false;
                }

                return true;
            }
    };

    class AutoCloseMap {
        private:
            Result m_result;
            uintptr_t m_map_address;
            os::NativeHandle m_handle;
            u64 m_address;
            u64 m_size;
        public:
            AutoCloseMap(uintptr_t map, os::NativeHandle handle, u64 addr, u64 size) : m_map_address(map), m_handle(handle), m_address(addr), m_size(size) {
                m_result = svc::MapProcessMemory(m_map_address, m_handle, m_address, m_size);
            }

            ~AutoCloseMap() {
                if (m_handle != os::InvalidNativeHandle && R_SUCCEEDED(m_result)) {
                    R_ABORT_UNLESS(svc::UnmapProcessMemory(m_map_address, m_handle, m_address, m_size));
                }
            }

            Result GetResult() const {
                return m_result;
            }

            bool IsSuccess() const {
                return R_SUCCEEDED(m_result);
            }

            void Cancel() {
                m_handle = os::InvalidNativeHandle;
            }
    };

    class MappedCodeMemory {
        NON_COPYABLE(MappedCodeMemory);
        private:
            os::NativeHandle m_handle;
            Result m_result;
            u64 m_dst_address;
            u64 m_src_address;
            u64 m_size;
        public:
            constexpr MappedCodeMemory() : m_handle(os::InvalidNativeHandle), m_result(ro::ResultInternalError()), m_dst_address(0), m_src_address(0), m_size(0) {
                /* ... */
            }

            MappedCodeMemory(os::NativeHandle handle, u64 dst, u64 src, u64 size) : m_handle(handle), m_dst_address(dst), m_src_address(src), m_size(size) {
                m_result = svc::MapProcessCodeMemory(m_handle, m_dst_address, m_src_address, m_size);
            }

            ~MappedCodeMemory() {
                if (m_handle != os::InvalidNativeHandle && R_SUCCEEDED(m_result) && m_size > 0) {
                    R_ABORT_UNLESS(svc::UnmapProcessCodeMemory(m_handle, m_dst_address, m_src_address, m_size));
                }
            }

            MappedCodeMemory(MappedCodeMemory &&rhs) : m_handle(rhs.m_handle), m_result(rhs.m_result), m_dst_address(rhs.m_dst_address), m_src_address(rhs.m_src_address), m_size(rhs.m_size) {
                rhs.m_handle = os::InvalidNativeHandle;
            }

            MappedCodeMemory &operator=(MappedCodeMemory &&rhs) {
                m_handle      = rhs.m_handle;
                m_result      = rhs.m_result;
                m_dst_address = rhs.m_dst_address;
                m_src_address = rhs.m_src_address;
                m_size        = rhs.m_size;

                rhs.m_handle  = os::InvalidNativeHandle;

                return *this;
            }

            Result GetResult() const {
                return m_result;
            }

            bool IsSuccess() const {
                return R_SUCCEEDED(m_result);
            }

            void Cancel() {
                m_handle = os::InvalidNativeHandle;
            }
    };

}
