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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_spin_lock.hpp>

namespace ams::kern {

    struct InitialProcessBinaryLayout;

    namespace init {

        struct KInitArguments;

    }

}

namespace ams::kern {

    class KResourceLimit;

    class KSystemControlBase {
        public:
            /* This can be overridden as needed. */
            static constexpr size_t SecureAppletMemorySize = 0;
        protected:
            /* Nintendo uses std::mt19937_t for randomness. */
            /* To save space (and because mt19337_t isn't secure anyway), */
            /* We will use TinyMT. */
            static constinit inline bool         s_uninitialized_random_generator{true};
            static constinit inline util::TinyMT s_random_generator{util::ConstantInitialize};
            static constinit inline KSpinLock    s_random_lock;
        public:
            class Init {
                private:
                    static void CpuOnImpl(u64 core_id, uintptr_t entrypoint, uintptr_t arg);
                public:
                    /* Initialization. */
                    static size_t GetRealMemorySize();
                    static size_t GetIntendedMemorySize();
                    static KPhysicalAddress GetKernelPhysicalBaseAddress(KPhysicalAddress base_address);
                    static void GetInitialProcessBinaryLayout(InitialProcessBinaryLayout *out, KPhysicalAddress kern_base_address);
                    static bool ShouldIncreaseThreadResourceLimit();
                    static void TurnOnCpu(u64 core_id, const ams::kern::init::KInitArguments *args);
                    static size_t GetApplicationPoolSize();
                    static size_t GetAppletPoolSize();
                    static size_t GetMinimumNonSecureSystemPoolSize();
                    static u8 GetDebugLogUartPort();

                    /* Randomness. */
                    static void GenerateRandom(u64 *dst, size_t count);
                    static u64  GenerateRandomRange(u64 min, u64 max);
            };
        protected:
            static NOINLINE void InitializePhase1Base(u64 seed);
        public:
            /* Initialization. */
            static NOINLINE void ConfigureKTargetSystem();
            static NOINLINE void InitializePhase1();
            static NOINLINE void InitializePhase2();
            static NOINLINE u32 GetCreateProcessMemoryPool();

            /* Randomness. */
            static void GenerateRandom(u64 *dst, size_t count);
            static u64  GenerateRandomRange(u64 min, u64 max);
            static u64  GenerateRandomU64();

            /* Register access Access. */
            static Result ReadWriteRegister(u32 *out, ams::svc::PhysicalAddress address, u32 mask, u32 value);
            static void ReadWriteRegisterPrivileged(u32 *out, ams::svc::PhysicalAddress address, u32 mask, u32 value);

            static u32 ReadRegisterPrivileged(ams::svc::PhysicalAddress address);
            static void WriteRegisterPrivileged(ams::svc::PhysicalAddress address, u32 value);

            /* Power management. */
            static void SleepSystem();
            static NORETURN void StopSystem(void *arg = nullptr);

            /* User access. */
            #if defined(ATMOSPHERE_ARCH_ARM64)
            static void CallSecureMonitorFromUser(ams::svc::lp64::SecureMonitorArguments *args);
            #endif

            /* Secure Memory. */
            static size_t CalculateRequiredSecureMemorySize(size_t size, u32 pool);
            static Result AllocateSecureMemory(KVirtualAddress *out, size_t size, u32 pool);
            static void FreeSecureMemory(KVirtualAddress address, size_t size, u32 pool);

            /* Insecure Memory. */
            static KResourceLimit *GetInsecureMemoryResourceLimit();
            static u32 GetInsecureMemoryPool();
        protected:
            template<typename F>
            static ALWAYS_INLINE u64 GenerateUniformRange(u64 min, u64 max, F f) {
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

            /* User access. */
            #if defined(ATMOSPHERE_ARCH_ARM64)
            static void CallSecureMonitorFromUserImpl(ams::svc::lp64::SecureMonitorArguments *args);
            #endif
    };

}