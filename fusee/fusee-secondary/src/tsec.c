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

int tsec_get_key(uint8_t *key, uint32_t rev, const void *tsec_fw, size_t tsec_fw_size)
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
    tsec->TSEC_FALCON_MAILBOX0 = rev;
    tsec->TSEC_FALCON_BOOTVEC = 0;
    tsec->TSEC_FALCON_CPUCTL = 2;
    
    /* Make sure the DMA block is idle. */
    if (!tsec_dma_wait_idle())
    {
        /* Disable clocks. */
        tsec_disable_clkrst();
    
        return -4;
    }
    
    uint32_t timeout = (get_time_ms() + 2000);
    while (!tsec->TSEC_FALCON_MAILBOX1)
    {
        if (get_time_ms() > timeout)
        {
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

    /* Fetch result from SOR1. */
    uint32_t tmp[0x4] = {0};
    tmp[0] = SOR1_DP_HDCP_BKSV_LSB;
    tmp[1] = SOR1_TMDS_HDCP_BKSV_LSB;
    tmp[2] = SOR1_TMDS_HDCP_CN_MSB;
    tmp[3] = SOR1_TMDS_HDCP_CN_LSB;
    
    /* Clear SOR1 registers. */
    SOR1_DP_HDCP_BKSV_LSB = 0;
    SOR1_TMDS_HDCP_BKSV_LSB = 0;
    SOR1_TMDS_HDCP_CN_MSB = 0;
    SOR1_TMDS_HDCP_CN_LSB = 0;
    
    /* Copy back the key. */
    memcpy(key, &tmp, 0x10);

    return 0;
}

int tsec_load_fw(const void *tsec_fw, size_t tsec_fw_size)
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

    return 0;
}

void tsec_run_fw()
{
    volatile tegra_tsec_t *tsec = tsec_get_regs();
    
    /* Write magic value to HOST1X scratch register. */
    MAKE_HOST1X_REG(0x3300) = 0x34C2E1DA;
    
    /* Execute firmware. */
    tsec->TSEC_FALCON_MAILBOX1 = 0;
    tsec->TSEC_FALCON_MAILBOX0 = 1;
    tsec->TSEC_FALCON_BOOTVEC = 0;
    tsec->TSEC_FALCON_CPUCTL = 2;
}