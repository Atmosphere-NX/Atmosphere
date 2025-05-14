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
#include "kern_k_sleep_manager.hpp"

namespace ams::kern::board::nintendo::nx {

    namespace {

        constexpr size_t SecureAlignment = 128_KB;

        constexpr size_t SecureSizeMax = util::AlignDown(512_MB - 1, SecureAlignment);

        /* Global variables for panic. */
        constinit const volatile bool g_call_smc_on_panic = false;

        /* Global variables for secure memory. */
        constinit KSpinLock g_secure_applet_lock;
        constinit bool g_secure_applet_memory_used = false;
        constinit KVirtualAddress g_secure_applet_memory_address = Null<KVirtualAddress>;

        constinit KSpinLock g_secure_region_lock;
        constinit bool g_secure_region_used = false;
        constinit KPhysicalAddress g_secure_region_phys_addr = Null<KPhysicalAddress>;
        constinit size_t g_secure_region_size = 0;

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
            smc::init::GenerateRandomBytes(std::addressof(value), sizeof(value));
            return value;
        }

        ALWAYS_INLINE u64 GenerateRandomU64FromSmc() {
            u64 value;
            smc::GenerateRandomBytes(std::addressof(value), sizeof(value));
            return value;
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
            const KMemoryRegion *region = KMemoryLayout::Find(KPhysicalAddress(address));
            if (AMS_LIKELY(region != nullptr)) {
                if (AMS_LIKELY(region->IsDerivedFrom(KMemoryRegionType_MemoryController))) {
                    /* Check the region is valid. */
                    MESOSPHERE_ABORT_UNLESS(region->GetEndAddress() != 0);

                    /* Get the offset within the region. */
                    const size_t offset = address - region->GetAddress();
                    MESOSPHERE_ABORT_UNLESS(offset < region->GetSize());

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
            const KMemoryRegion *region = KMemoryLayout::Find(KPhysicalAddress(address));
            if (AMS_LIKELY(region != nullptr)) {
                /* The PMC is always allowed. */
                if (region->IsDerivedFrom(KMemoryRegionType_PowerManagementController)) {
                    return true;
                }

                /* Memory controller is allowed if the register is whitelisted. */
                if (region->IsDerivedFrom(KMemoryRegionType_MemoryController ) ||
                    region->IsDerivedFrom(KMemoryRegionType_MemoryController0) ||
                    region->IsDerivedFrom(KMemoryRegionType_MemoryController1))
                {
                    /* Check the region is valid. */
                    MESOSPHERE_ABORT_UNLESS(region->GetEndAddress() != 0);

                    /* Get the offset within the region. */
                    const size_t offset = address - region->GetAddress();
                    MESOSPHERE_ABORT_UNLESS(offset < region->GetSize());

                    /* Check the whitelist. */
                    if (AMS_LIKELY(CheckRegisterAllowedTable(McUserRegisterWhitelist, offset))) {
                        return true;
                    }
                }
            }

            return false;
        }

        bool SetSecureRegion(KPhysicalAddress phys_addr, size_t size) {
            /* Ensure size is valid. */
            if (size > SecureSizeMax) {
                return false;
            }

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
            R_UNLESS(util::IsAligned(size, PageSize),                svc::ResultInvalidSize());
            R_UNLESS(size <= KSystemControl::SecureAppletMemorySize, svc::ResultOutOfMemory());

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

            R_SUCCEED();
        }

        void FreeSecureMemoryForApplet(KVirtualAddress address, size_t size) {
            /* Disable interrupts and acquire the secure applet lock. */
            KScopedInterruptDisable di;
            KScopedSpinLock lk(g_secure_applet_lock);

            /* Verify that the memory being freed is correct. */
            MESOSPHERE_ABORT_UNLESS(address == g_secure_applet_memory_address);
            MESOSPHERE_ABORT_UNLESS(size <= KSystemControl::SecureAppletMemorySize);
            MESOSPHERE_ABORT_UNLESS(util::IsAligned(size, PageSize));
            MESOSPHERE_ABORT_UNLESS(g_secure_applet_memory_used);

            /* Release the secure applet memory. */
            g_secure_applet_memory_used = false;
        }

        u32 GetVersionIdentifier() {
            u32 value = 0;
            value |= static_cast<u64>(ATMOSPHERE_RELEASE_VERSION_MICRO) <<  0;
            value |= static_cast<u64>(ATMOSPHERE_RELEASE_VERSION_MINOR) <<  8;
            value |= static_cast<u64>(ATMOSPHERE_RELEASE_VERSION_MAJOR) << 16;
            value |= static_cast<u64>('M') << 24;
            return value;
        }

    }

    /* Initialization. */
    size_t KSystemControl::Init::GetRealMemorySize() {
        /* TODO: Move this into a header for the MC in general. */
        constexpr u32 MemoryControllerConfigurationRegister = 0x70019050;
        u32 config_value;
        smc::init::ReadWriteRegister(std::addressof(config_value), MemoryControllerConfigurationRegister, 0, 0);
        return static_cast<size_t>(config_value & 0x3FFF) << 20;
    }

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

    bool KSystemControl::Init::ShouldIncreaseThreadResourceLimit() {
        return GetKernelConfigurationForInit().Get<smc::KernelConfiguration::IncreaseThreadResourceLimit>();
    }

    size_t KSystemControl::Init::GetApplicationPoolSize() {
        /* Get the base pool size. */
        const size_t base_pool_size = []() ALWAYS_INLINE_LAMBDA -> size_t {
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
                    return 6964_MB;
            }
        }();

        /* Return (possibly) adjusted size. */
        return base_pool_size;
    }

    size_t KSystemControl::Init::GetAppletPoolSize() {
        /* Get the base pool size. */
        const size_t base_pool_size = []() ALWAYS_INLINE_LAMBDA -> size_t {
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
                    return 562_MB;
            }
        }();

        /* Return (possibly) adjusted size. */
        /* NOTE: On 20.0.0+ the browser requires much more memory in the applet pool in order to function. */
        /* Thus, we have to reduce our extra system memory size by 26 MB to compensate. */
        const size_t ExtraSystemMemoryForAtmosphere = kern::GetTargetFirmware() >= ams::TargetFirmware_20_0_0 ? 14_MB : 40_MB;
        return base_pool_size - ExtraSystemMemoryForAtmosphere - KTraceBufferSize;
    }

    size_t KSystemControl::Init::GetMinimumNonSecureSystemPoolSize() {
        /* Verify that our minimum is at least as large as Nintendo's. */
        constexpr size_t MinimumSizeWithFatal = ::ams::svc::RequiredNonSecureSystemMemorySizeWithFatal;
        static_assert(MinimumSizeWithFatal >= 0x2C04000);

        constexpr size_t MinimumSizeWithoutFatal = ::ams::svc::RequiredNonSecureSystemMemorySize;
        static_assert(MinimumSizeWithoutFatal >= 0x2A00000);

        /* Include fatal in non-seure size on 16.0.0+. */
        return kern::GetTargetFirmware() >= ams::TargetFirmware_16_0_0 ? MinimumSizeWithFatal : MinimumSizeWithoutFatal;
    }

    u8 KSystemControl::Init::GetDebugLogUartPort() {
        /* Get the log configuration. */
        u64 value = 0;
        smc::init::GetConfig(std::addressof(value), 1, smc::ConfigItem::ExosphereLogConfiguration);

        /* Extract the port. */
        return static_cast<u8>((value >> 32) & 0xFF);
    }

    void KSystemControl::Init::CpuOnImpl(u64 core_id, uintptr_t entrypoint, uintptr_t arg) {
        MESOSPHERE_INIT_ABORT_UNLESS((::ams::kern::arch::arm64::smc::CpuOn<smc::SmcId_Supervisor>(core_id, entrypoint, arg)) == 0);
    }

    /* Randomness for Initialization. */
    void KSystemControl::Init::GenerateRandom(u64 *dst, size_t count) {
        MESOSPHERE_INIT_ABORT_UNLESS(count <= 7);
        smc::init::GenerateRandomBytes(dst, count * sizeof(u64));
    }

    u64 KSystemControl::Init::GenerateRandomRange(u64 min, u64 max) {
        return KSystemControlBase::GenerateUniformRange(min, max, GenerateRandomU64ForInit);
    }

    /* System Initialization. */
    void KSystemControl::ConfigureKTargetSystem() {
        /* Configure KTargetSystem. */
        volatile auto *ts = const_cast<volatile KTargetSystem::KTargetSystemData *>(std::addressof(KTargetSystem::s_data));
        {
            /* Set whether we're in debug mode. */
            {
                ts->is_not_debug_mode    = !GetConfigBool(smc::ConfigItem::IsDebugMode);

                /* If we're not in debug mode, we don't want to initialize uart logging. */
                ts->disable_debug_logging = ts->is_not_debug_mode;
            }

            /* Set Kernel Configuration. */
            {
                const auto kernel_config = util::BitPack32{GetConfigU32(smc::ConfigItem::KernelConfiguration)};

                ts->disable_debug_memory_fill       = !kernel_config.Get<smc::KernelConfiguration::DebugFillMemory>();
                ts->disable_user_exception_handlers = !kernel_config.Get<smc::KernelConfiguration::EnableUserExceptionHandlers>();
                ts->disable_dynamic_resource_limits = kernel_config.Get<smc::KernelConfiguration::DisableDynamicResourceLimits>();
                ts->disable_user_pmu_access         = !kernel_config.Get<smc::KernelConfiguration::EnableUserPmuAccess>();

                /* Configure call smc on panic. */
                *const_cast<volatile bool *>(std::addressof(g_call_smc_on_panic)) = kernel_config.Get<smc::KernelConfiguration::UseSecureMonitorPanicCall>();
            }

            /* Set Kernel Debugging. */
            {
                /* NOTE: This is used to restrict access to SvcKernelDebug/SvcChangeKernelTraceState. */
                /* Mesosphere may wish to not require this, as we'd ideally keep ProgramVerification enabled for userland. */
                ts->disable_kernel_debugging = !GetConfigBool(smc::ConfigItem::DisableProgramVerification);
            }
        }
    }

    void KSystemControl::InitializePhase1() {
        /* Enable KTargetSystem. */
        KTargetSystem::SetInitialized();

        /* Check KTargetSystem was configured correctly. */
        {
            /* Check IsDebugMode. */
            {
                MESOSPHERE_ABORT_UNLESS(KTargetSystem::IsDebugMode() == GetConfigBool(smc::ConfigItem::IsDebugMode));
                MESOSPHERE_ABORT_UNLESS(KTargetSystem::IsDebugLoggingEnabled() == GetConfigBool(smc::ConfigItem::IsDebugMode));
            }

            /* Check Kernel Configuration. */
            {
                const auto kernel_config = util::BitPack32{GetConfigU32(smc::ConfigItem::KernelConfiguration)};

                MESOSPHERE_ABORT_UNLESS(KTargetSystem::IsDebugMemoryFillEnabled() == kernel_config.Get<smc::KernelConfiguration::DebugFillMemory>());
                MESOSPHERE_ABORT_UNLESS(KTargetSystem::IsUserExceptionHandlersEnabled() == kernel_config.Get<smc::KernelConfiguration::EnableUserExceptionHandlers>());
                MESOSPHERE_ABORT_UNLESS(KTargetSystem::IsDynamicResourceLimitsEnabled() == !kernel_config.Get<smc::KernelConfiguration::DisableDynamicResourceLimits>());
                MESOSPHERE_ABORT_UNLESS(KTargetSystem::IsUserPmuAccessEnabled() == kernel_config.Get<smc::KernelConfiguration::EnableUserPmuAccess>());

                MESOSPHERE_ABORT_UNLESS(g_call_smc_on_panic == kernel_config.Get<smc::KernelConfiguration::UseSecureMonitorPanicCall>());
            }

            /* Check Kernel Debugging. */
            {
                MESOSPHERE_ABORT_UNLESS(KTargetSystem::IsKernelDebuggingEnabled() == GetConfigBool(smc::ConfigItem::DisableProgramVerification));
            }
        }

        /* Initialize random and resource limit. */
        {
            u64 seed;
            smc::GenerateRandomBytes(std::addressof(seed), sizeof(seed));
            KSystemControlBase::InitializePhase1Base(seed);
        }

        /* Configure the Kernel Carveout region. */
        {
            const auto carveout = KMemoryLayout::GetCarveoutRegionExtents();
            MESOSPHERE_ABORT_UNLESS(carveout.GetEndAddress() != 0);

            smc::ConfigureCarveout(0, carveout.GetAddress(), carveout.GetSize());
        }
    }

    void KSystemControl::InitializePhase2() {
        /* Initialize the sleep manager. */
        KSleepManager::Initialize();

        /* Get the secure applet memory. */
        const auto &secure_applet_memory = KMemoryLayout::GetSecureAppletMemoryRegion();
        MESOSPHERE_INIT_ABORT_UNLESS(secure_applet_memory.GetSize() == SecureAppletMemorySize);

        g_secure_applet_memory_address = secure_applet_memory.GetAddress();

        /* Initialize KTrace (and potentially other init). */
        KSystemControlBase::InitializePhase2();
    }

    u32 KSystemControl::GetCreateProcessMemoryPool() {
        return KMemoryManager::Pool_Unsafe;
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
        R_SUCCEED();
    }

    /* Randomness. */
    void KSystemControl::GenerateRandom(u64 *dst, size_t count) {
        MESOSPHERE_INIT_ABORT_UNLESS(count <= 7);
        smc::GenerateRandomBytes(dst, count * sizeof(u64));
    }

    u64 KSystemControl::GenerateRandomRange(u64 min, u64 max) {
        KScopedInterruptDisable intr_disable;
        KScopedSpinLock lk(s_random_lock);


        if (AMS_LIKELY(!s_uninitialized_random_generator)) {
            return KSystemControlBase::GenerateUniformRange(min, max, []() ALWAYS_INLINE_LAMBDA -> u64 { return s_random_generator.GenerateRandomU64(); });
        } else {
            return KSystemControlBase::GenerateUniformRange(min, max, GenerateRandomU64FromSmc);
        }
    }

    u64 KSystemControl::GenerateRandomU64() {
        KScopedInterruptDisable intr_disable;
        KScopedSpinLock lk(s_random_lock);

        if (AMS_LIKELY(!s_uninitialized_random_generator)) {
            return s_random_generator.GenerateRandomU64();
        } else {
            return GenerateRandomU64FromSmc();
        }
    }

    void KSystemControl::SleepSystem() {
        MESOSPHERE_LOG("SleepSystem() was called\n");
        KSleepManager::SleepSystem();
    }

    void KSystemControl::StopSystem(void *arg) {
        if (arg != nullptr) {
            /* Get the address of the legacy IRAM region. */
            const KVirtualAddress iram_address = KMemoryLayout::GetDeviceVirtualAddress(KMemoryRegionType_LegacyLpsIram) + 64_KB;
            constexpr size_t RebootPayloadSize = 0x24000;

            /* NOTE: Atmosphere extension; if we received an exception context from Panic(), */
            /*       generate a fatal error report using it. */
            const KExceptionContext *e_ctx = static_cast<const KExceptionContext *>(arg);
            auto *f_ctx = GetPointer<::ams::impl::FatalErrorContext>(iram_address + 0x2E000);

            /* Clear the fatal context. */
            std::memset(f_ctx, 0xCC, sizeof(*f_ctx));

            /* Set metadata. */
            f_ctx->magic      = ::ams::impl::FatalErrorContext::Magic;
            f_ctx->error_desc = ::ams::impl::FatalErrorContext::KernelPanicDesc;
            f_ctx->program_id = (static_cast<u64>(util::FourCC<'M', 'E', 'S', 'O'>::Code) << 0) | (static_cast<u64>(util::FourCC<'S', 'P', 'H', 'R'>::Code) << 32);

            /* Set identifier. */
            f_ctx->report_identifier = KHardwareTimer::GetTick();

            /* Set module base. */
            f_ctx->module_base = KMemoryLayout::GetKernelCodeRegionExtents().GetAddress();

            /* Set afsr1. */
            f_ctx->afsr0 = GetVersionIdentifier();
            f_ctx->afsr1 = static_cast<u32>(kern::GetTargetFirmware());

            /* Set efsr/far. */
            f_ctx->far = cpu::GetFarEl1();
            f_ctx->esr = cpu::GetEsrEl1();

            /* Copy registers. */
            for (size_t i = 0; i < util::size(e_ctx->x); ++i) {
                f_ctx->gprs[i] = e_ctx->x[i];
            }
            f_ctx->sp = e_ctx->sp;
            f_ctx->pc = cpu::GetElrEl1();

            /* Dump stack trace. */
            {
                uintptr_t fp = e_ctx->x[29];
                for (f_ctx->stack_trace_size = 0; f_ctx->stack_trace_size < ::ams::impl::FatalErrorContext::MaxStackTrace && fp != 0 && util::IsAligned(fp, 0x10) && cpu::GetPhysicalAddressWritable(nullptr, fp, true); ++(f_ctx->stack_trace_size)) {
                    struct {
                        uintptr_t fp;
                        uintptr_t lr;
                    } *stack_frame = reinterpret_cast<decltype(stack_frame)>(fp);

                    f_ctx->stack_trace[f_ctx->stack_trace_size] = stack_frame->lr;
                    fp = stack_frame->fp;
                }
            }

            /* Dump stack. */
            {
                uintptr_t sp = e_ctx->sp;
                for (f_ctx->stack_dump_size = 0; f_ctx->stack_dump_size < ::ams::impl::FatalErrorContext::MaxStackDumpSize && cpu::GetPhysicalAddressWritable(nullptr, sp + f_ctx->stack_dump_size, true); f_ctx->stack_dump_size += sizeof(u64)) {
                    *reinterpret_cast<u64 *>(f_ctx->stack_dump + f_ctx->stack_dump_size) = *reinterpret_cast<u64 *>(sp + f_ctx->stack_dump_size);
                }
            }

            /* Try to get a payload address. */
            const KMemoryRegion *cached_region = nullptr;
            u64 reboot_payload_paddr = 0;
            if (smc::TryGetConfig(std::addressof(reboot_payload_paddr), 1, smc::ConfigItem::ExospherePayloadAddress) && KMemoryLayout::IsLinearMappedPhysicalAddress(cached_region, reboot_payload_paddr, RebootPayloadSize)) {
                /* If we have a payload, reboot to it. */
                const KVirtualAddress reboot_payload = KMemoryLayout::GetLinearVirtualAddress(KPhysicalAddress(reboot_payload_paddr));

                /* Clear IRAM. */
                std::memset(GetVoidPointer(iram_address), 0xCC, RebootPayloadSize);

                /* Copy the payload to iram. */
                for (size_t i = 0; i < RebootPayloadSize / sizeof(u32); ++i) {
                    GetPointer<volatile u32>(iram_address)[i] = GetPointer<volatile u32>(reboot_payload)[i];
                }
            }
            smc::SetConfig(smc::ConfigItem::ExosphereNeedsReboot, smc::UserRebootType_ToFatalError);
        }

        if (g_call_smc_on_panic) {
            /* If we should, instruct the secure monitor to display a panic screen. */
            smc::ShowError(0xF00);
        }

        AMS_INFINITE_LOOP();
    }

    /* User access. */
    void KSystemControl::CallSecureMonitorFromUserImpl(ams::svc::lp64::SecureMonitorArguments *args) {
        /* Invoke the secure monitor. */
        return smc::CallSecureMonitorFromUser(args);
    }

    /* Secure Memory. */
    size_t KSystemControl::CalculateRequiredSecureMemorySize(size_t size, u32 pool) {
        if (pool == KMemoryManager::Pool_Applet) {
            return 0;
        } else {
            return KSystemControlBase::CalculateRequiredSecureMemorySize(size, pool);
        }
    }

    Result KSystemControl::AllocateSecureMemory(KVirtualAddress *out, size_t size, u32 pool) {
        /* Applet secure memory is handled separately. */
        if (pool == KMemoryManager::Pool_Applet) {
            R_RETURN(AllocateSecureMemoryForApplet(out, size));
        }

        /* Ensure the size is aligned. */
        const size_t alignment = (pool == KMemoryManager::Pool_System ? PageSize : SecureAlignment);
        R_UNLESS(util::IsAligned(size, alignment), svc::ResultInvalidSize());

        /* Allocate the memory. */
        const size_t num_pages = size / PageSize;
        const KPhysicalAddress paddr = Kernel::GetMemoryManager().AllocateAndOpenContinuous(num_pages, alignment / PageSize, KMemoryManager::EncodeOption(static_cast<KMemoryManager::Pool>(pool), KMemoryManager::Direction_FromFront));
        R_UNLESS(paddr != Null<KPhysicalAddress>, svc::ResultOutOfMemory());

        /* Ensure we don't leak references to the memory on error. */
        ON_RESULT_FAILURE { Kernel::GetMemoryManager().Close(paddr, num_pages); };

        /* If the memory isn't already secure, set it as secure. */
        if (pool != KMemoryManager::Pool_System) {
            /* Set the secure region. */
            R_UNLESS(SetSecureRegion(paddr, size), svc::ResultOutOfMemory());
        }

        /* We succeeded. */
        *out = KPageTable::GetHeapVirtualAddress(paddr);
        R_SUCCEED();
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
