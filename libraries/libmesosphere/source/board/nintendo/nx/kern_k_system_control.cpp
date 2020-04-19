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
#include <mesosphere.hpp>
#include "kern_secure_monitor.hpp"
#include "kern_k_sleep_manager.hpp"

namespace ams::kern::board::nintendo::nx {

    namespace {

        /* Global variables for panic. */
        bool g_call_smc_on_panic;

        /* Global variables for secure memory. */
        constexpr size_t SecureAppletReservedMemorySize = 4_MB;
        KVirtualAddress g_secure_applet_memory_address;


        /* Global variables for randomness. */
        /* Nintendo uses std::mt19937_t for randomness. */
        /* To save space (and because mt19337_t isn't secure anyway), */
        /* We will use TinyMT. */
        bool         g_initialized_random_generator;
        util::TinyMT g_random_generator;
        KSpinLock    g_random_lock;

        ALWAYS_INLINE size_t GetRealMemorySizeForInit() {
            /* TODO: Move this into a header for the MC in general. */
            constexpr u32 MemoryControllerConfigurationRegister = 0x70019050;
            u32 config_value;
            MESOSPHERE_INIT_ABORT_UNLESS(smc::init::ReadWriteRegister(&config_value, MemoryControllerConfigurationRegister, 0, 0));
            return static_cast<size_t>(config_value & 0x3FFF) << 20;
        }

        ALWAYS_INLINE util::BitPack32 GetKernelConfigurationForInit() {
            u64 value = 0;
            smc::init::GetConfig(&value, 1, smc::ConfigItem::KernelConfiguration);
            return util::BitPack32{static_cast<u32>(value)};
        }

        ALWAYS_INLINE u32 GetMemoryModeForInit() {
            u64 value = 0;
            smc::init::GetConfig(&value, 1, smc::ConfigItem::MemoryMode);
            return static_cast<u32>(value);
        }

        ALWAYS_INLINE smc::MemoryArrangement GetMemoryArrangeForInit() {
            switch(GetMemoryModeForInit() & 0x3F) {
                case 0x01:
                default:
                    return smc::MemoryArrangement_4GB;
                case 0x02:
                    return smc::MemoryArrangement_4GBForAppletDev;
                case 0x03:
                    return smc::MemoryArrangement_4GBForSystemDev;
                case 0x11:
                    return smc::MemoryArrangement_6GB;
                case 0x12:
                    return smc::MemoryArrangement_6GBForAppletDev;
                case 0x21:
                    return smc::MemoryArrangement_8GB;
            }
        }

        ALWAYS_INLINE u64 GenerateRandomU64ForInit() {
            u64 value;
            smc::init::GenerateRandomBytes(&value, sizeof(value));
            return value;
        }

        void EnsureRandomGeneratorInitialized() {
            if (AMS_UNLIKELY(!g_initialized_random_generator)) {
                u64 seed;
                smc::GenerateRandomBytes(&seed, sizeof(seed));
                g_random_generator.Initialize(reinterpret_cast<u32*>(&seed), sizeof(seed) / sizeof(u32));
                g_initialized_random_generator = true;
            }
        }

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

        ALWAYS_INLINE u64 GetConfigU64(smc::ConfigItem which) {
            u64 value;
            smc::GetConfig(&value, 1, which);
            return value;
        }

        ALWAYS_INLINE u32 GetConfigU32(smc::ConfigItem which) {
            return static_cast<u32>(GetConfigU64(which));
        }

        ALWAYS_INLINE bool GetConfigBool(smc::ConfigItem which) {
            return GetConfigU64(which) != 0;
        }

        bool IsRegisterAccessibleToPrivileged(ams::svc::PhysicalAddress address) {
            if (!KMemoryLayout::GetMemoryControllerRegion().Contains(address)) {
                return false;
            }
            /* TODO: Validate specific offsets. */
            return true;
        }

    }

    /* Initialization. */
    size_t KSystemControl::Init::GetIntendedMemorySize() {
        switch (GetKernelConfigurationForInit().Get<smc::KernelConfiguration::MemorySize>()) {
            case smc::MemorySize_4GB:
            default: /* All invalid modes should go to 4GB. */
                return 4_GB;
            case smc::MemorySize_6GB:
                return 6_GB;
            case smc::MemorySize_8GB:
                return 8_GB;
        }
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

    bool KSystemControl::Init::ShouldIncreaseThreadResourceLimit() {
        return GetKernelConfigurationForInit().Get<smc::KernelConfiguration::IncreaseThreadResourceLimit>();
    }

    size_t KSystemControl::Init::GetApplicationPoolSize() {
        switch (GetMemoryArrangeForInit()) {
            case smc::MemoryArrangement_4GB:
            default:
                return 3285_MB;
            case smc::MemoryArrangement_4GBForAppletDev:
                return 2048_MB;
            case smc::MemoryArrangement_4GBForSystemDev:
                return 3285_MB;
            case smc::MemoryArrangement_6GB:
                return 4916_MB;
            case smc::MemoryArrangement_6GBForAppletDev:
                return 3285_MB;
            case smc::MemoryArrangement_8GB:
                return 4916_MB;
        }
    }

    size_t KSystemControl::Init::GetAppletPoolSize() {
        switch (GetMemoryArrangeForInit()) {
            case smc::MemoryArrangement_4GB:
            default:
                return 507_MB;
            case smc::MemoryArrangement_4GBForAppletDev:
                return 1554_MB;
            case smc::MemoryArrangement_4GBForSystemDev:
                return 448_MB;
            case smc::MemoryArrangement_6GB:
                return 562_MB;
            case smc::MemoryArrangement_6GBForAppletDev:
                return 2193_MB;
            case smc::MemoryArrangement_8GB:
                return 2193_MB;
        }
    }

    size_t KSystemControl::Init::GetMinimumNonSecureSystemPoolSize() {
        /* TODO: Where does this constant actually come from? */
        return 0x29C8000;
    }

    void KSystemControl::Init::CpuOn(u64 core_id, uintptr_t entrypoint, uintptr_t arg) {
        smc::init::CpuOn(core_id, entrypoint, arg);
    }

    /* Randomness for Initialization. */
    void KSystemControl::Init::GenerateRandomBytes(void *dst, size_t size) {
        MESOSPHERE_INIT_ABORT_UNLESS(size <= 0x38);
        smc::init::GenerateRandomBytes(dst, size);
    }

    u64 KSystemControl::Init::GenerateRandomRange(u64 min, u64 max) {
        return GenerateUniformRange(min, max, GenerateRandomU64ForInit);
    }

    /* System Initialization. */
    void KSystemControl::InitializePhase1() {
        /* Set IsDebugMode. */
        {
            KTargetSystem::SetIsDebugMode(GetConfigBool(smc::ConfigItem::IsDebugMode));

            /* If debug mode, we want to initialize uart logging. */
            KTargetSystem::EnableDebugLogging(KTargetSystem::IsDebugMode());
            KDebugLog::Initialize();
        }

        /* Set Kernel Configuration. */
        {
            const auto kernel_config = util::BitPack32{GetConfigU32(smc::ConfigItem::KernelConfiguration)};

            KTargetSystem::EnableDebugMemoryFill(kernel_config.Get<smc::KernelConfiguration::DebugFillMemory>());
            KTargetSystem::EnableUserExceptionHandlers(kernel_config.Get<smc::KernelConfiguration::EnableUserExceptionHandlers>());
            KTargetSystem::EnableUserPmuAccess(kernel_config.Get<smc::KernelConfiguration::EnableUserPmuAccess>());

            g_call_smc_on_panic = kernel_config.Get<smc::KernelConfiguration::UseSecureMonitorPanicCall>();
        }

        /* Set Kernel Debugging. */
        {
            /* NOTE: This is used to restrict access to SvcKernelDebug/SvcChangeKernelTraceState. */
            /* Mesosphere may wish to not require this, as we'd ideally keep ProgramVerification enabled for userland. */
            KTargetSystem::EnableKernelDebugging(GetConfigBool(smc::ConfigItem::DisableProgramVerification));
        }

        /* Configure the Kernel Carveout region. */
        {
            const auto carveout = KMemoryLayout::GetCarveoutRegionExtents();
            smc::ConfigureCarveout(0, carveout.GetAddress(), carveout.GetSize());
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
        /* Initialize the sleep manager. */
        KSleepManager::Initialize();

        /* Reserve secure applet memory. */
        {
            MESOSPHERE_ABORT_UNLESS(g_secure_applet_memory_address == Null<KVirtualAddress>);
            MESOSPHERE_ABORT_UNLESS(Kernel::GetSystemResourceLimit().Reserve(ams::svc::LimitableResource_PhysicalMemoryMax, SecureAppletReservedMemorySize));

            constexpr auto SecureAppletAllocateOption = KMemoryManager::EncodeOption(KMemoryManager::Pool_System, KMemoryManager::Direction_FromFront);
            g_secure_applet_memory_address = Kernel::GetMemoryManager().AllocateContinuous(SecureAppletReservedMemorySize / PageSize, 1, SecureAppletAllocateOption);
            MESOSPHERE_ABORT_UNLESS(g_secure_applet_memory_address != Null<KVirtualAddress>);
        }
    }

    u32 KSystemControl::GetInitialProcessBinaryPool() {
        return KMemoryManager::Pool_Application;
    }

    /* Privileged Access. */
    void KSystemControl::ReadWriteRegisterPrivileged(u32 *out, ams::svc::PhysicalAddress address, u32 mask, u32 value) {
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(address, sizeof(u32)));
        MESOSPHERE_ABORT_UNLESS(IsRegisterAccessibleToPrivileged(address));
        MESOSPHERE_ABORT_UNLESS(smc::ReadWriteRegister(out, address, mask, value));
    }

    void KSystemControl::ReadWriteRegister(u32 *out, ams::svc::PhysicalAddress address, u32 mask, u32 value) {
        MESOSPHERE_UNIMPLEMENTED();
    }

    /* Randomness. */
    void KSystemControl::GenerateRandomBytes(void *dst, size_t size) {
        MESOSPHERE_INIT_ABORT_UNLESS(size <= 0x38);
        smc::GenerateRandomBytes(dst, size);
    }

    u64 KSystemControl::GenerateRandomRange(u64 min, u64 max) {
        KScopedInterruptDisable intr_disable;
        KScopedSpinLock lk(g_random_lock);

        EnsureRandomGeneratorInitialized();

        return GenerateUniformRange(min, max, GenerateRandomU64FromGenerator);
    }

    u64 KSystemControl::GenerateRandomU64() {
        KScopedInterruptDisable intr_disable;
        KScopedSpinLock lk(g_random_lock);

        EnsureRandomGeneratorInitialized();

        return GenerateRandomU64FromGenerator();
    }

    void KSystemControl::SleepSystem() {
        MESOSPHERE_LOG("SleepSystem() was called\n");
        KSleepManager::SleepSystem();
    }

    void KSystemControl::StopSystem() {
        if (g_call_smc_on_panic) {
            /* Display a panic screen via secure monitor. */
            smc::Panic(0xF00);
        }
        while (true) { /* ... */ }
    }

    /* User access. */
    void KSystemControl::CallSecureMonitorFromUser(ams::svc::lp64::SecureMonitorArguments *args) {
        /* Get the function id for the current call. */
        u64 function_id = args->r[0];

        MESOSPHERE_LOG("CallSecureMonitor(%lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx);\n", args->r[0], args->r[1], args->r[2], args->r[3], args->r[4], args->r[5], args->r[6], args->r[7]);

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

                    KPhysicalAddress phys_addr = page_table.GetHeapPhysicalAddress(it->GetAddress());

                    args->r[reg_id] = GetInteger(phys_addr) | (GetInteger(virt_addr) & (PageSize - 1));
                    MESOSPHERE_LOG("Mapped arg %zu\n", reg_id);
                } else {
                    /* If we couldn't map, we should clear the address. */
                    MESOSPHERE_LOG("Failed to map arg %zu\n", reg_id);
                    args->r[reg_id] = 0;
                }
            }
        }

        /* Invoke the secure monitor. */
        smc::CallSecureMonitorFromUser(args);

        MESOSPHERE_LOG("Secure Monitor Returned: (%lx, %lx, %lx, %lx, %lx, %lx, %lx, %lx);\n", args->r[0], args->r[1], args->r[2], args->r[3], args->r[4], args->r[5], args->r[6], args->r[7]);

        /* Make sure that we close any pages that we opened. */
        for (size_t i = 0; i < MaxMappedRegisters; i++) {
            page_groups[i].Close();
        }
    }

}