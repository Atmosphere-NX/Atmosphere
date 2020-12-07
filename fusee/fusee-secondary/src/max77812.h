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

#ifndef FUSEE_MAX77812_H_
#define FUSEE_MAX77812_H_

#define MAX77812_REG_RESET          0x00
#define MAX77812_REG_INT_SRC        0x01
#define MAX77812_REG_INT_SRC_M      0x02
#define MAX77812_REG_TOPSYS_INT     0x03
#define MAX77812_REG_TOPSYS_INT_M   0x04
#define MAX77812_REG_TOPSYS_STAT    0x05
#define MAX77812_REG_EN_CTRL        0x06
#define MAX77812_REG_STUP_DLY1      0x07
#define MAX77812_REG_STUP_DLY2      0x08
#define MAX77812_REG_STUP_DLY3      0x09
#define MAX77812_REG_SHDN_DLY1      0x0A
#define MAX77812_REG_SHDN_DLY2      0x0B
#define MAX77812_REG_SHDN_DLY3      0x0C
#define MAX77812_REG_SHDN_DLY4      0x0D
#define MAX77812_REG_WDTRSTB_DEB    0x0E
#define MAX77812_REG_GPI_FUNC       0x0F
#define MAX77812_REG_GPI_DEB1       0x10
#define MAX77812_REG_GPI_DEB2       0x11
#define MAX77812_REG_GPI_PD_CTRL    0x12
#define MAX77812_REG_PROT_CFG       0x13
#define MAX77812_REG_I2C_CFG        0x15
#define MAX77812_REG_BUCK_INT       0x20
#define MAX77812_REG_BUCK_INT_M     0x21
#define MAX77812_REG_BUCK_STAT      0x22
#define MAX77812_REG_M1_VOUT        0x23
#define MAX77812_REG_M2_VOUT        0x24
#define MAX77812_REG_M3_VOUT        0x25
#define MAX77812_REG_M4_VOUT        0x26
#define MAX77812_REG_M1_VOUT_D      0x27
#define MAX77812_REG_M2_VOUT_D      0x28
#define MAX77812_REG_M3_VOUT_D      0x29
#define MAX77812_REG_M4_VOUT_D      0x2A
#define MAX77812_REG_M1_VOUT_S      0x2B
#define MAX77812_REG_M2_VOUT_S      0x2C
#define MAX77812_REG_M3_VOUT_S      0x2D
#define MAX77812_REG_M4_VOUT_S      0x2E
#define MAX77812_REG_M1_CGF         0x2F
#define MAX77812_REG_M2_CGF         0x30
#define MAX77812_REG_M3_CGF         0x31
#define MAX77812_REG_M4_CGF         0x32
#define MAX77812_REG_GLB_CFG1       0x33
#define MAX77812_REG_GLB_CFG2       0x34
#define MAX77812_REG_GLB_CFG3       0x35

#define MAX77812_REG_EN_CTRL_MASK(n)        BIT(n)
#define MAX77812_START_SLEW_RATE_MASK       0x07
#define MAX77812_SHDN_SLEW_RATE_MASK        0x70
#define MAX77812_RAMPDOWN_SLEW_RATE_MASK    0x07
#define MAX77812_RAMPUP_SLEW_RATE_MASK      0x70

#define MAX77812_VOUT_MASK          0xFF
#define MAX77812_VOUT_N_VOLTAGE     0xFF
#define MAX77812_VOUT_VMIN          250000
#define MAX77812_VOUT_VMAX          1525000
#define MAX77812_VOUT_STEP          5000

#define MAX77812_REGULATOR_ID_M1    0
#define MAX77812_REGULATOR_ID_M2    1
#define MAX77812_REGULATOR_ID_M3    2
#define MAX77812_REGULATOR_ID_M4    3

#endif
