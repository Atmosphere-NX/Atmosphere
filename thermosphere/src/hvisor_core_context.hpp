/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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
#include "defines.hpp"

namespace ams::hvisor {

    struct ExceptionStackFrame;
    class CoreContext;

    register CoreContext *currentCoreCtx asm("x18");

    class alignas(64) CoreContext final {
        // This should be 64-byte big
        NON_COPYABLE(CoreContext);
        NON_MOVEABLE(CoreContext);
        private:
            static std::array<CoreContext, MAX_CORE> instances;
            static std::atomic<u32> activeCoreMask;
            static bool coldboot; // "coldboot" to be 'true' on init & thus not in BSS

            // start.s
            static uintptr_t initialKernelEntrypoint;

        private:
            ExceptionStackFrame *m_guestFrame = nullptr;

            u64 m_kernelArgument = 0;
            uintptr_t m_kernelEntrypoint = 0;
        
            u32 m_coreId = 0;
            bool m_bootCore = false;

            // Debug features
            bool m_wasPaused = false;
            uintptr_t m_steppingRangeStartAddr = 0;
            uintptr_t m_steppingRangeEndAddr = 0;

            // Timer stuff
            u64 m_totalTimeInHypervisor = 0;
            u64 m_emulPtimerCval = 0;

        private:
            constexpr CoreContext() = default;

        public:
            static void InitializeCoreInstance(u32 coreId, bool isBootCore, u64 argument);

            static CoreContext &GetInstanceFor(u32 coreId)                  { return instances[coreId]; }
            static u32 GetActiveCoreMask()                                  { return activeCoreMask.load(); }
            static u32 SetCurrentCoreActive()
            {
                activeCoreMask |= BIT(currentCoreCtx->m_coreId);
            }
            static bool IsColdboot()                                        { return coldboot; }

        public:
            constexpr ExceptionStackFrame *GetGuestFrame() const            { return m_guestFrame; }
            constexpr void SetGuestFrame(ExceptionStackFrame *frame)        { m_guestFrame = frame; }

            constexpr u64 GetKernelArgument() const                         { return m_kernelArgument; }

            constexpr u64 GetKernelEntrypoint() const                       { return m_kernelEntrypoint; }

            constexpr u32 GetCoreId() const                                 { return m_coreId; }
            constexpr bool IsBootCore() const                               { return m_bootCore; }

            constexpr u64 SetKernelEntrypoint(uintptr_t ep, bool warmboot = false)
            {
                if (warmboot) {
                    // No race possible, only possible transition is 1->0 and we only really check IsColdboot() at init time
                    // And CPU_SUSPEND should only be called with only one core left.
                    coldboot = false;
                }
                m_kernelEntrypoint = ep;
            }

            constexpr bool WasPaused() const                                { return m_wasPaused; }
            constexpr void SetPausedFlag(bool wasPaused)                    { m_wasPaused = wasPaused; }

            constexpr auto GetSteppingRange() const
            {
                return std::tuple{m_steppingRangeStartAddr, m_steppingRangeEndAddr};
            }
            constexpr void SetSteppingRange(uintptr_t startAddr, uintptr_t endAddr)
            {
                m_steppingRangeStartAddr = startAddr;
                m_steppingRangeEndAddr = endAddr;
            }

            constexpr u64 GetTotalTimeInHypervisor() const                  { return m_totalTimeInHypervisor; }
            constexpr void IncrementTotalTimeInHypervisor(u64 timeDelta)    { m_totalTimeInHypervisor += timeDelta; }

            constexpr u64 GetEmulPtimerCval() const                         { return m_emulPtimerCval; }
            constexpr void SetEmulPtimerCval(u64 cval)                      { m_emulPtimerCval = cval; }
    };
}
