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

        constexpr size_t SecureAlignment = 128_KB;

        /* Global variables for panic. */
        constinit bool g_call_smc_on_panic;

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
        /* Nintendo uses std::mt19937_t for randomness. */
        /* To save space (and because mt19337_t isn't secure anyway), */
        /* We will use TinyMT. */
        bool         g_initialized_random_generator;
        util::TinyMT g_random_generator;
        constinit KSpinLock    g_random_lock;

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

        ALWAYS_INLINE bool CheckRegisterAllowedTable(const u8 *table, const size_t offset) {
            return (table[(offset / sizeof(u32)) / BITSIZEOF(u8)] & (1u << ((offset / sizeof(u32)) % BITSIZEOF(u8)))) != 0;
        }

        /* TODO: Generate this from a list of register names (see similar logic in exosphere)? */
        constexpr inline const u8 McKernelRegisterWhitelist[(PageSize / sizeof(u32)) / BITSIZEOF(u8)] = {
            0x9F, 0x31, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0xC0, 0x73, 0x3E, 0x6F, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0xE4, 0xFF, 0xFF, 0x01,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        };

        /* TODO: Generate this from a list of register names (see similar logic in exosphere)? */
        constexpr inline const u8 McUserRegisterWhitelist[(PageSize / sizeof(u32)) / BITSIZEOF(u8)] = {
            0x00, 0x00, 0x20, 0x00, 0xF0, 0xFF, 0xF7, 0x01,
            0xCD, 0xFE, 0xC0, 0xFE, 0x00, 0x00, 0x00, 0x00,
            0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6E,
            0x30, 0x05, 0x06, 0xB0, 0x71, 0xC8, 0x43, 0x04,
            0x80, 0xFF, 0x08, 0x80, 0x03, 0x38, 0x8E, 0x1F,
            0xC8, 0xFF, 0xFF, 0x00, 0x0E, 0x00, 0x00, 0x00,
            0xF0, 0x1F, 0x00, 0x30, 0xF0, 0x03, 0x03, 0x30,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0xFE, 0x0F,
            0x01, 0x00, 0x80, 0x00, 0x00, 0x08, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        };

        bool IsRegisterAccessibleToPrivileged(ams::svc::PhysicalAddress address) {
            /* Find the region for the address. */
            KMemoryRegionTree::const_iterator it = KMemoryLayout::FindContainingRegion(KPhysicalAddress(address));
            if (AMS_LIKELY(it != KMemoryLayout::GetPhysicalMemoryRegionTree().end())) {
                if (AMS_LIKELY(it->IsDerivedFrom(KMemoryRegionAttr_NoUserMap | KMemoryRegionType_MemoryController))) {
                    /* Get the offset within the region. */
                    const size_t offset = address - it->GetAddress();
                    MESOSPHERE_ABORT_UNLESS(offset < it->GetSize());

                    /* Check the whitelist. */
                    if (AMS_LIKELY(CheckRegisterAllowedTable(McKernelRegisterWhitelist, offset))) {
                        return true;
                    }
                }
            }

            return false;
        }

        bool IsRegisterAccessibleToUser(ams::svc::PhysicalAddress address) {
            /* Find the region for the address. */
            KMemoryRegionTree::const_iterator it = KMemoryLayout::FindContainingRegion(KPhysicalAddress(address));
            if (AMS_LIKELY(it != KMemoryLayout::GetPhysicalMemoryRegionTree().end())) {
                /* The PMC is always allowed. */
                if (it->IsDerivedFrom(KMemoryRegionAttr_NoUserMap | KMemoryRegionType_PowerManagementController)) {
                    return true;
                }

                /* Memory controller is allowed if the register is whitelisted. */
                if (it->IsDerivedFrom(KMemoryRegionAttr_NoUserMap | KMemoryRegionType_MemoryController ) ||
                    it->IsDerivedFrom(KMemoryRegionAttr_NoUserMap | KMemoryRegionType_MemoryController0) ||
                    it->IsDerivedFrom(KMemoryRegionAttr_NoUserMap | KMemoryRegionType_MemoryController1))
                {
                    /* Get the offset within the region. */
                    const size_t offset = address - it->GetAddress();
                    MESOSPHERE_ABORT_UNLESS(offset < it->GetSize());

                    /* Check the whitelist. */
                    if (AMS_LIKELY(CheckRegisterAllowedTable(McUserRegisterWhitelist, offset))) {
                        return true;
                    }
                }
            }

            return false;
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

            /* Configure the carveout with the secure monitor. */
            smc::ConfigureCarveout(1, GetInteger(phys_addr), size);

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
        /* Get the base pool size. */
        const size_t base_pool_size = [] ALWAYS_INLINE_LAMBDA () -> size_t {
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
        }();

        /* Return (possibly) adjusted size. */
        return base_pool_size;
    }

    size_t KSystemControl::Init::GetAppletPoolSize() {
        /* Get the base pool size. */
        const size_t base_pool_size = [] ALWAYS_INLINE_LAMBDA () -> size_t {
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
        }();

        /* Return (possibly) adjusted size. */
        constexpr size_t ExtraSystemMemoryForAtmosphere = 33_MB;
        return base_pool_size - ExtraSystemMemoryForAtmosphere - KTraceBufferSize;
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
            MESOSPHERE_ABORT_UNLESS(Kernel::GetSystemResourceLimit().Reserve(ams::svc::LimitableResource_PhysicalMemoryMax, SecureAppletMemorySize));

            constexpr auto SecureAppletAllocateOption = KMemoryManager::EncodeOption(KMemoryManager::Pool_System, KMemoryManager::Direction_FromFront);
            g_secure_applet_memory_address = Kernel::GetMemoryManager().AllocateContinuous(SecureAppletMemorySize / PageSize, 1, SecureAppletAllocateOption);
            MESOSPHERE_ABORT_UNLESS(g_secure_applet_memory_address != Null<KVirtualAddress>);
        }

        /* Initialize KTrace. */
        if constexpr (IsKTraceEnabled) {
            const auto &ktrace = KMemoryLayout::GetKernelTraceBufferRegion();
            KTrace::Initialize(ktrace.GetAddress(), ktrace.GetSize());
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

    Result KSystemControl::ReadWriteRegister(u32 *out, ams::svc::PhysicalAddress address, u32 mask, u32 value) {
        R_UNLESS(AMS_LIKELY(util::IsAligned(address, sizeof(u32))),             svc::ResultInvalidAddress());
        R_UNLESS(AMS_LIKELY(IsRegisterAccessibleToUser(address)),               svc::ResultInvalidAddress());
        R_UNLESS(AMS_LIKELY(smc::ReadWriteRegister(out, address, mask, value)), svc::ResultInvalidAddress());
        return ResultSuccess();
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
        u32 dummy;
        smc::init::ReadWriteRegister(std::addressof(dummy), 0x7000E400, 0x10, 0x10);
        while (true) { /* ... */ }
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

                    KPhysicalAddress phys_addr = page_table.GetHeapPhysicalAddress(it->GetAddress());

                    args->r[reg_id] = GetInteger(phys_addr) | (GetInteger(virt_addr) & (PageSize - 1));
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
        const KVirtualAddress vaddr = Kernel::GetMemoryManager().AllocateContinuous(num_pages, alignment / PageSize, KMemoryManager::EncodeOption(static_cast<KMemoryManager::Pool>(pool), KMemoryManager::Direction_FromFront));
        R_UNLESS(vaddr != Null<KVirtualAddress>, svc::ResultOutOfMemory());

        /* Open a reference to the memory. */
        Kernel::GetMemoryManager().Open(vaddr, num_pages);

        /* Ensure we don't leak references to the memory on error. */
        auto mem_guard = SCOPE_GUARD { Kernel::GetMemoryManager().Close(vaddr, num_pages); };

        /* If the memory isn't already secure, set it as secure. */
        if (pool != KMemoryManager::Pool_System) {
            /* Get the physical address. */
            const KPhysicalAddress paddr = KPageTable::GetHeapPhysicalAddress(vaddr);
            MESOSPHERE_ABORT_UNLESS(paddr != Null<KPhysicalAddress>);

            /* Set the secure region. */
            R_UNLESS(SetSecureRegion(paddr, size), svc::ResultOutOfMemory());
        }

        /* We succeeded. */
        mem_guard.Cancel();
        *out = vaddr;
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
        Kernel::GetMemoryManager().Close(address, size / PageSize);
    }

}