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
#include <exosphere.hpp>
#include "fusee_secure_initialize.hpp"
#include "fusee_registers_di.hpp"

namespace ams::nxboot {

    namespace {

        constexpr inline const uintptr_t CLKRST = secmon::MemoryRegionPhysicalDeviceClkRst.GetAddress();
        constexpr inline const uintptr_t PMC    = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();
        constexpr inline const uintptr_t AHB    = AHB_ARBC(0);
        constexpr inline const uintptr_t I2S    = I2S_REG(0);
        constexpr inline const uintptr_t DISP1  = secmon::MemoryRegionPhysicalDeviceDisp1.GetAddress();
        constexpr inline const uintptr_t VIC    = secmon::MemoryRegionPhysicalDeviceDsi.GetAddress() + 0x40000;

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

    }

    void SecureInitialize() {
        /* Get SoC type. */
        const auto soc_type = fuse::GetSocType();

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

        /* TODO */
    }

}
