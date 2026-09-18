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
#include <vapours/crypto/impl/crypto_aes_impl.hpp>
#if defined(ATMOSPHERE_ARCH_ARM64)
#include <mesosphere/arch/arm64/kern_secure_monitor_base.hpp>
#endif

namespace ams::kern {

    namespace init {

        /* TODO: Is this function name architecture specific? */
        void StartOtherCore(const ams::kern::init::KInitArguments *init_args);

    }

    namespace {

        /* The kernel is otherwise FP/SIMD-free, but AES instructions require the NEON registers. */
        class KScopedFpuEnable {
            private:
                u64 m_saved_cpacr_el1;
            public:
                ALWAYS_INLINE KScopedFpuEnable() : m_saved_cpacr_el1(cpu::GetCpacrEl1()) {
                    cpu::SetCpacrEl1(m_saved_cpacr_el1 | 0x100000);
                    cpu::InstructionMemoryBarrier();
                }

                ALWAYS_INLINE ~KScopedFpuEnable() {
                    cpu::SetCpacrEl1(m_saved_cpacr_el1);
                    cpu::InstructionMemoryBarrier();
                }
        };

        ALWAYS_INLINE void CtrDrbgIncrementCounter(u8 *v) {
            for (size_t i = 16; i > 0; --i) {
                if ((++v[i - 1]) != 0) {
                    break;
                }
            }
        }

        void CtrDrbgUpdate(CtrDrbgContext &ctx, const u8 *data) {
            /* Initialize the block cipher with the current key. */
            ams::crypto::impl::AesImpl<16> aes;
            aes.Initialize(ctx.key, sizeof(ctx.key), true);

            /* Generate two blocks of new key/counter material. */
            CtrDrbgIncrementCounter(ctx.v);
            aes.EncryptBlock(ctx.temp + 0,  16, ctx.v, 16);
            CtrDrbgIncrementCounter(ctx.v);
            aes.EncryptBlock(ctx.temp + 16, 16, ctx.v, 16);

            /* Mix in the provided data. */
            for (size_t i = 0; i < sizeof(ctx.temp); ++i) {
                ctx.temp[i] ^= data[i];
            }

            /* Update the key and counter. */
            std::memcpy(ctx.key, ctx.temp,                   sizeof(ctx.key));
            std::memcpy(ctx.v,   ctx.temp + sizeof(ctx.key), sizeof(ctx.v));
        }

    }

    /* Initialization. */
    size_t KSystemControlBase::Init::GetRealMemorySize() {
        return ams::kern::MainMemorySize;
    }

    size_t KSystemControlBase::Init::GetIntendedMemorySize() {
        return ams::kern::MainMemorySize;
    }

    KPhysicalAddress KSystemControlBase::Init::GetKernelPhysicalBaseAddress(KPhysicalAddress base_address) {
        const size_t real_dram_size     = KSystemControl::Init::GetRealMemorySize();
        const size_t intended_dram_size = KSystemControl::Init::GetIntendedMemorySize();
        if (intended_dram_size * 2 <= real_dram_size) {
            return base_address;
        } else {
            return base_address + ((real_dram_size - intended_dram_size) / 2);
        }
    }

    void KSystemControlBase::Init::GetInitialProcessBinaryLayout(InitialProcessBinaryLayout *out, KPhysicalAddress kern_base_address) {
        *out = {
            .address      = GetInteger(KSystemControl::Init::GetKernelPhysicalBaseAddress(ams::kern::MainMemoryAddress)) + KSystemControl::Init::GetIntendedMemorySize() - InitialProcessBinarySizeMax,
            ._08          = 0,
            .kern_address = GetInteger(kern_base_address),
        };
    }


    bool KSystemControlBase::Init::ShouldIncreaseThreadResourceLimit() {
        return true;
    }


    size_t KSystemControlBase::Init::GetApplicationPoolSize() {
        return 0;
    }

    size_t KSystemControlBase::Init::GetAppletPoolSize() {
        return 0;
    }

    size_t KSystemControlBase::Init::GetMinimumNonSecureSystemPoolSize() {
        return 0;
    }

    u8 KSystemControlBase::Init::GetDebugLogUartPort() {
        return 0;
    }

    void KSystemControlBase::Init::CpuOnImpl(u64 core_id, uintptr_t entrypoint, uintptr_t arg) {
        #if defined(ATMOSPHERE_ARCH_ARM64)
        MESOSPHERE_INIT_ABORT_UNLESS((::ams::kern::arch::arm64::smc::CpuOn<0>(core_id, entrypoint, arg)) == 0);
        #else
        AMS_INFINITE_LOOP();
        #endif
    }

    void KSystemControlBase::Init::TurnOnCpu(u64 core_id, const ams::kern::init::KInitArguments *args) {
        /* Get entrypoint. */
        KPhysicalAddress entrypoint = Null<KPhysicalAddress>;
        while (!cpu::GetPhysicalAddressReadable(std::addressof(entrypoint), reinterpret_cast<uintptr_t>(::ams::kern::init::StartOtherCore), true)) { /* ... */ }

        /* Get arguments. */
        KPhysicalAddress args_addr = Null<KPhysicalAddress>;
        while (!cpu::GetPhysicalAddressReadable(std::addressof(args_addr), reinterpret_cast<uintptr_t>(args), true)) { /* ... */ }

        /* Ensure cache is correct for the initial arguments. */
        cpu::StoreDataCacheForInitArguments(args, sizeof(*args));

        /* Turn on the cpu. */
        KSystemControl::Init::CpuOnImpl(core_id, GetInteger(entrypoint), GetInteger(args_addr));
    }

    /* Randomness for Initialization. */
    void KSystemControlBase::Init::GenerateRandom(u64 *dst, size_t count) {
        for (size_t i = 0; i < count; ++i) {
            dst[i] = KSystemControlBase::GenerateRandomU64();
        }
    }

    u64 KSystemControlBase::Init::GenerateRandomRange(u64 min, u64 max) {
        return KSystemControlBase::GenerateRandomRange(min, max);
    }

    /* System Initialization. */
    void KSystemControlBase::ConfigureKTargetSystem() {
        /* By default, use the default config set in the KTargetSystem header. */
    }

    void KSystemControlBase::InitializePhase1() {
        /* Enable KTargetSystem. */
        {
            KTargetSystem::SetInitialized();
        }

        /* Initialize random and resource limit. */
        u64 seed[4];
        for (size_t i = 0; i < util::size(seed); ++i) {
            seed[i] = KHardwareTimer::GetTick();
        }
        KSystemControlBase::InitializePhase1Base(seed, sizeof(seed));
    }

    void KSystemControlBase::InitializePhase1Base(const void *seed, size_t size) {
        /* Initialize the random generator. */
        KSystemControlBase::InitializeRandomGenerator(seed, size);
        s_uninitialized_random_generator = false;

        /* Initialize debug logging. */
        KDebugLog::Initialize();

        /* System ResourceLimit initialization. */
        {
            /* Construct the resource limit object. */
            KResourceLimit &sys_res_limit = Kernel::GetSystemResourceLimit();
            KAutoObject::Create<KResourceLimit>(std::addressof(sys_res_limit));
            sys_res_limit.Initialize();

            /* Set the initial limits. */
            const auto [total_memory_size, kernel_memory_size] = KMemoryLayout::GetTotalAndKernelMemorySizes();

            /* Update 39-bit address space infos. */
            {
                /* Heap should be equal to the total memory size, minimum 8 GB, maximum 32 GB. */
                /* Alias should be equal to 8 * heap size, maximum 128 GB. */
                const size_t heap_size  = std::max(std::min(util::AlignUp(total_memory_size, 1_GB), 32_GB), 8_GB);
                const size_t alias_size = std::min(heap_size * 8, 128_GB);

                /* Set the address space sizes. */
                KAddressSpaceInfo::SetAddressSpaceSize(39, KAddressSpaceInfo::Type_Heap,  heap_size);
                KAddressSpaceInfo::SetAddressSpaceSize(39, KAddressSpaceInfo::Type_Alias, alias_size);
            }

            /* Update 42-bit address space infos. */
            {
                /* Heap should be equal to the total memory size, maximum 128 GB. */
                /* Alias/Stack/MapSmall are left at their static defaults. */
                KAddressSpaceInfo::SetAddressSpaceSize(42, KAddressSpaceInfo::Type_Heap, std::min(util::AlignUp(total_memory_size, 1_GB), 128_GB));
            }

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

    void KSystemControlBase::InitializePhase2() {
        /* Initialize KTrace. */
        if constexpr (IsKTraceEnabled) {
            const auto &ktrace = KMemoryLayout::GetKernelTraceBufferRegion();
            KTrace::Initialize(ktrace.GetAddress(), ktrace.GetSize());
        }
    }

    u32 KSystemControlBase::GetCreateProcessMemoryPool() {
        return KMemoryManager::Pool_System;
    }

    /* Privileged Access. */
    void KSystemControlBase::ReadWriteRegisterPrivileged(u32 *out, ams::svc::PhysicalAddress address, u32 mask, u32 value) {
        /* TODO */
        MESOSPHERE_UNUSED(out, address, mask, value);
        MESOSPHERE_UNIMPLEMENTED();
    }

    Result KSystemControlBase::ReadWriteRegister(u32 *out, ams::svc::PhysicalAddress address, u32 mask, u32 value) {
        MESOSPHERE_UNUSED(out, address, mask, value);
        R_THROW(svc::ResultNotImplemented());
    }

    /* Randomness helpers. Assume the caller holds the random lock. */
    void KSystemControlBase::InitializeRandomGenerator(const void *seed, size_t size) {
        MESOSPHERE_ASSERT(size == sizeof(s_random_generator.seed));

        KScopedFpuEnable fp_enabled;
        auto &ctx = s_random_generator;

        /* Zero the counter and key, then derive the initial state from the seed. */
        std::memset(ctx.v,   0, sizeof(ctx.v));
        std::memset(ctx.key, 0, sizeof(ctx.key));
        std::memcpy(ctx.seed, seed, sizeof(ctx.seed));

        CtrDrbgUpdate(ctx, ctx.seed);

        ctx.reseed_counter = 1;
        ctx.remaining      = 0;
    }

    void KSystemControlBase::ReseedRandomGeneratorInternal() {
        KScopedFpuEnable fp_enabled;
        auto &ctx = s_random_generator;

        /* Pull fresh entropy, then derive a new state from it. */
        KSystemControl::GenerateRandomBytesForUninitialized(ctx.seed, sizeof(ctx.seed));
        CtrDrbgUpdate(ctx, ctx.seed);

        ctx.remaining      = 0;
        ctx.reseed_counter = 1;
    }

    u32 KSystemControlBase::GenerateRandomU32() {
        auto &ctx = s_random_generator;

        if (ctx.remaining == 0) {
            KScopedFpuEnable fp_enabled;

            /* Reseed if we've exhausted this generation's entropy, otherwise use zero additional input. */
            if (ctx.reseed_counter <= 0x7FFFFFF0) {
                std::memset(ctx.seed, 0, sizeof(ctx.seed));
            } else {
                KSystemControl::GenerateRandomBytesForUninitialized(ctx.seed, sizeof(ctx.seed));
                CtrDrbgUpdate(ctx, ctx.seed);
                ctx.remaining = 0;
                std::memset(ctx.seed, 0, sizeof(ctx.seed));
                ctx.reseed_counter = 1;
            }

            /* Generate eight blocks of output. */
            ams::crypto::impl::AesImpl<16> aes;
            aes.Initialize(ctx.key, sizeof(ctx.key), true);
            for (size_t i = 0; i < 8; ++i) {
                CtrDrbgIncrementCounter(ctx.v);
                aes.EncryptBlock(ctx.output + 16 * i, 16, ctx.v, 16);
            }

            /* Derive the next state. */
            CtrDrbgUpdate(ctx, ctx.seed);

            ++ctx.reseed_counter;
            ctx.remaining = 32;
        }

        /* Consume a word from the output buffer, zeroizing it in place for backtracking resistance. */
        const size_t index = 32 - ctx.remaining;
        u32 value;
        std::memcpy(std::addressof(value), ctx.output + 4 * index, sizeof(value));
        std::memset(ctx.output + 4 * index, 0, sizeof(value));
        --ctx.remaining;
        return value;
    }

    u64 KSystemControlBase::GenerateRandomU64Internal() {
        /* Fall back to the board's raw source while the generator is uninitialized. */
        if (AMS_UNLIKELY(s_uninitialized_random_generator)) {
            u64 value;
            KSystemControl::GenerateRandomBytesForUninitialized(std::addressof(value), sizeof(value));
            return value;
        }

        const u64 high = KSystemControlBase::GenerateRandomU32();
        const u64 low  = KSystemControlBase::GenerateRandomU32();
        return (high << 32) | low;
    }

    /* Randomness. */
    void KSystemControlBase::GenerateRandom(u64 *dst, size_t count) {
        KScopedInterruptDisable intr_disable;
        KScopedSpinLock lk(s_random_lock);

        for (size_t i = 0; i < count; ++i) {
            dst[i] = KSystemControlBase::GenerateRandomU64Internal();
        }
    }

    u64 KSystemControlBase::GenerateRandomRange(u64 min, u64 max) {
        KScopedInterruptDisable intr_disable;
        KScopedSpinLock lk(s_random_lock);

        /* While uninitialized, fall back to a raw single-draw modulo. */
        if (AMS_UNLIKELY(s_uninitialized_random_generator)) {
            u64 value;
            KSystemControl::GenerateRandomBytesForUninitialized(std::addressof(value), sizeof(value));

            const u64 difference = max - min;
            if (difference == std::numeric_limits<u64>::max()) {
                return value;
            }

            return min + (value % (difference + 1));
        }

        return KSystemControlBase::GenerateUniformRange(min, max, []() ALWAYS_INLINE_LAMBDA -> u64 { return KSystemControlBase::GenerateRandomU64Internal(); });
    }

    u64 KSystemControlBase::GenerateRandomU64() {
        KScopedInterruptDisable intr_disable;
        KScopedSpinLock lk(s_random_lock);

        return KSystemControlBase::GenerateRandomU64Internal();
    }

    void KSystemControlBase::ReseedRandomGenerator() {
        /* This must be unreachable while the generator is uninitialized. */
        MESOSPHERE_ABORT_UNLESS(!s_uninitialized_random_generator);

        KScopedInterruptDisable intr_disable;
        KScopedSpinLock lk(s_random_lock);

        KSystemControlBase::ReseedRandomGeneratorInternal();
    }

    void KSystemControlBase::GenerateRandomBytesForUninitialized(void *dst, size_t size) {
        /* The default board has no secure monitor; fall back to the hardware timer. */
        u8 *dst_8 = static_cast<u8 *>(dst);
        for (size_t offset = 0; offset < size; offset += sizeof(u64)) {
            const u64 tick = KHardwareTimer::GetTick();
            std::memcpy(dst_8 + offset, std::addressof(tick), std::min(sizeof(tick), size - offset));
        }
    }

    void KSystemControlBase::SleepSystem() {
        MESOSPHERE_LOG("SleepSystem() was called\n");
    }

    void KSystemControlBase::StopSystem(void *) {
        MESOSPHERE_LOG("KSystemControlBase::StopSystem\n");
        AMS_INFINITE_LOOP();
    }

    /* User access. */
    #if defined(ATMOSPHERE_ARCH_ARM64)
    void KSystemControlBase::CallSecureMonitorFromUser(ams::svc::lp64::SecureMonitorArguments *args) {
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
        KSystemControl::CallSecureMonitorFromUserImpl(args);

        /* Make sure that we close any pages that we opened. */
        for (size_t i = 0; i < MaxMappedRegisters; i++) {
            page_groups[i].Close();
        }
    }

    void KSystemControlBase::CallSecureMonitorFromUserImpl(ams::svc::lp64::SecureMonitorArguments *args) {
        /* By default, we don't actually support secure monitor, so just set args to a failure code. */
        args->r[0] = 1;
    }
    #endif

    /* Secure Memory. */
    size_t KSystemControlBase::CalculateRequiredSecureMemorySize(size_t size, u32 pool) {
        MESOSPHERE_UNUSED(pool);
        return size;
    }

    Result KSystemControlBase::AllocateSecureMemory(KVirtualAddress *out, size_t size, u32 pool) {
        /* Ensure the size is aligned. */
        constexpr size_t Alignment = PageSize;
        R_UNLESS(util::IsAligned(size, Alignment), svc::ResultInvalidSize());

        /* Allocate the memory. */
        const size_t num_pages = size / PageSize;
        const KPhysicalAddress paddr = Kernel::GetMemoryManager().AllocateAndOpenContinuous(num_pages, Alignment / PageSize, KMemoryManager::EncodeOption(static_cast<KMemoryManager::Pool>(pool), KMemoryManager::Direction_FromFront));
        R_UNLESS(paddr != Null<KPhysicalAddress>, svc::ResultOutOfMemory());

        *out = KPageTable::GetHeapVirtualAddress(paddr);
        R_SUCCEED();
    }

    void KSystemControlBase::FreeSecureMemory(KVirtualAddress address, size_t size, u32 pool) {
        /* Ensure the size is aligned. */
        constexpr size_t Alignment = PageSize;
        MESOSPHERE_UNUSED(pool);
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(GetInteger(address), Alignment));
        MESOSPHERE_ABORT_UNLESS(util::IsAligned(size, Alignment));

        /* Close the secure region's pages. */
        Kernel::GetMemoryManager().Close(KPageTable::GetHeapPhysicalAddress(address), size / PageSize);
    }

    /* Insecure Memory. */
    KResourceLimit *KSystemControlBase::GetInsecureMemoryResourceLimit() {
        return std::addressof(Kernel::GetSystemResourceLimit());
    }

    u32 KSystemControlBase::GetInsecureMemoryPool() {
        return KMemoryManager::Pool_SystemNonSecure;
    }

}
