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
#include <exosphere.hpp>
#include "se_execute.hpp"

namespace ams::se {

    namespace {

        struct LinkedListEntry {
            u32 zero;
            u32 address;
            u32 size;
        };
        static_assert(util::is_pod<LinkedListEntry>::value);
        static_assert(sizeof(LinkedListEntry) == 0xC);

        uintptr_t GetPhysicalAddress(const void *ptr) {
            const uintptr_t virt_address = reinterpret_cast<uintptr_t>(ptr);

            #if   defined(ATMOSPHERE_ARCH_ARM64)
                u64 phys_address;
                __asm__ __volatile__("at s1e3r, %[virt]; mrs %[phys], par_el1" : [phys]"=r"(phys_address) : [virt]"r"(virt_address) : "memory", "cc");
                return (phys_address & 0x0000FFFFFFFFF000ul) | (virt_address & 0x0000000000000FFFul);
            #elif defined(ATMOSPHERE_ARCH_ARM)
                return virt_address;
            #else
                #error "Unknown architecture for Tegra Security Engine physical address translation"
            #endif
        }

        constexpr void SetLinkedListEntry(LinkedListEntry *entry, const void *ptr, size_t size) {
            /* Clear the zero field. */
            entry->zero = 0;

            /* Set the address. */
            if (ptr != nullptr) {
                entry->address = GetPhysicalAddress(ptr);
                entry->size    = static_cast<u32>(size);
            } else {
                entry->address = 0;
                entry->size    = 0;
            }
        }

        void StartOperation(volatile SecurityEngineRegisters *SE, SE_OPERATION_OP op) {
            /* Write back the current values of the error and interrupt status. */
            reg::Write(SE->SE_ERR_STATUS, reg::Read(SE->SE_ERR_STATUS));
            reg::Write(SE->SE_INT_STATUS, reg::Read(SE->SE_INT_STATUS));

            /* Write the operation. */
            reg::Write(SE->SE_OPERATION, SE_REG_BITS_VALUE(OPERATION_OP, op));
        }

        void EnsureOperationStarted(volatile SecurityEngineRegisters *SE) {
            /* Read the operation register to make sure our write takes. */
            reg::Read(SE->SE_OPERATION);

            hw::DataSynchronizationBarrierInnerShareable();
        }

        void WaitForOperationComplete(volatile SecurityEngineRegisters *SE) {
            /* Spin until the operation is done. */
            while (reg::HasValue(SE->SE_INT_STATUS, SE_REG_BITS_ENUM(INT_STATUS_SE_OP_DONE, CLEAR))) { /* ... */ }

            /* Check for operation success. */
            ValidateAesOperationResult(SE);
        }

    }

    void ExecuteOperation(volatile SecurityEngineRegisters *SE, SE_OPERATION_OP op, void *dst, size_t dst_size, const void *src, size_t src_size) {
        /* Set the linked list entries. */
        LinkedListEntry src_entry;
        LinkedListEntry dst_entry;

        SetLinkedListEntry(std::addressof(src_entry), src, src_size);
        SetLinkedListEntry(std::addressof(dst_entry), dst, dst_size);

        /* Ensure the linked list entry data is seen correctly. */
        hw::FlushDataCache(std::addressof(src_entry), sizeof(src_entry));
        hw::FlushDataCache(std::addressof(dst_entry), sizeof(dst_entry));
        hw::DataSynchronizationBarrierInnerShareable();

        /* Configure the linked list addresses. */
        reg::Write(SE->SE_IN_LL_ADDR,  static_cast<u32>(GetPhysicalAddress(std::addressof(src_entry))));
        reg::Write(SE->SE_OUT_LL_ADDR, static_cast<u32>(GetPhysicalAddress(std::addressof(dst_entry))));

        /* Start the operation. */
        StartOperation(SE, op);

        /* Wait for the operation to complete. */
        WaitForOperationComplete(SE);
    }

    void ExecuteOperationSingleBlock(volatile SecurityEngineRegisters *SE, void *dst, size_t dst_size, const void *src, size_t src_size) {
        /* Validate sizes. */
        AMS_ABORT_UNLESS(dst_size <= AesBlockSize);
        AMS_ABORT_UNLESS(src_size <= AesBlockSize);

        /* Set the block count to 1. */
        reg::Write(SE->SE_CRYPTO_LAST_BLOCK, 0);

        /* Create an aligned buffer. */
        util::AlignedBuffer<hw::DataCacheLineSize, AesBlockSize> aligned;
        std::memcpy(aligned, src, src_size);
        hw::FlushDataCache(aligned, AesBlockSize);
        hw::DataSynchronizationBarrierInnerShareable();

        /* Execute the operation. */
        ExecuteOperation(SE, SE_OPERATION_OP_START, aligned, AesBlockSize, aligned, AesBlockSize);

        /* Ensure that the CPU will see the correct output. */
        hw::DataSynchronizationBarrierInnerShareable();
        hw::FlushDataCache(aligned, AesBlockSize);
        hw::DataSynchronizationBarrierInnerShareable();

        /* Copy the output to the destination. */
        std::memcpy(dst, aligned, dst_size);
    }

    void StartInputOperation(volatile SecurityEngineRegisters *SE, const void *src, size_t src_size) {
        /* Set the linked list entry. */
        LinkedListEntry src_entry;
        SetLinkedListEntry(std::addressof(src_entry), src, src_size);

        /* Ensure the linked list entry data is seen correctly. */
        hw::FlushDataCache(std::addressof(src_entry), sizeof(src_entry));
        hw::DataSynchronizationBarrierInnerShareable();

        /* Configure the linked list addresses. */
        reg::Write(SE->SE_IN_LL_ADDR,  static_cast<u32>(GetPhysicalAddress(std::addressof(src_entry))));

        /* Start the operation. */
        StartOperation(SE, SE_OPERATION_OP_START);

        /* Ensure the operation is started. */
        EnsureOperationStarted(SE);
    }

    void StartOperationRaw(volatile SecurityEngineRegisters *SE, SE_OPERATION_OP op, u32 out_ll_address, u32 in_ll_address) {
        /* Configure the linked list addresses. */
        reg::Write(SE->SE_IN_LL_ADDR,  in_ll_address);
        reg::Write(SE->SE_OUT_LL_ADDR, out_ll_address);

        /* Start the operation. */
        StartOperation(SE, op);

        /* Ensure the operation is started. */
        EnsureOperationStarted(SE);
    }

    void ValidateAesOperationResult(volatile SecurityEngineRegisters *SE) {
        /* Ensure no error occurred. */
        AMS_ABORT_UNLESS(reg::HasValue(SE->SE_INT_STATUS, SE_REG_BITS_ENUM(INT_STATUS_ERR_STAT, CLEAR)));

        /* Ensure the security engine is idle. */
        AMS_ABORT_UNLESS(reg::HasValue(SE->SE_STATUS, SE_REG_BITS_ENUM(STATUS_STATE, IDLE)));

        /* Ensure there is no error status. */
        AMS_ABORT_UNLESS(reg::Read(SE->SE_ERR_STATUS) == 0);
    }

    void ValidateAesOperationResult() {
        return ValidateAesOperationResult(GetRegisters());
    }

}
