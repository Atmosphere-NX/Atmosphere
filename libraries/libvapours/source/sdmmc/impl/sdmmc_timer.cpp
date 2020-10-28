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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif
#include "sdmmc_timer.hpp"

namespace ams::sdmmc::impl {

    namespace {

        #if defined(AMS_SDMMC_USE_OS_TIMER)
        void SpinWaitMicroSeconds(u32 us) {
            const os::Tick timeout_tick = os::GetSystemTick() + os::ConvertToTick(TimeSpan::FromMicroSeconds(us)) + os::Tick(1);
            while (true) {
                if (os::GetSystemTick() > timeout_tick) {
                    break;
                }
            }
        }

        ALWAYS_INLINE void DataSynchronizationBarrier() {
            #if defined(ATMOSPHERE_ARCH_ARM64)
            __asm__ __volatile__("dsb sy" ::: "memory");
            #elif defined(ATMOSPHERE_ARCH_ARM)
            __asm__ __volatile__("dsb" ::: "memory");
            #else
                #error "Unknown architecture for DataSynchronizationBarrier"
            #endif
        }

        ALWAYS_INLINE void InstructionSynchronizationBarrier() {
            #if defined(ATMOSPHERE_ARCH_ARM64) || defined(ATMOSPHERE_ARCH_ARM)
            __asm__ __volatile__("isb" ::: "memory");
            #else
                #error "Unknown architecture for InstructionSynchronizationBarrier"
            #endif
        }
        #endif

    }

    void WaitMicroSeconds(u32 us) {
        #if defined(AMS_SDMMC_USE_OS_TIMER)
            /* Ensure that nothing is reordered before we wait. */
            DataSynchronizationBarrier();
            InstructionSynchronizationBarrier();

            /* If the time is small, spinloop, otherwise pend ourselves. */
            if (us < 100) {
                SpinWaitMicroSeconds(us);
            } else {
                os::SleepThread(TimeSpan::FromMicroSeconds(us));
            }

            /* Ensure that nothing is reordered after we wait. */
            DataSynchronizationBarrier();
            InstructionSynchronizationBarrier();
        #elif defined(AMS_SDMMC_USE_UTIL_TIMER)
            util::WaitMicroSeconds(us);
        #else
            #error "Unknown context for ams::sdmmc::impl::WaitMicroSeconds"
        #endif
    }

    void WaitClocks(u32 num_clocks, u32 clock_frequency_khz) {
        AMS_ABORT_UNLESS(clock_frequency_khz > 0);
        WaitMicroSeconds(util::DivideUp(1000 * num_clocks, clock_frequency_khz));
    }

}
