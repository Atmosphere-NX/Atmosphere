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
#include <vapours.hpp>
#include <mesosphere/arch/arm64/kern_cpu_system_registers.hpp>
#include <mesosphere/kern_select_userspace_memory_access.hpp>

namespace ams::kern::arch::arm64::cpu {

#if defined(ATMOSPHERE_CPU_ARM_CORTEX_A57) || defined(ATMOSPHERE_CPU_ARM_CORTEX_A53)
    constexpr inline size_t InstructionCacheLineSize = 0x40;
    constexpr inline size_t DataCacheLineSize        = 0x40;
    constexpr inline size_t NumPerformanceCounters   = 6;
#else
    #error "Unknown CPU for cache line sizes"
#endif

#if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
    constexpr inline size_t NumCores = 4;
#else
    #error "Unknown Board for cpu::NumCores"
#endif

    /* Initialization. */
    NOINLINE void InitializeInterruptThreads(s32 core_id);

    /* Helpers for managing memory state. */
    ALWAYS_INLINE void DataSynchronizationBarrier() {
        __asm__ __volatile__("dsb sy" ::: "memory");
    }

    ALWAYS_INLINE void DataSynchronizationBarrierInnerShareable() {
        __asm__ __volatile__("dsb ish" ::: "memory");
    }

    ALWAYS_INLINE void DataMemoryBarrier() {
        __asm__ __volatile__("dmb sy" ::: "memory");
    }

    ALWAYS_INLINE void InstructionMemoryBarrier() {
        __asm__ __volatile__("isb" ::: "memory");
    }

    ALWAYS_INLINE void EnsureInstructionConsistency() {
        DataSynchronizationBarrier();
        InstructionMemoryBarrier();
    }

    ALWAYS_INLINE void Yield() {
        __asm__ __volatile__("yield" ::: "memory");
    }

    ALWAYS_INLINE void SwitchProcess(u64 ttbr, u32 proc_id) {
        SetTtbr0El1(ttbr);
        ContextIdRegisterAccessor(0).SetProcId(proc_id).Store();
        InstructionMemoryBarrier();
    }

    /* Performance counter helpers. */
    ALWAYS_INLINE u64 GetCycleCounter() {
        return cpu::GetPmcCntrEl0();
    }

    ALWAYS_INLINE u32 GetPerformanceCounter(s32 n) {
        u64 counter = 0;
        if (n < static_cast<s32>(NumPerformanceCounters)) {
            switch (n) {
                case 0:
                    counter = cpu::GetPmevCntr0El0();
                    break;
                case 1:
                    counter = cpu::GetPmevCntr1El0();
                    break;
                case 2:
                    counter = cpu::GetPmevCntr2El0();
                    break;
                case 3:
                    counter = cpu::GetPmevCntr3El0();
                    break;
                case 4:
                    counter = cpu::GetPmevCntr4El0();
                    break;
                case 5:
                    counter = cpu::GetPmevCntr5El0();
                    break;
                default:
                    break;
            }
        }
        return static_cast<u32>(counter);
    }

    /* Helper for address access. */
    ALWAYS_INLINE bool GetPhysicalAddressWritable(KPhysicalAddress *out, KVirtualAddress addr, bool privileged = false) {
        const uintptr_t va = GetInteger(addr);

        if (privileged) {
            __asm__ __volatile__("at s1e1w, %[va]" :: [va]"r"(va) : "memory");
        } else {
            __asm__ __volatile__("at s1e0w, %[va]" :: [va]"r"(va) : "memory");
        }
        InstructionMemoryBarrier();

        u64 par = GetParEl1();

        if (par & 0x1) {
            return false;
        }

        if (out) {
            *out = KPhysicalAddress((par & 0xFFFFFFFFF000ull) | (va & 0xFFFull));
        }
        return true;
    }

    ALWAYS_INLINE bool GetPhysicalAddressReadable(KPhysicalAddress *out, KVirtualAddress addr, bool privileged = false) {
        const uintptr_t va = GetInteger(addr);

        if (privileged) {
            __asm__ __volatile__("at s1e1r, %[va]" :: [va]"r"(va) : "memory");
        } else {
            __asm__ __volatile__("at s1e0r, %[va]" :: [va]"r"(va) : "memory");
        }
        InstructionMemoryBarrier();

        u64 par = GetParEl1();

        if (par & 0x1) {
            return false;
        }

        if (out) {
            *out = KPhysicalAddress((par & 0xFFFFFFFFF000ull) | (va & 0xFFFull));
        }
        return true;
    }

    ALWAYS_INLINE bool CanAccessAtomic(KProcessAddress addr, bool privileged = false) {
        const uintptr_t va = GetInteger(addr);

        if (privileged) {
            __asm__ __volatile__("at s1e1w, %[va]" :: [va]"r"(va) : "memory");
        } else {
            __asm__ __volatile__("at s1e0w, %[va]" :: [va]"r"(va) : "memory");
        }
        InstructionMemoryBarrier();

        u64 par = GetParEl1();

        if (par & 0x1) {
            return false;
        }

        return (par >> (BITSIZEOF(par) - BITSIZEOF(u8))) == 0xFF;
    }

    /* Synchronization helpers. */
    NOINLINE void SynchronizeAllCores();

    /* Cache management helpers. */
    void ClearPageToZeroImpl(void *);
    void FlushEntireDataCacheSharedForInit();
    void FlushEntireDataCacheLocalForInit();
    void InvalidateEntireInstructionCacheForInit();
    void StoreEntireCacheForInit();

    void FlushEntireDataCache();

    Result InvalidateDataCache(void *addr, size_t size);
    Result StoreDataCache(const void *addr, size_t size);
    Result FlushDataCache(const void *addr, size_t size);
    Result InvalidateInstructionCache(void *addr, size_t size);

    void InvalidateEntireInstructionCache();

    ALWAYS_INLINE void ClearPageToZero(void *page) {
        MESOSPHERE_ASSERT(util::IsAligned(reinterpret_cast<uintptr_t>(page), PageSize));
        MESOSPHERE_ASSERT(page != nullptr);
        ClearPageToZeroImpl(page);
    }

    ALWAYS_INLINE void InvalidateTlbByAsid(u32 asid) {
        const u64 value = (static_cast<u64>(asid) << 48);
        __asm__ __volatile__("tlbi aside1is, %[value]" :: [value]"r"(value) : "memory");
        EnsureInstructionConsistency();
    }

    ALWAYS_INLINE void InvalidateTlbByAsidAndVa(u32 asid, KProcessAddress virt_addr) {
        const u64 value = (static_cast<u64>(asid) << 48) | ((GetInteger(virt_addr) >> 12) & 0xFFFFFFFFFFFul);
        __asm__ __volatile__("tlbi aside1is, %[value]" :: [value]"r"(value) : "memory");
        EnsureInstructionConsistency();
    }

    ALWAYS_INLINE void InvalidateEntireTlb() {
        __asm__ __volatile__("tlbi vmalle1is" ::: "memory");
        EnsureInstructionConsistency();
    }

    ALWAYS_INLINE void InvalidateEntireTlbDataOnly() {
        __asm__ __volatile__("tlbi vmalle1is" ::: "memory");
        DataSynchronizationBarrier();
    }

    ALWAYS_INLINE void InvalidateTlbByVaDataOnly(KProcessAddress virt_addr) {
        const u64 value = ((GetInteger(virt_addr) >> 12) & 0xFFFFFFFFFFFul);
        __asm__ __volatile__("tlbi vaae1is, %[value]" :: [value]"r"(value) : "memory");
        DataSynchronizationBarrier();
    }

    ALWAYS_INLINE uintptr_t GetCurrentThreadPointerValue() {
        register uintptr_t x18 asm("x18");
        __asm__ __volatile__("" : [x18]"=r"(x18));
        return x18;
    }

    ALWAYS_INLINE void SetCurrentThreadPointerValue(uintptr_t value) {
        register uintptr_t x18 asm("x18") = value;
        __asm__ __volatile__("":: [x18]"r"(x18));
    }

    ALWAYS_INLINE void SetExceptionThreadStackTop(uintptr_t top) {
        SetTpidrEl1(top);
    }

    ALWAYS_INLINE void SwitchThreadLocalRegion(uintptr_t tlr) {
        cpu::SetTpidrRoEl0(tlr);
    }

}
