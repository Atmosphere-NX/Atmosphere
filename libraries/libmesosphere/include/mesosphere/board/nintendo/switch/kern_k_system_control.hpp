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
#include <mesosphere/kern_common.hpp>

namespace ams::kern {

    class KSystemControl {
        public:
            class Init {
                public:
                    /* Initialization. */
                    static size_t GetIntendedMemorySize();
                    static KPhysicalAddress GetKernelPhysicalBaseAddress(uintptr_t base_address);
                    static bool ShouldIncreaseThreadResourceLimit();
                    static void CpuOn(u64 core_id, uintptr_t entrypoint, uintptr_t arg);
                    static size_t GetApplicationPoolSize();
                    static size_t GetAppletPoolSize();
                    static size_t GetMinimumNonSecureSystemPoolSize();

                    /* Randomness. */
                    static void GenerateRandomBytes(void *dst, size_t size);
                    static u64  GenerateRandomRange(u64 min, u64 max);
            };
        public:
            /* Initialization. */
            static NOINLINE void Initialize();
            static NOINLINE u32 GetInitialProcessBinaryPool();

            /* Randomness. */
            static void GenerateRandomBytes(void *dst, size_t size);
            static u64  GenerateRandomRange(u64 min, u64 max);

            /* Panic. */
            static NORETURN void StopSystem();
    };

}