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
 
#include "mc.h"
#include "car.h"
#include "timers.h"

void mc_enable_for_tsec()
{
    volatile tegra_car_t *car = car_get_regs();
    
    /* Set EMC clock source. */
    car->clk_source_emc = ((car->clk_source_emc & 0x1FFFFFFF) | 0x40000000);
    
    /* Enable MIPI CAL clock. */
    car->clk_enb_h_set = ((car->clk_enb_h_set & 0xFDFFFFFF) | 0x2000000);
    
    /* Enable MC clock. */
    car->clk_enb_h_set = ((car->clk_enb_h_set & 0xFFFFFFFE) | 1);

    /* Enable EMC DLL clock. */
    car->clk_enb_x_set = ((car->clk_enb_x_set & 0xFFFFBFFF) | 0x4000);
    
    /* Clear EMC and MC reset. */
    /* NOTE: [4.0.0+] This was changed to use the right register. */
    /* car->rst_dev_h_set = 0x2000001; */
    car->rst_dev_h_clr = 0x2000001; 
    udelay(5);

    /* Enable AHB redirect, weird boundaries for new TSEC firmware. */
    car->lvl2_clk_gate_ovrd = ((car->lvl2_clk_gate_ovrd & 0xFFF7FFFF) | 0x80000);
    
    MAKE_MC_REG(MC_IRAM_REG_CTRL) &= 0xFFFFFFFE;
    MAKE_MC_REG(MC_IRAM_BOM) = 0x40000000;
    MAKE_MC_REG(MC_IRAM_TOM) = 0x80000000;
}