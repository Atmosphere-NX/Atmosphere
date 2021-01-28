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

#include <stdint.h>
#include "utils.h"
#include "car.h"
#include "timers.h"
#include "di.h"
#include "se.h"
#include "fuse.h"
#include "pmc.h"
#include "mc.h"
#include "sysreg.h"
#include "tsec.h"

#define I2S_BASE 0x702D1000
#define MAKE_I2S_REG(n) MAKE_REG32(I2S_BASE + n)

static void setup_exception_vectors(void) {
    for (unsigned int i = 0; i < 0x20; i += 4) {
        MAKE_REG32(0x6000F200u + i) = (uint32_t)generic_panic;
    }
}

static void mbist_workaround(void)
{
    volatile tegra_car_t *car = car_get_regs();

    car->clk_source_sor1 = ((car->clk_source_sor1 | 0x8000) & 0xFFFFBFFF);
    car->plld_base |= 0x40800000u;
    car->rst_dev_y_clr = 0x40;
    car->rst_dev_x_clr = 0x40000;
    car->rst_dev_l_clr = 0x18000000;
    udelay(3);

    /* Setup I2S. */
    MAKE_I2S_REG(0x0A0) |= 0x400;
    MAKE_I2S_REG(0x088) &= 0xFFFFFFFE;
    MAKE_I2S_REG(0x1A0) |= 0x400;
    MAKE_I2S_REG(0x188) &= 0xFFFFFFFE;
    MAKE_I2S_REG(0x2A0) |= 0x400;
    MAKE_I2S_REG(0x288) &= 0xFFFFFFFE;
    MAKE_I2S_REG(0x3A0) |= 0x400;
    MAKE_I2S_REG(0x388) &= 0xFFFFFFFE;
    MAKE_I2S_REG(0x4A0) |= 0x400;
    MAKE_I2S_REG(0x488) &= 0xFFFFFFFE;

    MAKE_DI_REG(DC_COM_DSC_TOP_CTL) |= 4;
    MAKE_VIC_REG(0x8C) = 0xFFFFFFFF;
    udelay(3);

    /* Set devices in reset. */
    car->rst_dev_y_set = 0x40;
    car->rst_dev_l_set = 0x18000000;
    car->rst_dev_x_set = 0x40000;

    /* Clock out enables. */
    car->clk_out_enb_h = 0xC0;
    car->clk_out_enb_l = 0x80000130;
    car->clk_out_enb_u = 0x1F00200;
    car->clk_out_enb_v = 0x80400808;
    car->clk_out_enb_w = 0x402000FC;
    car->clk_out_enb_x = 0x23000780;
    car->clk_out_enb_y = 0x300;

    /* LVL2 clock gate overrides. */
    car->lvl2_clk_gate_ovra = 0;
    car->lvl2_clk_gate_ovrb = 0;
    car->lvl2_clk_gate_ovrc = 0;
    car->lvl2_clk_gate_ovrd = 0;
    car->lvl2_clk_gate_ovre = 0;

    /* Configure clock sources. */
    car->plld_base &= 0x1F7FFFFF;
    car->clk_source_sor1 &= 0xFFFF3FFF;
    car->clk_source_vi = ((car->clk_source_vi & 0x1FFFFFFF) | 0x80000000);
    car->clk_source_host1x = ((car->clk_source_host1x & 0x1FFFFFFF) | 0x80000000);
    car->clk_source_nvenc = ((car->clk_source_nvenc & 0x1FFFFFFF) | 0x80000000);
}

static int tsec_dma_wait_idle()
{
    volatile tegra_tsec_t *tsec = tsec_get_regs();
    uint32_t timeout = (get_time_ms() + 10000);

    while (!(tsec->TSEC_FALCON_DMATRFCMD & 2))
    {
        if (get_time_ms() > timeout)
            return 0;
    }

    return 1;
}

static int tsec_dma_phys_to_flcn(bool is_imem, uint32_t flcn_offset, uint32_t phys_offset)
{
    volatile tegra_tsec_t *tsec = tsec_get_regs();
    uint32_t cmd = 0;

    if (!is_imem)
        cmd = 0x600;
    else
        cmd = 0x10;

    tsec->TSEC_FALCON_DMATRFMOFFS = flcn_offset;
    tsec->TSEC_FALCON_DMATRFFBOFFS = phys_offset;
    tsec->TSEC_FALCON_DMATRFCMD = cmd;

    return tsec_dma_wait_idle();
}

static int tsec_kfuse_wait_ready()
{
    uint32_t timeout = (get_time_ms() + 10000);

    /* Wait for STATE_DONE. */
    while (!(KFUSE_STATE & 0x10000))
    {
        if (get_time_ms() > timeout)
            return 0;
    }

    /* Check for STATE_CRCPASS. */
    if (!(KFUSE_STATE & 0x20000))
        return 0;

    return 1;
}

int load_tsec_fw(void) {
	volatile uint32_t* tsec_fw = (volatile uint32_t*)0x40010F00;
    const uint32_t tsec_fw_length = MAKE_REG32(0x40010EFC);

	volatile tegra_tsec_t *tsec = tsec_get_regs();

    /* Enable clocks. */
    clkrst_reboot(CARDEVICE_HOST1X);
    clkrst_reboot(CARDEVICE_TSEC);
    clkrst_reboot(CARDEVICE_SOR_SAFE);
    clkrst_reboot(CARDEVICE_SOR0);
    clkrst_reboot(CARDEVICE_SOR1);
    clkrst_reboot(CARDEVICE_KFUSE);

    /* Make sure KFUSE is ready. */
    if (!tsec_kfuse_wait_ready())
    {
        /* Disable clocks. */
        clkrst_disable(CARDEVICE_KFUSE);
        clkrst_disable(CARDEVICE_SOR1);
        clkrst_disable(CARDEVICE_SOR0);
        clkrst_disable(CARDEVICE_SOR_SAFE);
        clkrst_disable(CARDEVICE_TSEC);
        clkrst_disable(CARDEVICE_HOST1X);

        return -1;
    }

    /* Configure Falcon. */
    tsec->TSEC_FALCON_DMACTL = 0;
    tsec->TSEC_FALCON_IRQMSET = 0xFFF2;
    tsec->TSEC_FALCON_IRQDEST = 0xFFF0;
    tsec->TSEC_FALCON_ITFEN = 3;

    /* Make sure the DMA block is idle. */
    if (!tsec_dma_wait_idle())
    {
        /* Disable clocks. */
        clkrst_disable(CARDEVICE_KFUSE);
        clkrst_disable(CARDEVICE_SOR1);
        clkrst_disable(CARDEVICE_SOR0);
        clkrst_disable(CARDEVICE_SOR_SAFE);
        clkrst_disable(CARDEVICE_TSEC);
        clkrst_disable(CARDEVICE_HOST1X);

        return -2;
    }

    /* Load firmware. */
    tsec->TSEC_FALCON_DMATRFBASE = (uint32_t)tsec_fw >> 8;
    for (uint32_t addr = 0; addr < tsec_fw_length; addr += 0x100)
    {
        if (!tsec_dma_phys_to_flcn(true, addr, addr))
        {
            /* Disable clocks. */
            clkrst_disable(CARDEVICE_KFUSE);
            clkrst_disable(CARDEVICE_SOR1);
            clkrst_disable(CARDEVICE_SOR0);
            clkrst_disable(CARDEVICE_SOR_SAFE);
            clkrst_disable(CARDEVICE_TSEC);
            clkrst_disable(CARDEVICE_HOST1X);

            return -3;
        }
    }

    /* Write magic value to HOST1X scratch register. */
    MAKE_HOST1X_REG(0x3300) = 0x34C2E1DA;

    /* Execute firmware. */
    tsec->TSEC_FALCON_MAILBOX1 = 0;
    tsec->TSEC_FALCON_MAILBOX0 = 1;
    tsec->TSEC_FALCON_BOOTVEC = 0;
    tsec->TSEC_FALCON_CPUCTL = 2;

    while (true) {
        /* Yield to Nintendo's TSEC firmware. */
    }
}

int main(void) {
    /* Setup vectors */
    setup_exception_vectors();

    volatile tegra_pmc_t *pmc = pmc_get_regs();
    volatile tegra_car_t *car = car_get_regs();

    /* Clear the boot reason to avoid problems later */
    pmc->scratch200 = 0;
    pmc->rst_status = 0;

    //AHB_AHB_SPARE_REG_0 &= 0xFFFFFF9F;
    //pmc->scratch49 = (((pmc->scratch49 >> 1) << 1) & 0xFFFFFFFD);

    /* Apply the memory built-in self test workaround. */
    mbist_workaround();

    /* Reboot SE. */
    clkrst_reboot(CARDEVICE_SE);

    /* Initialize the fuse driver. */
    fuse_init();

    /* Don't bother checking SKU, fuses, or bootloader version. */
    mc_enable_for_tsec();

    /* 7.0.0 package1ldr holds I2C5 in reset, clears SYS clock. */
    car->clk_source_sys = 0;
    rst_enable(CARDEVICE_I2C5);

    load_tsec_fw();

    while (true) { }
    return 0;
}
