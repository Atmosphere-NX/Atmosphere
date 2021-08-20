/*
 * Copyright (c) 2018 naehrwert
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

#include "tsec.h"
#include "di.h"
#include "timers.h"
#include "car.h"
#include "mc.h"

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

void tsec_enable_clkrst()
{
    /* Enable all devices used by TSEC. */
    clkrst_reboot(CARDEVICE_HOST1X);
    clkrst_reboot(CARDEVICE_TSEC);
    clkrst_reboot(CARDEVICE_SOR_SAFE);
    clkrst_reboot(CARDEVICE_SOR0);
    clkrst_reboot(CARDEVICE_SOR1);
    clkrst_reboot(CARDEVICE_KFUSE);
}

void tsec_disable_clkrst()
{
    /* Disable all devices used by TSEC. */
    clkrst_disable(CARDEVICE_KFUSE);
    clkrst_disable(CARDEVICE_SOR1);
    clkrst_disable(CARDEVICE_SOR0);
    clkrst_disable(CARDEVICE_SOR_SAFE);
    clkrst_disable(CARDEVICE_TSEC);
    clkrst_disable(CARDEVICE_HOST1X);
}

static int tsec_run_fw_impl(const void *tsec_fw, size_t tsec_fw_size)
{
    volatile tegra_tsec_t *tsec = tsec_get_regs();

    /* Enable clocks. */
    tsec_enable_clkrst();

    /* Make sure KFUSE is ready. */
    if (!tsec_kfuse_wait_ready())
    {
        /* Disable clocks. */
        tsec_disable_clkrst();

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
        tsec_disable_clkrst();

        return -2;
    }

    /* Load firmware. */
    tsec->TSEC_FALCON_DMATRFBASE = (uint32_t)tsec_fw >> 8;
    for (uint32_t addr = 0; addr < tsec_fw_size; addr += 0x100)
    {
        if (!tsec_dma_phys_to_flcn(true, addr, addr))
        {
            /* Disable clocks. */
            tsec_disable_clkrst();

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

    /* Make sure the DMA block is idle. */
    if (!tsec_dma_wait_idle())
    {
        /* Disable clocks. */
        tsec_disable_clkrst();

        return -4;
    }

    uint32_t timeout = (get_time_ms() + 4000);
    while (!(tsec->TSEC_FALCON_CPUCTL & 0x10)) {
        if (get_time_ms() > timeout) {
            /* Disable clocks. */
            tsec_disable_clkrst();

            return -5;
        }
    }

    if (tsec->TSEC_FALCON_MAILBOX1 != 0xB0B0B0B0)
    {
        /* Disable clocks. */
        tsec_disable_clkrst();

        return -6;
    }

    /* Clear magic value from HOST1X scratch register. */
    MAKE_HOST1X_REG(0x3300) = 0;

    return 0;
}

int tsec_run_fw(const void *tsec_fw, size_t tsec_fw_size) {
    /* Ensure that the ahb redirect is enabled. */
    mc_enable_ahb_redirect();

    /* Get bom/tom */
    uint32_t bom = MAKE_MC_REG(MC_IRAM_BOM);
    uint32_t tom = MAKE_MC_REG(MC_IRAM_TOM);

    /* Override the ahb redirect extents. */
    MAKE_MC_REG(MC_IRAM_BOM) = 0x40000000;
    MAKE_MC_REG(MC_IRAM_TOM) = 0x80000000;

    /* Run the fw. */
    int res = tsec_run_fw_impl(tsec_fw, tsec_fw_size);

    /* Reset the ahb redirect extents. */
    MAKE_MC_REG(MC_IRAM_BOM) = bom;
    MAKE_MC_REG(MC_IRAM_TOM) = tom;

    return res;
}