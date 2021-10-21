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
#include "kern_secure_monitor.hpp"

namespace ams::kern::board::qemu::virt {

    namespace {

        constexpr uintptr_t DramPhysicalAddress = 0x40000000;
        constexpr size_t SecureAlignment        = 128_KB;

        /* Global variables for secure memory. */
        constexpr size_t SecureAppletMemorySize = 4_MB;
        constinit KSpinLock g_secure_applet_lock;
        constinit bool g_secure_applet_memory_used = false;
        constinit KVirtualAddress g_secure_applet_memory_address = Null<KVirtualAddress>;

        constinit KSpinLock g_secure_region_lock;
        constinit bool g_secure_region_used = false;
        constinit KPhysicalAddress g_secure_region_phys_addr = Null<KPhysicalAddress>;
        constinit size_t g_secure_region_size = 0;

        /* Global variables for randomness. */
        constinit bool         g_initialized_random_generator;
        constinit util::TinyMT g_random_generator;
        constinit KSpinLock    g_random_lock;

        ALWAYS_INLINE u64 GenerateRandomU64FromGenerator() {
            return g_random_generator.GenerateRandomU64();
        }

        template<typename F>
        ALWAYS_INLINE u64 GenerateUniformRange(u64 min, u64 max, F f) {
            /* Handle the case where the difference is too large to represent. */
            if (max == std::numeric_limits<u64>::max() && min == std::numeric_limits<u64>::min()) {
                return f();
            }

            /* Iterate until we get a value in range. */
            const u64 range_size    = ((max + 1) - min);
            const u64 effective_max = (std::numeric_limits<u64>::max() / range_size) * range_size;
            while (true) {
                if (const u64 rnd = f(); rnd < effective_max) {
                    return min + (rnd % range_size);
                }
            }
        }


        /* TODO */

        ALWAYS_INLINE size_t GetRealMemorySizeForInit() {
            return 4_GB;
        }

        bool SetSecureRegion(KPhysicalAddress phys_addr, size_t size) {
            /* Ensure address and size are aligned. */
            if (!util::IsAligned(GetInteger(phys_addr), SecureAlignment)) {
                return false;
            }
            if (!util::IsAligned(size, SecureAlignment)) {
                return false;
            }

            /* Disable interrupts and acquire the secure region lock. */
            KScopedInterruptDisable di;
            KScopedSpinLock lk(g_secure_region_lock);

            /* If size is non-zero, we're allocating the secure region. Otherwise, we're freeing it. */
            if (size != 0) {
                /* Verify that the secure region is free. */
                if (g_secure_region_used) {
                    return false;
                }

                /* Set the secure region. */
                g_secure_region_used      = true;
                g_secure_region_phys_addr = phys_addr;
                g_secure_region_size      = size;
            } else {
                /* Verify that the secure region is in use. */
                if (!g_secure_region_used) {
                    return false;
                }

                /* Verify that the address being freed is the secure region. */
                if (phys_addr != g_secure_region_phys_addr) {
                    return false;
                }

                /* Clear the secure region. */
                g_secure_region_used      = false;
                g_secure_region_phys_addr = Null<KPhysicalAddress>;
                g_secure_region_size      = 0;
            }

            // /* Configure the carveout with the secure monitor. */
            // smc::ConfigureCarveout(1, GetInteger(phys_addr), size);

            return true;
        }

        Result AllocateSecureMemoryForApplet(KVirtualAddress *out, size_t size) {
            /* Verify that the size is valid. */
            R_UNLESS(util::IsAligned(size, PageSize), svc::ResultInvalidSize());
            R_UNLESS(size <= SecureAppletMemorySize,  svc::ResultOutOfMemory());

            /* Disable interrupts and acquire the secure applet lock. */
            KScopedInterruptDisable di;
            KScopedSpinLock lk(g_secure_applet_lock);

            /* Check that memory is reserved for secure applet use. */
            MESOSPHERE_ABORT_UNLESS(g_secure_applet_memory_address != Null<KVirtualAddress>);

            /* Verify that the secure applet memory isn't already being used. */
            R_UNLESS(!g_secure_applet_memory_used, svc::ResultOutOfMemory());

            /* Return the secure applet memory. */
            g_secure_applet_memory_used = true;
            *out = g_secure_applet_memory_address;

            return ResultSuccess();
        }

        void FreeSecureMemoryForApplet(KVirtualAddress address, size_t size) {
            /* Disable interrupts and acquire the secure applet lock. */
            KScopedInterruptDisable di;
            KScopedSpinLock lk(g_secure_applet_lock);

            /* Verify that the memory being freed is correct. */
            MESOSPHERE_ABORT_UNLESS(address == g_secure_applet_memory_address);
            MESOSPHERE_ABORT_UNLESS(size <= SecureAppletMemorySize);
            MESOSPHERE_ABORT_UNLESS(util::IsAligned(size, PageSize));
            MESOSPHERE_ABORT_UNLESS(g_secure_applet_memory_used);

            /* Release the secure applet memory. */
            g_secure_applet_memory_used = false;
        }

        void EnsureRandomGeneratorSeeded() {
            if (AMS_UNLIKELY(!g_initialized_random_generator)) {
                u64 seed = UINT64_C(0xF5F5F5F5F5F5F5F5);
                g_random_generator.Initialize(reinterpret_cast<u32*>(std::addressof(seed)), sizeof(seed) / sizeof(u32));
                g_initialized_random_generator = true;
            }
        }

    }

    /* Initialization. */
    size_t KSystemControl::Init::GetIntendedMemorySize() {
        return 4_GB;
    }

    KPhysicalAddress KSystemControl::Init::GetKernelPhysicalBaseAddress(uintptr_t base_address) {
        const size_t real_dram_size     = GetRealMemorySizeForInit();
        const size_t intended_dram_size = KSystemControl::Init::GetIntendedMemorySize();
        if (intended_dram_size * 2 < real_dram_size) {
            return base_address;
        } else {
            return base_address + ((real_dram_size - intended_dram_size) / 2);
        }
    }

    void KSystemControl::Init::GetInitialProcessBinaryLayout(InitialProcessBinaryLayout *out) {
        *out = {
            .address = GetInteger(GetKernelPhysicalBaseAddress(DramPhysicalAddress)) + GetIntendedMemorySize() - KTraceBufferSize - InitialProcessBinarySizeMax,
            ._08     = 0,
        };
    }


    bool KSystemControl::Init::ShouldIncreaseThreadResourceLimit() {
        return true;
    }


    size_t KSystemControl::Init::GetApplicationPoolSize() {
        /* Get the base pool size. */
        const size_t base_pool_size = 3285_MB;

        /* Return (possibly) adjusted size. */
        return base_pool_size;
    }

    size_t KSystemControl::Init::GetAppletPoolSize() {
        /* Get the base pool size. */
        const size_t base_pool_size = 507_MB;

        /* Return (possibly) adjusted size. */
        constexpr size_t ExtraSystemMemoryForAtmosphere = 40_MB;
        return base_pool_size - ExtraSystemMemoryForAtmosphere - KTraceBufferSize;
    }

    size_t KSystemControl::Init::GetMinimumNonSecureSystemPoolSize() {
        return 0x29C8000;
    }

    void KSystemControl::Init::CpuOn(u64 core_id, uintptr_t entrypoint, uintptr_t arg) {
        smc::init::CpuOn(core_id, entrypoint, arg);
    }

    /* Randomness for Initialization. */
    void KSystemControl::Init::GenerateRandomBytes(void *dst, size_t size) {
        EnsureRandomGeneratorSeeded();

        u8 *dst_8 = static_cast<u8 *>(dst);
        while (size > 0) {
            const u64 random = GenerateRandomU64FromGenerator();
            std::memcpy(dst_8, std::addressof(random), std::min(size, sizeof(u64)));
            size -= std::min(size, sizeof(u64));
        }
    }

    u64 KSystemControl::Init::GenerateRandomRange(u64 min, u64 max) {
        EnsureRandomGeneratorSeeded();

        return GenerateUniformRange(min, max, GenerateRandomU64FromGenerator);
    }

    /* System Initialization. */
    void KSystemControl::InitializePhase1() {
        /* Set IsDebugMode. */
        {
            KTargetSystem::SetIsDebugMode(true);

            /* If debug mode, we want to initialize uart logging. */
            KTargetSystem::EnableDebugLogging(true);
            KDebugLog::Initialize();
        }

        /* Set Kernel Configuration. */
        {
            KTargetSystem::EnableDebugMemoryFill(false);
            KTargetSystem::EnableUserExceptionHandlers(true);
            KTargetSystem::EnableDynamicResourceLimits(true);
            KTargetSystem::EnableUserPmuAccess(false);
        }

        /* Set Kernel Debugging. */
        {
            /* NOTE: This is used to restrict access to SvcKernelDebug/SvcChangeKernelTraceState. */
            /* Mesosphere may wish to not require this, as we'd ideally keep ProgramVerification enabled for userland. */
            KTargetSystem::EnableKernelDebugging(true);
        }

        /* System ResourceLimit initialization. */
        {
            /* Construct the resource limit object. */
            KResourceLimit &sys_res_limit = Kernel::GetSystemResourceLimit();
            KAutoObject::Create(std::addressof(sys_res_limit));
            sys_res_limit.Initialize();

            /* Set the initial limits. */
            const auto [total_memory_size, kernel_memory_size] = KMemoryLayout::GetTotalAndKernelMemorySizes();
            const auto &slab_counts = init::GetSlabResourceCounts();
            MESOSPHERE_R_ABORT_UNLESS(sys_res_limit.SetLimitValue(ams::svc::LimitableResource_PhysicalMemoryMax,      total_memory_size));
            MESOSPHERE_R_ABORT_UNLESS(sys_res_limit.SetLimitValue(ams::svc::LimitableResource_ThreadCountMax,         slab_counts.num_KThread));
            MESOSPHERE_R_ABORT_UNLESS(sys_res_limit.SetLimitValue(ams::svc::LimitableResource_EventCountMax,          slab_counts.num_KEvent));
            MESOSPHERE_R_ABORT_UNLESS(sys_res_limit.SetLimitValue(ams::svc::LimitableResource_TransferMemoryCountMax, slab_counts.num_KTransferMemory));
            MESOSPHERE_R_ABORT_UNLESS(sys_res_limit.SetLimitValue(ams::svc::LimitableResource_SessionCountMax,        slab_counts.num_KSession));

            /* Reserve system memory. */
            MESOSPHERE_ABORT_UNLESS(sys_res_limit.Reserve(ams::svc::LimitableResource_PhysicalMemoryMax, kernel_memory_size));
        }
    }

    void KSystemControl::InitializePhase2() {
        /* Reserve secure applet memory. */
        if (GetTargetFirmware() >= TargetFirmware_5_0_0) {
            MESOSPHERE_ABORT_UNLESS(g_secure_applet_memory_address == Null<KVirtualAddress>);
            MESOSPHERE_ABORT_UNLESS(Kernel::GetSystemResourceLimit().Reserve(ams::svc::LimitableResource_PhysicalMemoryMax, SecureAppletMemorySize));

            constexpr auto SecureAppletAllocateOption = KMemoryManager::EncodeOption(KMemoryManager::Pool_System, KMemoryManager::Direction_FromFront);
            const KPhysicalAddress secure_applet_memory_phys_addr = Kernel::GetMemoryManager().AllocateAndOpenContinuous(SecureAppletMemorySize / PageSize, 1, SecureAppletAllocateOption);
            MESOSPHERE_ABORT_UNLESS(secure_applet_memory_phys_addr != Null<KPhysicalAddress>);

            g_secure_applet_memory_address = KMemoryLayout::GetLinearVirtualAddress(secure_applet_memory_phys_addr);
        }

        /* Initialize KTrace. */
        if constexpr (IsKTraceEnabled) {
            const auto &ktrace = KMemoryLayout::GetKernelTraceBufferRegion();
            KTrace::Initialize(ktrace.GetAddress(), ktrace.GetSize());
        }
    }

    u32 KSystemControl::GetCreateProcessMemoryPool() {
        return KMemoryManager::Pool_Unsafe;
    }

    /* Privileged Access. */
    void KSystemControl::ReadWriteRegisterPrivileged(u32 *out, ams::svc::PhysicalAddress address, u32 mask, u32 value) {
        MESOSPHERE_UNUSED(out, address, mask, value);
        MESOSPHERE_UNIMPLEMENTED();
    }

    Result KSystemControl::ReadWriteRegister(u32 *out, ams::svc::PhysicalAddress address, u32 mask, u32 value) {
        MESOSPHERE_UNUSED(out, address, mask, value);
        MESOSPHERE_UNIMPLEMENTED();
    }

    /* Randomness. */
    void KSystemControl::GenerateRandomBytes(void *dst, size_t size) {
        KScopedInterruptDisable intr_disable;
        KScopedSpinLock lk(g_random_lock);

        u8 *dst_8 = static_cast<u8 *>(dst);
        while (size > 0) {
            const u64 random = GenerateRandomU64FromGenerator();
            std::memcpy(dst_8, std::addressof(random), std::min(size, sizeof(u64)));
            size -= std::min(size, sizeof(u64));
        }
    }

    u64 KSystemControl::GenerateRandomRange(u64 min, u64 max) {
        KScopedInterruptDisable intr_disable;
        KScopedSpinLock lk(g_random_lock);

        return GenerateUniformRange(min, max, GenerateRandomU64FromGenerator);
    }

    u64 KSystemControl::GenerateRandomU64() {
        KScopedInterruptDisable intr_disable;
        KScopedSpinLock lk(g_random_lock);

        return GenerateRandomU64FromGenerator();
    }

    void KSystemControl::SleepSystem() {
        MESOSPHERE_LOG("SleepSystem() was called\n");
        MESOSPHERE_UNIMPLEMENTED();
    }

    void KSystemControl::StopSystem(void *arg) {
        MESOSPHERE_UNUSED(arg);
        AMS_INFINITE_LOOP();
    }

    /* User access. */
    void KSystemControl::CallSecureMonitorFromUser(ams::svc::lp64::SecureMonitorArguments *args) {
        /* Get the function id for the current call. */
        u64 function_id = args->r[0];

        /* We'll need to map in pages if arguments are pointers. Prepare page groups to do so. */
        auto &page_table = GetCurrentProcess().GetPageTable();
        auto *bim = page_table.GetBlockInfoManager();

        constexpr size_t MaxMappedRegisters = 7;
        std::array<KPageGroup, MaxMappedRegisters> page_groups = { KPageGroup(bim), KPageGroup(bim), KPageGroup(bim), KPageGroup(bim), KPageGroup(bim), KPageGroup(bim), KPageGroup(bim), };

        for (size_t i = 0; i < MaxMappedRegisters; i++) {
            const size_t reg_id = i + 1;
            if (function_id & (1ul << (8 + reg_id))) {
                /* Create and open a new page group for the address. */
                KVirtualAddress virt_addr = args->r[reg_id];

                if (R_SUCCEEDED(page_table.MakeAndOpenPageGroup(std::addressof(page_groups[i]), util::AlignDown(GetInteger(virt_addr), PageSize), 1, KMemoryState_None, KMemoryState_None, KMemoryPermission_UserReadWrite, KMemoryPermission_UserReadWrite, KMemoryAttribute_None, KMemoryAttribute_None))) {
                    /* Translate the virtual address to a physical address. */
                    const auto it = page_groups[i].begin();
                    MESOSPHERE_ASSERT(it != page_groups[i].end());
                    MESOSPHERE_ASSERT(it->GetNumPages() == 1);

                    args->r[reg_id] = GetInteger(it->GetAddress()) | (GetInteger(virt_addr) & (PageSize - 1));
                } else {
                    /* If we couldn't map, we should clear the address. */
                    args->r[reg_id] = 0;
                }
            }
        }

        /* Invoke the secure monitor. */
        smc::CallSecureMonitorFromUser(args);

        /* Make sure that we close any pages that we opened. */
        for (size_t i = 0; i < MaxMappedRegisters; i++) {
            page_groups[i].Close();
        }
    }

    /* Secure Memory. */
    size_t KSystemControl::CalculateRequiredSecureMemorySize(size_t size, u32 pool) {
        if (pool == KMemoryManager::Pool_Applet) {
            return 0;
        }
        return size;
    }

    Result KSystemControl::AllocateSecureMemory(KVirtualAddress *out, size_t size, u32 pool) {
        /* Applet secure memory is handled separately. */
        if (pool == KMemoryManager::Pool_Applet) {
            return AllocateSecureMemoryForApplet(out, size);
        }

        /* Ensure the size is aligned. */
        const size_t alignment = (pool == KMemoryManager::Pool_System ? PageSize : SecureAlignment);
        R_UNLESS(util::IsAligned(size, alignment), svc::ResultInvalidSize());

        /* Allocate the memory. */
        const size_t num_pages = size / PageSize;
        const KPhysicalAddress paddr = Kernel::GetMemoryManager().AllocateAndOpenContinuous(num_pages, alignment / PageSize, KMemoryManager::EncodeOption(static_cast<KMemoryManager::Pool>(pool), KMemoryManager::Direction_FromFront));
        R_UNLESS(paddr != Null<KPhysicalAddress>, svc::ResultOutOfMemory());

        /* Ensure we don't leak references to the memory on error. */
        auto mem_guard = SCOPE_GUARD { Kernel::GetMemoryManager().Close(paddr, num_pages); };

        /* If the memory isn't already secure, set it as secure. */
        if (pool != KMemoryManager::Pool_System) {
            /* Set the secure region. */
            R_UNLESS(SetSecureRegion(paddr, size), svc::ResultOutOfMemory());
        }

        /* We succeeded. */
        mem_guard.Cancel();
        *out = KPageTable::GetHeapVirtualAddress(paddr);
        return ResultSuccess();
    }

    void KSystemControl::FreeSecureMemory(KVirtualAddress address, size_t size, u32 pool) {
        /* Applet secure memory is handled separately. */
        if (pool == KMemoryManager::Pool_Applet) {
            return FreeSecureMemoryForApplet(address, size);
        }

        /* Ensure the size is aligned. */
        const size_t alignment = (pool == KMemoryManager::Pool_System ? PageSize : SecureAlignment);
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(GetInteger(address), alignment));
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(size, alignment));

        /* If the memory isn't secure system, reset the secure region. */
        if (pool != KMemoryManager::Pool_System) {
            /* Check that the size being freed is the current secure region size. */
            MESOSPHERE_ABORT_UNLESS(g_secure_region_size == size);

            /* Get the physical address. */
            const KPhysicalAddress paddr = KPageTable::GetHeapPhysicalAddress(address);
            MESOSPHERE_ABORT_UNLESS(paddr != Null<KPhysicalAddress>);

            /* Check that the memory being freed is the current secure region. */
            MESOSPHERE_ABORT_UNLESS(paddr == g_secure_region_phys_addr);

            /* Free the secure region. */
            MESOSPHERE_ABORT_UNLESS(SetSecureRegion(paddr, 0));
        }

        /* Close the secure region's pages. */
        Kernel::GetMemoryManager().Close(KPageTable::GetHeapPhysicalAddress(address), size / PageSize);
    }

}