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
#include <mesosphere/kern_k_system_control_base.hpp>

namespace ams::kern::board::nintendo::nx {

    class KSystemControl : public KSystemControlBase {
        public:
            class Init : public KSystemControlBase::Init {
                public:
                    /* Initialization. */
                    static size_t GetRealMemorySize();
                    static size_t GetIntendedMemorySize();
                    static bool ShouldIncreaseThreadResourceLimit();
                    static void CpuOn(u64 core_id, uintptr_t entrypoint, uintptr_t arg);
                    static size_t GetApplicationPoolSize();
                    static size_t GetAppletPoolSize();
                    static size_t GetMinimumNonSecureSystemPoolSize();
                    static u8 GetDebugLogUartPort();

                    /* Randomness. */
                    static void GenerateRandom(u64 *dst, size_t count);
                    static u64  GenerateRandomRange(u64 min, u64 max);
            };
        public:
            /* Initialization. */
            static NOINLINE void InitializePhase1();
            static NOINLINE void InitializePhase2();
            static NOINLINE u32 GetCreateProcessMemoryPool();

            /* Randomness. */
            static void GenerateRandom(u64 *dst, size_t count);
            static u64  GenerateRandomRange(u64 min, u64 max);
            static u64  GenerateRandomU64();

            /* Privileged Access. */
            static void ReadWriteRegisterPrivileged(u32 *out, ams::svc::PhysicalAddress address, u32 mask, u32 value);
            static Result ReadWriteRegister(u32 *out, ams::svc::PhysicalAddress address, u32 mask, u32 value);

            /* Power management. */
            static void SleepSystem();
            static NORETURN void StopSystem(void *arg = nullptr);

            /* User access. */
            static void CallSecureMonitorFromUserImpl(ams::svc::lp64::SecureMonitorArguments *args);

            /* Secure Memory. */
            static size_t CalculateRequiredSecureMemorySize(size_t size, u32 pool);
            static Result AllocateSecureMemory(KVirtualAddress *out, size_t size, u32 pool);
            static void FreeSecureMemory(KVirtualAddress address, size_t size, u32 pool);
    };

}