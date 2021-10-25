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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {

        class CacheOperation {
            public:
                virtual void Operate(void *address, size_t size) const = 0;
        };

        Result DoProcessCacheOperation(const CacheOperation &operation, KProcessPageTable &page_table, uintptr_t address, size_t size) {
            /* Determine aligned extents. */
            const uintptr_t aligned_start = util::AlignDown(address, PageSize);
            const uintptr_t aligned_end   = util::AlignUp(address + size, PageSize);

            /* Iterate over and operate on contiguous ranges. */
            uintptr_t cur_address = aligned_start;
            size_t remaining = size;
            while (remaining > 0) {
                /* Get a contiguous range to operate on. */
                KPageTableBase::MemoryRange contig_range = { .address = Null<KPhysicalAddress>, .size = 0 };
                R_TRY(page_table.OpenMemoryRangeForProcessCacheOperation(std::addressof(contig_range), cur_address, aligned_end - cur_address));

                /* Close the range when we're done operating on it. */
                ON_SCOPE_EXIT { contig_range.Close(); };

                /* Adjust to remain within range. */
                KVirtualAddress operate_address = KMemoryLayout::GetLinearVirtualAddress(contig_range.address);
                size_t operate_size             = contig_range.size;
                if (cur_address < address) {
                    operate_address += (address - cur_address);
                    operate_size    -= (address - cur_address);
                }
                if (operate_size > remaining) {
                    operate_size = remaining;
                }

                /* Operate. */
                operation.Operate(GetVoidPointer(operate_address), operate_size);

                /* Advance. */
                cur_address += contig_range.size;
                remaining   -= operate_size;
            }
            MESOSPHERE_ASSERT(remaining == 0);

            return ResultSuccess();
        }

        void FlushEntireDataCache() {
            /* Flushing cache takes up to 1ms, so determine our minimum end tick. */
            const s64 timeout = KHardwareTimer::GetTick() + ams::svc::Tick(TimeSpan::FromMilliSeconds(1));

            /* Flush the entire data cache. */
            cpu::FlushEntireDataCache();

            /* Wait for 1ms to have passed. */
            while (KHardwareTimer::GetTick() < timeout) {
                cpu::Yield();
            }
        }

        Result FlushDataCache(uintptr_t address, size_t size) {
            /* Succeed if there's nothing to do. */
            R_SUCCEED_IF(size == 0);

            /* Validate that the region is within range. */
            R_UNLESS(GetCurrentProcess().GetPageTable().Contains(address, size), svc::ResultInvalidCurrentMemory());

            /* Flush the cache. */
            R_TRY(cpu::FlushDataCache(reinterpret_cast<void *>(address), size));

            return ResultSuccess();
        }

        Result InvalidateProcessDataCache(ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
            /* Validate address/size. */
            R_UNLESS(size > 0,                                   svc::ResultInvalidSize());
            R_UNLESS(address == static_cast<uintptr_t>(address), svc::ResultInvalidCurrentMemory());
            R_UNLESS(size == static_cast<size_t>(size),          svc::ResultInvalidCurrentMemory());

            /* Get the process from its handle. */
            KScopedAutoObject process = GetCurrentProcess().GetHandleTable().GetObject<KProcess>(process_handle);
            R_UNLESS(process.IsNotNull(), svc::ResultInvalidHandle());

            /* Invalidate the cache. */
            R_TRY(process->GetPageTable().InvalidateProcessDataCache(address, size));

            return ResultSuccess();
        }

        Result StoreProcessDataCache(ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
            /* Validate address/size. */
            R_UNLESS(size > 0,                                   svc::ResultInvalidSize());
            R_UNLESS(address == static_cast<uintptr_t>(address), svc::ResultInvalidCurrentMemory());
            R_UNLESS(size == static_cast<size_t>(size),          svc::ResultInvalidCurrentMemory());

            /* Get the process from its handle. */
            KScopedAutoObject process = GetCurrentProcess().GetHandleTable().GetObject<KProcess>(process_handle);
            R_UNLESS(process.IsNotNull(), svc::ResultInvalidHandle());

            /* Verify the region is within range. */
            auto &page_table = process->GetPageTable();
            R_UNLESS(page_table.Contains(address, size), svc::ResultInvalidCurrentMemory());

            /* Perform the operation. */
            if (process.GetPointerUnsafe() == GetCurrentProcessPointer()) {
                return cpu::StoreDataCache(reinterpret_cast<void *>(address), size);
            } else {
                class StoreCacheOperation : public CacheOperation {
                    public:
                        virtual void Operate(void *address, size_t size) const override { cpu::StoreDataCache(address, size); }
                } operation;

                return DoProcessCacheOperation(operation, page_table, address, size);
            }
        }

        Result FlushProcessDataCache(ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
            /* Validate address/size. */
            R_UNLESS(size > 0,                                   svc::ResultInvalidSize());
            R_UNLESS(address == static_cast<uintptr_t>(address), svc::ResultInvalidCurrentMemory());
            R_UNLESS(size == static_cast<size_t>(size),          svc::ResultInvalidCurrentMemory());

            /* Get the process from its handle. */
            KScopedAutoObject process = GetCurrentProcess().GetHandleTable().GetObject<KProcess>(process_handle);
            R_UNLESS(process.IsNotNull(), svc::ResultInvalidHandle());

            /* Verify the region is within range. */
            auto &page_table = process->GetPageTable();
            R_UNLESS(page_table.Contains(address, size), svc::ResultInvalidCurrentMemory());

            /* Perform the operation. */
            if (process.GetPointerUnsafe() == GetCurrentProcessPointer()) {
                return cpu::FlushDataCache(reinterpret_cast<void *>(address), size);
            } else {
                class FlushCacheOperation : public CacheOperation {
                    public:
                        virtual void Operate(void *address, size_t size) const override { cpu::FlushDataCache(address, size); }
                } operation;

                return DoProcessCacheOperation(operation, page_table, address, size);
            }
        }

    }

    /* =============================    64 ABI    ============================= */

    void FlushEntireDataCache64() {
        return FlushEntireDataCache();
    }

    Result FlushDataCache64(ams::svc::Address address, ams::svc::Size size) {
        return FlushDataCache(address, size);
    }

    Result InvalidateProcessDataCache64(ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
        return InvalidateProcessDataCache(process_handle, address, size);
    }

    Result StoreProcessDataCache64(ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
        return StoreProcessDataCache(process_handle, address, size);
    }

    Result FlushProcessDataCache64(ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
        return FlushProcessDataCache(process_handle, address, size);
    }

    /* ============================= 64From32 ABI ============================= */

    void FlushEntireDataCache64From32() {
        return FlushEntireDataCache();
    }

    Result FlushDataCache64From32(ams::svc::Address address, ams::svc::Size size) {
        return FlushDataCache(address, size);
    }

    Result InvalidateProcessDataCache64From32(ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
        return InvalidateProcessDataCache(process_handle, address, size);
    }

    Result StoreProcessDataCache64From32(ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
        return StoreProcessDataCache(process_handle, address, size);
    }

    Result FlushProcessDataCache64From32(ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
        return FlushProcessDataCache(process_handle, address, size);
    }

}
