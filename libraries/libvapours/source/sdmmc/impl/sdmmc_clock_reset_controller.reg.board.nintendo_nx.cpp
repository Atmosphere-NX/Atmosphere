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
#include "sdmmc_clock_reset_controller.reg.board.nintendo_nx.hpp"
#include "sdmmc_timer.hpp"

namespace ams::sdmmc::impl::ClockResetController::reg {

    namespace {

        #if defined(AMS_SDMMC_THREAD_SAFE)

            constinit os::SdkMutex g_init_mutex;

            #define AMS_SDMMC_LOCK_INIT_MUTEX() std::scoped_lock lk(g_init_mutex)

        #else

            #define AMS_SDMMC_LOCK_INIT_MUTEX()

        #endif

        constinit bool g_is_initialized = false;

        struct ModuleInfo {
            u32 target_frequency_khz;
            u32 actual_frequency_khz;
        };

        constinit ModuleInfo g_module_infos[Module_Count] = {};

        constexpr inline dd::PhysicalAddress ClockResetControllerRegistersPhysicalAddress = UINT64_C(0x60006000);
        constexpr inline size_t ClockResetControllerRegistersSize = 4_KB;

        constinit uintptr_t g_clkrst_registers_address = 0;

        [[maybe_unused]] void InitializePllc4() {
            /* Check that we have registers we can write to. */
            AMS_ABORT_UNLESS(g_clkrst_registers_address != 0);

            /* Check if PLLC4_BASE has the expected value; if it does, we have nothing to do. */
            constexpr u32 ExpectedPllc4Base = 0x58006804;
            if (ams::reg::Read(g_clkrst_registers_address + CLK_RST_CONTROLLER_PLLC4_BASE) == ExpectedPllc4Base) {
                return;
            }

            /* Disable PLLC4_ENABLE, if it's currently set. */
            if (ams::reg::HasValue(g_clkrst_registers_address + CLK_RST_CONTROLLER_PLLC4_BASE, CLK_RST_REG_BITS_ENUM(PLLC4_BASE_PLLC4_ENABLE, ENABLE))) {
                ams::reg::ReadWrite(g_clkrst_registers_address + CLK_RST_CONTROLLER_PLLC4_BASE, CLK_RST_REG_BITS_ENUM(PLLC4_BASE_PLLC4_ENABLE, DISABLE));
            }

            /* Operate on the register with PLLC4_ENABLE cleared. */
            {
                /* Clear IDDQ, read to be sure it takes, wait 5 us. */
                ams::reg::ReadWrite(g_clkrst_registers_address + CLK_RST_CONTROLLER_PLLC4_BASE, CLK_RST_REG_BITS_ENUM(PLLC4_BASE_PLLC4_IDDQ, OFF));
                ams::reg::Read(g_clkrst_registers_address + CLK_RST_CONTROLLER_PLLC4_BASE);
                WaitMicroSeconds(5);

                /* Write the expected value sans IDDQ/PLLC4_ENABLE. */
                constexpr u32 ExpectedPllc4BaseMask = ~ams::reg::EncodeMask(CLK_RST_REG_BITS_MASK(PLLC4_BASE_PLLC4_ENABLE),
                                                                            CLK_RST_REG_BITS_MASK(PLLC4_BASE_PLLC4_IDDQ));
                ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_PLLC4_BASE, ExpectedPllc4Base & ExpectedPllc4BaseMask);
            }

            /* Write PLLC4_ENABLE, and read to be sure our configuration takes. */
            ams::reg::ReadWrite(g_clkrst_registers_address + CLK_RST_CONTROLLER_PLLC4_BASE, CLK_RST_REG_BITS_ENUM(PLLC4_BASE_PLLC4_ENABLE, ENABLE));
            ams::reg::Read(g_clkrst_registers_address + CLK_RST_CONTROLLER_PLLC4_BASE);

            /* Wait up to 1s for changes to take. */
            {
                ManualTimer timer(1000);
                while (true) {
                    /* Check if we're done. */
                    if (!ams::reg::HasValue(g_clkrst_registers_address + CLK_RST_CONTROLLER_PLLC4_BASE, CLK_RST_REG_BITS_ENUM(PLLC4_BASE_PLLC4_LOCK, NOT_LOCK))) {
                        break;
                    }

                    /* Check that we haven't timed out. */
                    AMS_ABORT_UNLESS(timer.Update());
                }
            }
        }

        void InitializeLegacyTmClk() {
            /* Check that we have registers we can write to. */
            AMS_ABORT_UNLESS(g_clkrst_registers_address != 0);

            /* Configure the legacy tm clock as 12MHz. */
            ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC_LEGACY_TM, CLK_RST_REG_BITS_ENUM (CLK_SOURCE_LEGACY_TM_CLK_SRC,     PLLP_OUT0),
                                                                                                        CLK_RST_REG_BITS_VALUE(CLK_SOURCE_LEGACY_TM_CLK_DIVISOR,        66));

            /* Enable clock to the legacy tm. */
            ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_ENB_Y_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_Y_CLK_ENB_LEGACY_TM, ENABLE));
        }

        bool IsResetReleased(Module module) {
            /* Check that we have registers we can write to. */
            AMS_ABORT_UNLESS(g_clkrst_registers_address != 0);

            /* Get the reset bit from RST_DEVICES_* */
            switch (module) {
                case Module_Sdmmc1: return ams::reg::HasValue(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_REG_BITS_ENUM(RST_DEVICES_L_SWR_SDMMC1_RST, DISABLE));
                case Module_Sdmmc2: return ams::reg::HasValue(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_REG_BITS_ENUM(RST_DEVICES_L_SWR_SDMMC2_RST, DISABLE));
                case Module_Sdmmc3: return ams::reg::HasValue(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEVICES_U, CLK_RST_REG_BITS_ENUM(RST_DEVICES_U_SWR_SDMMC3_RST, DISABLE));
                case Module_Sdmmc4: return ams::reg::HasValue(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_REG_BITS_ENUM(RST_DEVICES_L_SWR_SDMMC4_RST, DISABLE));
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        void SetReset(Module module) {
            /* Check that we have registers we can write to. */
            AMS_ABORT_UNLESS(g_clkrst_registers_address != 0);

            /* Set reset in RST_DEV_*_SET */
            switch (module) {
                case Module_Sdmmc1: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEV_L_SET, CLK_RST_REG_BITS_ENUM(RST_DEV_L_SDMMC1_RST, ENABLE));
                case Module_Sdmmc2: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEV_L_SET, CLK_RST_REG_BITS_ENUM(RST_DEV_L_SDMMC2_RST, ENABLE));
                case Module_Sdmmc3: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEV_U_SET, CLK_RST_REG_BITS_ENUM(RST_DEV_U_SDMMC3_RST, ENABLE));
                case Module_Sdmmc4: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEV_L_SET, CLK_RST_REG_BITS_ENUM(RST_DEV_L_SDMMC4_RST, ENABLE));
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        void ClearReset(Module module) {
            /* Check that we have registers we can write to. */
            AMS_ABORT_UNLESS(g_clkrst_registers_address != 0);

            /* Set reset in RST_DEV_*_CLR */
            switch (module) {
                case Module_Sdmmc1: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEV_L_CLR, CLK_RST_REG_BITS_ENUM(RST_DEV_L_SDMMC1_RST, ENABLE));
                case Module_Sdmmc2: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEV_L_CLR, CLK_RST_REG_BITS_ENUM(RST_DEV_L_SDMMC2_RST, ENABLE));
                case Module_Sdmmc3: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEV_U_CLR, CLK_RST_REG_BITS_ENUM(RST_DEV_U_SDMMC3_RST, ENABLE));
                case Module_Sdmmc4: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEV_L_CLR, CLK_RST_REG_BITS_ENUM(RST_DEV_L_SDMMC4_RST, ENABLE));
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        bool IsClockEnabled(Module module) {
            /* Check that we have registers we can write to. */
            AMS_ABORT_UNLESS(g_clkrst_registers_address != 0);

            /* Get the enable bit from CLK_OUT_ENB_* */
            switch (module) {
                case Module_Sdmmc1: return ams::reg::HasValue(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_REG_BITS_ENUM(CLK_OUT_ENB_L_CLK_ENB_SDMMC1, ENABLE));
                case Module_Sdmmc2: return ams::reg::HasValue(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_REG_BITS_ENUM(CLK_OUT_ENB_L_CLK_ENB_SDMMC2, ENABLE));
                case Module_Sdmmc3: return ams::reg::HasValue(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_OUT_ENB_U, CLK_RST_REG_BITS_ENUM(CLK_OUT_ENB_U_CLK_ENB_SDMMC3, ENABLE));
                case Module_Sdmmc4: return ams::reg::HasValue(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_REG_BITS_ENUM(CLK_OUT_ENB_L_CLK_ENB_SDMMC4, ENABLE));
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        void SetClockEnable(Module module) {
            /* Check that we have registers we can write to. */
            AMS_ABORT_UNLESS(g_clkrst_registers_address != 0);

            /* Set clock enable bit in CLK_ENB_*_SET */
            switch (module) {
                case Module_Sdmmc1: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_ENB_L_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_L_CLK_ENB_SDMMC1, ENABLE));
                case Module_Sdmmc2: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_ENB_L_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_L_CLK_ENB_SDMMC2, ENABLE));
                case Module_Sdmmc3: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_ENB_U_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_U_CLK_ENB_SDMMC3, ENABLE));
                case Module_Sdmmc4: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_ENB_L_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_L_CLK_ENB_SDMMC4, ENABLE));
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        void ClearClockEnable(Module module) {
            /* Check that we have registers we can write to. */
            AMS_ABORT_UNLESS(g_clkrst_registers_address != 0);

            /* Set clock enable bit in CLK_ENB_*_CLR */
            switch (module) {
                case Module_Sdmmc1: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_ENB_L_CLR, CLK_RST_REG_BITS_ENUM(CLK_ENB_L_CLK_ENB_SDMMC1, ENABLE));
                case Module_Sdmmc2: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_ENB_L_CLR, CLK_RST_REG_BITS_ENUM(CLK_ENB_L_CLK_ENB_SDMMC2, ENABLE));
                case Module_Sdmmc3: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_ENB_U_CLR, CLK_RST_REG_BITS_ENUM(CLK_ENB_U_CLK_ENB_SDMMC3, ENABLE));
                case Module_Sdmmc4: return ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_ENB_L_CLR, CLK_RST_REG_BITS_ENUM(CLK_ENB_L_CLK_ENB_SDMMC4, ENABLE));
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        void SetClockSourceSdmmc(u32 *out_actual_frequency_khz, Module module, u32 target_frequency_khz) {
            /* Check that we can write to output. */
            AMS_ABORT_UNLESS(out_actual_frequency_khz != nullptr);

            /* Determine frequency/divisor. */
            u32 clk_m = ams::reg::Encode(CLK_RST_REG_BITS_ENUM(CLK_SOURCE_SDMMCX_SDMMCX_CLK_SRC, PLLP_OUT0));
            u8 n;
            switch (target_frequency_khz) {
                case 25'000:
                    *out_actual_frequency_khz = 24'728;
                    n                         = 31;
                    break;
                case 26'000:
                    *out_actual_frequency_khz = 25'500;
                    n                         = 30;
                    break;
                case 40'800:
                    *out_actual_frequency_khz = 40'800;
                    n                         = 18;
                    break;
                case 50'000:
                    *out_actual_frequency_khz = 48'000;
                    n                         = 15;
                    break;
                case 52'000:
                    *out_actual_frequency_khz = 51'000;
                    n                         = 14;
                    break;
                case 100'000:
                    #if defined(AMS_SDMMC_SET_PLLC4_BASE)
                    *out_actual_frequency_khz = 99'840;
                    n                         = 2;
                    clk_m                     = ams::reg::Encode(CLK_RST_REG_BITS_ENUM(CLK_SOURCE_SDMMCX_SDMMCX_CLK_SRC, PLLC4_OUT2));
                    #else
                    *out_actual_frequency_khz = 90'667;
                    n                         = 7;
                    #endif
                    break;
                case 200'000:
                    #if defined(AMS_SDMMC_SET_PLLC4_BASE)
                    *out_actual_frequency_khz = 199'680;
                    n                         = 0;
                    if (module == Module_Sdmmc2 || module == Module_Sdmmc4) {
                        clk_m = ams::reg::Encode(CLK_RST_REG_BITS_ENUM(CLK_SOURCE_SDMMC24_SDMMC24_CLK_SRC, PLLC4_OUT2_LJ));
                    } else {
                        clk_m = ams::reg::Encode(CLK_RST_REG_BITS_ENUM(CLK_SOURCE_SDMMCX_SDMMCX_CLK_SRC,   PLLC4_OUT2));
                    }
                    #else
                    *out_actual_frequency_khz = 163'200;
                    n                         = 3;
                    #endif
                    break;
                case 208'000:
                    *out_actual_frequency_khz = 204'000;
                    n                         = 2;
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            /* Set frequencies in module info. */
            g_module_infos[module].target_frequency_khz = target_frequency_khz;
            g_module_infos[module].actual_frequency_khz = *out_actual_frequency_khz;

            /* Check that we have registers we can write to. */
            AMS_ABORT_UNLESS(g_clkrst_registers_address != 0);

            /* Update the clock source. */
            switch (module) {
                case Module_Sdmmc1: ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC1, clk_m | static_cast<u32>(n)); break;
                case Module_Sdmmc2: ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC2, clk_m | static_cast<u32>(n)); break;
                case Module_Sdmmc3: ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC3, clk_m | static_cast<u32>(n)); break;
                case Module_Sdmmc4: ams::reg::Write(g_clkrst_registers_address + CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC4, clk_m | static_cast<u32>(n)); break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        void EnsureControl(Module module) {
            /* Read from RST_DEVICES_* to be sure previous configuration takes. */
            switch (module) {
                case Module_Sdmmc1: ams::reg::Read(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEVICES_L); break;
                case Module_Sdmmc2: ams::reg::Read(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEVICES_L); break;
                case Module_Sdmmc3: ams::reg::Read(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEVICES_U); break;
                case Module_Sdmmc4: ams::reg::Read(g_clkrst_registers_address + CLK_RST_CONTROLLER_RST_DEVICES_L); break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

    }

    void Initialize(Module module) {
        /* Initialization isn't module specific. */
        AMS_UNUSED(module);

        /* Acquire exclusive access to the initialization lock. */
        AMS_SDMMC_LOCK_INIT_MUTEX();

        /* If we've already initialized, we don't need to do anything. */
        if (g_is_initialized) {
            return;
        }

        /* Clear module infos. */
        std::memset(g_module_infos, 0, sizeof(g_module_infos));

        /* Get the registers address. */
        g_clkrst_registers_address = dd::QueryIoMapping(ClockResetControllerRegistersPhysicalAddress, ClockResetControllerRegistersSize);
        AMS_ABORT_UNLESS(g_clkrst_registers_address != 0);

        /* Perform register initialization. */
        #if defined(AMS_SDMMC_SET_PLLC4_BASE)
        InitializePllc4();
        #endif
        InitializeLegacyTmClk();

        /* Mark that we've initialized. */
        g_is_initialized = true;
    }

    void Finalize(Module module) {
        /* Nothing is needed for finalization. */
        AMS_UNUSED(module);
    }

    bool IsAvailable(Module module) {
        return IsResetReleased(module) && IsClockEnabled(module);
    }

    void SetClockFrequencyKHz(u32 *out_actual_frequency_khz, Module module, u32 target_frequency_khz) {
        /* If we're not changing the clock frequency, we don't need to do anything. */
        if (target_frequency_khz == g_module_infos[module].target_frequency_khz) {
            *out_actual_frequency_khz = g_module_infos[module].actual_frequency_khz;
            return;
        }

        /* Temporarily disable clock. */
        const bool clock_enabled = IsClockEnabled(module);
        if (clock_enabled) {
            ClearClockEnable(module);
        }

        /* Set the clock source. */
        SetClockSourceSdmmc(out_actual_frequency_khz, module, target_frequency_khz);

        /* Re-enable clock, if we should. */
        if (clock_enabled) {
            SetClockEnable(module);
        }

        /* Ensure that our configuration takes. */
        EnsureControl(module);
    }

    void AssertReset(Module module) {
        /* Set reset and disable clock. */
        SetReset(module);
        ClearClockEnable(module);

        /* Ensure that our configuration takes. */
        EnsureControl(module);
    }

    void ReleaseReset(Module module, u32 target_frequency_khz) {
        /* Disable clock if it's enabled. */
        if (IsClockEnabled(module)) {
            ClearClockEnable(module);
        }

        /* Set reset. */
        SetReset(module);

        /* Set the clock source. */
        u32 actual_source_frequency_khz;
        SetClockSourceSdmmc(std::addressof(actual_source_frequency_khz), module, target_frequency_khz);

        /* Enable clock. */
        SetClockEnable(module);

        /* Ensure that our configuration takes. */
        EnsureControl(module);

        /* Wait 100 clocks. */
        WaitClocks(100, actual_source_frequency_khz);

        /* Clear reset. */
        ClearReset(module);

        /* Ensure that our configuration takes. */
        EnsureControl(module);
    }

}
