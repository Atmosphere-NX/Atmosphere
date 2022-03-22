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
#include <exosphere.hpp>
#include "fusee_secure_initialize.hpp"
#include "../fusee_registers_di.hpp"

namespace ams::nxboot {

    namespace {

        constexpr inline const uintptr_t CLKRST  = secmon::MemoryRegionPhysicalDeviceClkRst.GetAddress();
        constexpr inline const uintptr_t PMC     = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();
        constexpr inline const uintptr_t MC      = secmon::MemoryRegionPhysicalDeviceMemoryController.GetAddress();
        constexpr inline const uintptr_t APB     = secmon::MemoryRegionPhysicalDeviceApbMisc.GetAddress();
        constexpr inline const uintptr_t AHB     = AHB_ARBC(0);
        constexpr inline const uintptr_t I2S     = I2S_REG(0);
        constexpr inline const uintptr_t DISP1   = secmon::MemoryRegionPhysicalDeviceDisp1.GetAddress();
        constexpr inline const uintptr_t VIC     = secmon::MemoryRegionPhysicalDeviceDsi.GetAddress() + 0x40000;
        constexpr inline const uintptr_t TIMER   = secmon::MemoryRegionPhysicalDeviceTimer.GetAddress();
        constexpr inline const uintptr_t SYSCTR0 = secmon::MemoryRegionPhysicalDeviceSysCtr0.GetAddress();

        void DoRcmWorkaround(const void *sbk, size_t sbk_size) {
            /* Set the SBK inside the security engine. */
            se::SetAesKey(pkg1::AesKeySlot_SecureBoot, sbk, sbk_size);

            /* Lock the SBK/SSK as unreadable. */
            se::LockAesKeySlot(pkg1::AesKeySlot_SecureBoot,    se::KeySlotLockFlags_KeyRead);
            se::LockAesKeySlot(pkg1::AesKeySlot_SecureStorage, se::KeySlotLockFlags_KeyRead);

            /* Clear TZRAM. */
            std::memset(secmon::MemoryRegionPhysicalTzram.GetPointer(), 0, secmon::MemoryRegionPhysicalTzram.GetSize());

            /* Clear APBDEV_PMC_CRYPTO_OP. */
            reg::Write(PMC + APBDEV_PMC_CRYPTO_OP, 0);

            /* Clear the boot reason. */
            reg::Write(PMC + APBDEV_PMC_SCRATCH200, 0);
            reg::Write(PMC + APBDEV_PMC_CRYPTO_OP, 0);

            /* Clear OBS_OVERRIDE/APB2JTAG_OVERRIDE */
            reg::ReadWrite(AHB + AHB_AHB_SPARE_REG, AHB_REG_BITS_ENUM(AHB_SPARE_REG_OBS_OVERRIDE_EN,      DISABLE),
                                                    AHB_REG_BITS_ENUM(AHB_SPARE_REG_APB2JTAG_OVERRIDE_EN, DISABLE));

            /* Clear low bits of APBDEV_PMC_SCRATCH49 */
            reg::ClearBits(PMC + APBDEV_PMC_SCRATCH49, 0x3);
        }

        void DoMbistWorkaround() {
            /* Enable AHUB/APE clock prior to I2S accesses. */
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_V, CLK_RST_REG_BITS_ENUM(CLK_ENB_V_CLK_ENB_AHUB, ENABLE));
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_Y, CLK_RST_REG_BITS_ENUM(CLK_ENB_Y_CLK_ENB_APE,  ENABLE));

            /* Configure CLK_RST_CONTROLLER_CLK_SOURCE_SOR1. */
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_SOR1, CLK_RST_REG_BITS_ENUM(CLK_SOURCE_SOR1_SOR1_CLK_SEL0,               MUX),
                                                                        CLK_RST_REG_BITS_ENUM(CLK_SOURCE_SOR1_SOR1_CLK_SEL1, SOR1_CLOCK_SWITCH));

            /* Set CSI clock source as PLLD. */
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_PLLD_BASE, CLK_RST_REG_BITS_ENUM(PLLD_BASE_PLLD_ENABLE, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(PLLD_BASE_CSI_CLK_SRC,  PLL_D));

            /* Clear APE, VIC, HOST1X, DISP1 reset. */
            reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_DEV_Y_CLR, CLK_RST_REG_BITS_ENUM(RST_DEV_Y_APE_RST, ENABLE));
            reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_DEV_X_CLR, CLK_RST_REG_BITS_ENUM(RST_DEV_X_VIC_RST, ENABLE));
            reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_DEV_L_CLR, CLK_RST_REG_BITS_ENUM(RST_DEV_L_DISP1_RST, ENABLE), CLK_RST_REG_BITS_ENUM(RST_DEV_L_HOST1X_RST, ENABLE));

            /* Wait two microseconds for the devices to come out of reset. */
            util::WaitMicroSeconds(2);

            /* Set I2S_CTRL.MASTER and clear I2S_CG.SCLG_ENABLE for all I2S registers. */
            reg::ReadWrite(I2S + I2S0_I2S_CTRL, I2S_REG_BITS_ENUM(I2S_CTRL_MASTER,    ENABLE));
            reg::ReadWrite(I2S + I2S0_I2S_CG,   I2S_REG_BITS_ENUM(I2S_CG_SLCG_ENABLE,  FALSE));
            reg::ReadWrite(I2S + I2S1_I2S_CTRL, I2S_REG_BITS_ENUM(I2S_CTRL_MASTER,    ENABLE));
            reg::ReadWrite(I2S + I2S1_I2S_CG,   I2S_REG_BITS_ENUM(I2S_CG_SLCG_ENABLE,  FALSE));
            reg::ReadWrite(I2S + I2S2_I2S_CTRL, I2S_REG_BITS_ENUM(I2S_CTRL_MASTER,    ENABLE));
            reg::ReadWrite(I2S + I2S2_I2S_CG,   I2S_REG_BITS_ENUM(I2S_CG_SLCG_ENABLE,  FALSE));
            reg::ReadWrite(I2S + I2S3_I2S_CTRL, I2S_REG_BITS_ENUM(I2S_CTRL_MASTER,    ENABLE));
            reg::ReadWrite(I2S + I2S3_I2S_CG,   I2S_REG_BITS_ENUM(I2S_CG_SLCG_ENABLE,  FALSE));
            reg::ReadWrite(I2S + I2S4_I2S_CTRL, I2S_REG_BITS_ENUM(I2S_CTRL_MASTER,    ENABLE));
            reg::ReadWrite(I2S + I2S4_I2S_CG,   I2S_REG_BITS_ENUM(I2S_CG_SLCG_ENABLE,  FALSE));

            /* Set DC_COM_DSC_TOP_CTL.DSC_SLG_OVERRIDE */
            reg::SetBits(DISP1 + DC_COM_DSC_TOP_CTL * sizeof(u32), 0x4);

            /* Set NV_PVIC_THI_SLCG_OVERRIDE_LOW_A */
            reg::SetBits(VIC +  NV_PVIC_THI_SLCG_OVERRIDE_LOW_A, 0xFFFFFFFF);

            /* Wait two microseconds for configuration to take. */
            util::WaitMicroSeconds(2);

            /* Set APE, VIC, HOST1X, DISP1 reset. */
            reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_DEV_Y_SET, CLK_RST_REG_BITS_ENUM(RST_DEV_Y_APE_RST, ENABLE));
            reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_DEV_L_SET, CLK_RST_REG_BITS_ENUM(RST_DEV_L_DISP1_RST, ENABLE), CLK_RST_REG_BITS_ENUM(RST_DEV_L_HOST1X_RST, ENABLE));
            reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_DEV_X_SET, CLK_RST_REG_BITS_ENUM(RST_DEV_X_VIC_RST, ENABLE));

            /* Set clock enable for a select few devices. */
            reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_H, CLK_RST_REG_BITS_ENUM(CLK_ENB_H_CLK_ENB_FUSE, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_H_CLK_ENB_PMC,  ENABLE));

            reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_L, CLK_RST_REG_BITS_ENUM(CLK_ENB_L_CLK_ENB_CACHE2, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_L_CLK_ENB_GPIO,   ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_L_CLK_ENB_TMR,    ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_L_CLK_ENB_RTC,    ENABLE));

            reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_U, CLK_RST_REG_BITS_ENUM(CLK_ENB_U_CLK_ENB_CRAM2, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_U_CLK_ENB_IRAMD, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_U_CLK_ENB_IRAMC, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_U_CLK_ENB_IRAMB, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_U_CLK_ENB_IRAMA, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_U_CLK_ENB_CSITE, ENABLE));

            reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_V, CLK_RST_REG_BITS_ENUM(CLK_ENB_V_CLK_ENB_SE,            ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_V_CLK_ENB_SPDIF_DOUBLER, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_V_CLK_ENB_APB2APE,       ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_V_CLK_ENB_MSELECT,       ENABLE));

            reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_W, CLK_RST_REG_BITS_ENUM(CLK_ENB_W_CLK_ENB_MC1,     ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_W_CLK_ENB_ENTROPY, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_W_CLK_ENB_PCIERX5, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_W_CLK_ENB_PCIERX4, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_W_CLK_ENB_PCIERX3, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_W_CLK_ENB_PCIERX2, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_W_CLK_ENB_PCIERX1, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_W_CLK_ENB_PCIERX0, ENABLE));

            reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_X, CLK_RST_REG_BITS_ENUM(CLK_ENB_X_CLK_ENB_PLLG_REF, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_X_CLK_ENB_DBGAPB,   ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_X_CLK_ENB_GPU,      ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_X_CLK_ENB_MC_BBC,   ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_X_CLK_ENB_MC_CPU,   ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_X_CLK_ENB_MC_CBPA,  ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_X_CLK_ENB_MC_CAPA,  ENABLE));

            reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_OUT_ENB_Y, CLK_RST_REG_BITS_ENUM(CLK_ENB_Y_CLK_ENB_MC_CDPA, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(CLK_ENB_Y_CLK_ENB_MC_CCPA, ENABLE));

            /* Clear all LVL2 clock gate overrides to zero. */
            reg::Write(CLKRST + CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRA, 0);
            reg::Write(CLKRST + CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRB, 0);
            reg::Write(CLKRST + CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRC, 0);
            reg::Write(CLKRST + CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD, 0);
            reg::Write(CLKRST + CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRE, 0);

            /* Reset CSI clock source. */
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_PLLD_BASE, CLK_RST_REG_BITS_ENUM(PLLD_BASE_PLLD_BYPASS,     DISABLE),
                                                                  CLK_RST_REG_BITS_ENUM(PLLD_BASE_PLLD_ENABLE,     DISABLE),
                                                                  CLK_RST_REG_BITS_ENUM(PLLD_BASE_PLLD_REF_DIS, REF_ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(PLLD_BASE_CSI_CLK_SRC,       BRICK));

            /* Configure CLK_RST_CONTROLLER_CLK_SOURCE_SOR1. */
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_SOR1, CLK_RST_REG_BITS_ENUM(CLK_SOURCE_SOR1_SOR1_CLK_SEL0,        MUX),
                                                                        CLK_RST_REG_BITS_ENUM(CLK_SOURCE_SOR1_SOR1_CLK_SEL1, SAFE_CLOCK));

            /* Configure VI, HOST1X, NVENC clock sources. */
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_VI,     CLK_RST_REG_BITS_ENUM(CLK_SOURCE_VI_VI_CLK_SRC,         PLLP_OUT0));
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_HOST1X, CLK_RST_REG_BITS_ENUM(CLK_SOURCE_HOST1X_HOST1X_CLK_SRC, PLLP_OUT0));
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_NVENC,  CLK_RST_REG_BITS_ENUM(CLK_SOURCE_NVENC_NVENC_CLK_SRC,   PLLP_OUT0));
        }

        void EnableArc() {
            /* Enable clocks for EMC/MC, using PLLP_OUT0. */
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC, CLK_RST_REG_BITS_ENUM(CLK_SOURCE_EMC_EMC_2X_CLK_SRC, PLLP_OUT0));
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_H_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_H_CLK_ENB_EMC, ENABLE));
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_H_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_H_CLK_ENB_MEM, ENABLE));
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_X_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_X_CLK_ENB_EMC_DLL, ENABLE));

            /* Clear reset for MEM/EMC. */
            reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_DEV_H_CLR, CLK_RST_REG_BITS_ENUM(RST_DEV_H_EMC_RST, ENABLE),
                                                                  CLK_RST_REG_BITS_ENUM(RST_DEV_H_MEM_RST, ENABLE));

            /* Wait 5 microseconds for configuration to take. */
            util::WaitMicroSeconds(5);

            /* Enable ARC_CLK_OVR_ON. */
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD, CLK_RST_REG_BITS_ENUM(LVL2_CLK_GATE_OVRD_ARC_CLK_OVR_ON, ON));

            /* Enable the ARC. */
            reg::ReadWrite(MC + MC_IRAM_REG_CTRL, MC_REG_BITS_ENUM(IRAM_REG_CTRL_IRAM_CFG_WRITE_ACCESS, ENABLED));

            /* Set IRAM BOM/TOP to open up access to all mmio. */
            reg::Write(MC + MC_IRAM_BOM, 0x40000000);
            reg::Write(MC + MC_IRAM_TOM, 0x80000000);

            /* Read to ensure our configuration takes. */
            reg::Read(MC + MC_IRAM_REG_CTRL);
        }

        void InitializeClock() {
            /* Set SPARE_REG0 clock divisor 2. */
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_SPARE_REG0, CLK_RST_REG_BITS_ENUM(SPARE_REG0_CLK_M_DIVISOR, CLK_M_DIVISOR2));

            /* Set system counter frequency. */
            reg::Write(SYSCTR0 + SYSCTR0_CNTFID0, 19'200'000);

            /* Restore TIMERUS config to 19.2 MHz. */
            reg::Write(TIMER + TIMERUS_USEC_CFG, TIMER_REG_BITS_VALUE(USEC_CFG_USEC_DIVIDEND,  5 - 1),
                                                 TIMER_REG_BITS_VALUE(USEC_CFG_USEC_DIVISOR,  96 - 1));

            /* Enable the crystal oscillator, and copy the drive strength from pmc. */
            reg::Write(CLKRST + CLK_RST_CONTROLLER_OSC_CTRL, CLK_RST_REG_BITS_ENUM (OSC_CTRL_OSC_FREQ, OSC38P4),
                                                             CLK_RST_REG_BITS_ENUM (OSC_CTRL_XOE,       ENABLE),
                                                             CLK_RST_REG_BITS_VALUE(OSC_CTRL_XOFS,           7));

            /* Set the crystal oscillator value in PMC. */
            reg::ReadWrite(PMC + APBDEV_PMC_OSC_EDPD_OVER, PMC_REG_BITS_VALUE(OSC_EDPD_OVER_XOFS, 7));

            /* Configure the crystal oscillator to use PMC value on warmboot. */
            reg::ReadWrite(PMC + APBDEV_PMC_OSC_EDPD_OVER, PMC_REG_BITS_ENUM(OSC_EDPD_OVER_OSC_CTRL_SELECT, PMC));

            /* Set HOLD_CKE_LOW_EN. */
            reg::ReadWrite(PMC + APBDEV_PMC_CNTRL2, PMC_REG_BITS_ENUM(CNTRL2_HOLD_CKE_LOW_EN, ENABLE));

            /* Set CFG2TMC_RAM_SVOP_PDP to 2. */
            /* NOTE: Nintendo acidentally writes this to the PMC instead of the APB due to a bug. */
            reg::ReadWrite(APB + APB_MISC_GP_ASDBGREG, APB_MISC_REG_BITS_VALUE(GP_ASDBGREG_CFG2TMC_RAM_SVOP_PDP, 2));

            /* Set CLK_SYSTEM_RATE. */
            reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_SYSTEM_RATE, CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_HCLK_DIS, 0),
                                                                    CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_AHB_RATE, 1),
                                                                    CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_PCLK_DIS, 0),
                                                                    CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_APB_RATE, 0));

            /* Configure PLLMB_BASE. */
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_PLLMB_BASE, CLK_RST_REG_BITS_ENUM(PLLMB_BASE_PLLMB_ENABLE, DISABLE));

            /* Configure TSC_MULT. */
            constexpr u32 TscMultValue = 19'200'000 * 16 / 32768;
            reg::ReadWrite(PMC + APBDEV_PMC_TSC_MULT, PMC_REG_BITS_VALUE(TSC_MULT_MULT_VAL, TscMultValue));

            /* Configure SCLK_BURST_POLICY */
            reg::Write(CLKRST + CLK_RST_CONTROLLER_SCLK_BURST_POLICY, CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SYS_STATE,                       RUN),
                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_COP_AUTO_SWAKEUP_FROM_FIQ,       NOP),
                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_CPU_AUTO_SWAKEUP_FROM_FIQ,       NOP),
                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_COP_AUTO_SWAKEUP_FROM_IRQ,       NOP),
                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_CPU_AUTO_SWAKEUP_FROM_IRQ,       NOP),
                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_FIQ_SOURCE,        PLLP_OUT2),
                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_IRQ_SOURCE,        PLLP_OUT2),
                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_RUN_SOURCE,        PLLP_OUT2),
                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_IDLE_SOURCE,       PLLP_OUT2));

            /* Configure SUPER_SCLK_DIVIDER */
            reg::Write(CLKRST + CLK_RST_CONTROLLER_SUPER_SCLK_DIVIDER, CLK_RST_REG_BITS_ENUM (SUPER_SCLK_DIVIDER_SUPER_SDIV_ENB,              ENABLE),
                                                                       CLK_RST_REG_BITS_ENUM (SUPER_SCLK_DIVIDER_SUPER_SDIV_DIS_FROM_COP_FIQ,    NOP),
                                                                       CLK_RST_REG_BITS_ENUM (SUPER_SCLK_DIVIDER_SUPER_SDIV_DIS_FROM_CPU_FIQ,    NOP),
                                                                       CLK_RST_REG_BITS_ENUM (SUPER_SCLK_DIVIDER_SUPER_SDIV_DIS_FROM_COP_IRQ,    NOP),
                                                                       CLK_RST_REG_BITS_ENUM (SUPER_SCLK_DIVIDER_SUPER_SDIV_DIS_FROM_CPU_IRQ,    NOP),
                                                                       CLK_RST_REG_BITS_VALUE(SUPER_SCLK_DIVIDER_SUPER_SDIV_DIVIDEND,              0),
                                                                       CLK_RST_REG_BITS_VALUE(SUPER_SCLK_DIVIDER_SUPER_SDIV_DIVISOR,               0));

            /* Set CLK_SYSTEM_RATE */
            reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_SYSTEM_RATE, CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_HCLK_DIS, 0),
                                                                    CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_AHB_RATE, 0),
                                                                    CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_PCLK_DIS, 0),
                                                                    CLK_RST_REG_BITS_VALUE(CLK_SYSTEM_RATE_APB_RATE, 2));
        }

        void InitializePinmux(fuse::HardwareType hw_type) {
            /* Clear global pinmux control register */
            reg::Write(APB + APB_MISC_PP_PINMUX_GLOBAL_0, 0);

            /* Perform initial pinmux setup. */
            pinmux::SetupFirst(hw_type);

            /* Setup important pinmux devices. */
            pinmux::SetupI2c1();
            pinmux::SetupI2c5();
            pinmux::SetupUartA();
            pinmux::SetupVolumeButton();
            pinmux::SetupHomeButton();
        }

    }

    void SecureInitialize(bool enable_log) {
        /* Get SoC type/hardware type. */
        const auto soc_type = fuse::GetSocType();
        const auto hw_type  = fuse::GetHardwareType();

        /* If Erista, perform bootrom logic (to compensate for RCM exploit) and MBIST workaround. */
        if (soc_type == fuse::SocType_Erista) {
            /* Potentially perform bootrom compensation. */
            {
                u32 sbk[4];
                if (fuse::GetSecureBootKey(sbk)) {
                    DoRcmWorkaround(sbk, sizeof(sbk));
                }
            }
            DoMbistWorkaround();
        }

        /* Initialize security engine clock. */
        clkrst::EnableSeClock();

        /* Set fuse visibility. */
        clkrst::SetFuseVisibility(true);

        /* Disable fuse programming. */
        fuse::Lockout();

        /* Initialize the security engine. */
        se::Initialize();

        /* Enable the arc. */
        EnableArc();

        /* Setup initial clocks. */
        InitializeClock();

        /* Setup initial pinmux. */
        InitializePinmux(hw_type);

        /* Initialize logging. */
        if (enable_log) {
            clkrst::EnableUartAClock();
        }

        /* Enable various clocks. */
        clkrst::EnableCldvfsClock();
        clkrst::EnableI2c1Clock();
        clkrst::EnableI2c5Clock();
        clkrst::EnableTzramClock();

        /* Ensure avp cache is enabled, since we'll be using it. */
        clkrst::EnableCache2Clock();
        clkrst::EnableCram2Clock();

        /* Initialize I2C5. */
        i2c::Initialize(i2c::Port_5);

        /* Configure pmic system setting. */
        pmic::SetSystemSetting(soc_type);

        /* Enable VDD core */
        pmic::EnableVddCore(soc_type);

        /* On hoag, enable Ldo8 */
        if (hw_type == fuse::HardwareType_Hoag) {
            pmic::EnableLdo8();
        }

        /* Initialize I2C1. */
        i2c::Initialize(i2c::Port_1);

        /* Configure SCLK_BURST_POLICY. */
        reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_SCLK_BURST_POLICY, CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_FIQ_SOURCE,  PLLP_OUT0),
                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_IRQ_SOURCE,  PLLP_OUT0),
                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_RUN_SOURCE,  PLLP_OUT0),
                                                                      CLK_RST_REG_BITS_ENUM(SCLK_BURST_POLICY_SWAKEUP_IDLE_SOURCE, PLLP_OUT0));

        /* Do mariko-only TZRAM configuration. */
        if (soc_type == fuse::SocType_Mariko) {
            reg::ReadWrite(PMC + APBDEV_PMC_TZRAM_PWR_CNTRL, PMC_REG_BITS_VALUE(TZRAM_PWR_CNTRL_TZRAM_SD, 0));

            reg::Write(PMC + APBDEV_PMC_TZRAM_NON_SEC_DISABLE, PMC_REG_BITS_ENUM(TZRAM_NON_SEC_DISABLE_SD_WRITE, ON),
                                                               PMC_REG_BITS_ENUM(TZRAM_NON_SEC_DISABLE_SD_READ,  ON));

            reg::Write(PMC + APBDEV_PMC_TZRAM_SEC_DISABLE, PMC_REG_BITS_ENUM(TZRAM_SEC_DISABLE_SD_WRITE, ON),
                                                           PMC_REG_BITS_ENUM(TZRAM_SEC_DISABLE_SD_READ,  ON));
        }

        /* Hold certain devices in reset. */
        reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_RST_DEVICES_L, CLK_RST_REG_BITS_ENUM(RST_DEVICES_L_SWR_USBD_RST, ENABLE));
    }

}
