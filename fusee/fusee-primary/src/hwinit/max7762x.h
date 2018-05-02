/*
* Copyright (c) 2018 naehrwert
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

#ifndef _MAX7762X_H_
#define _MAX7762X_H_

#include "types.h"

/*
* Switch Power domains (max77620):
* Name  | Usage         | uV step | uV min | uV default | uV max  | Init
*-------+---------------+---------+--------+------------+---------+------------------
*  sd0  | core          | 12500   | 600000 |  625000    | 1400000 | 1.125V (pkg1.1)
*  sd1  | SDRAM         | 12500   | 600000 | 1125000    | 1125000 | 1.1V (pkg1.1)
*  sd2  | ldo{0-1, 7-8} | 12500   | 600000 | 1325000    | 1350000 | 1.325V (pcv)
*  sd3  | 1.8V general  | 12500   | 600000 | 1800000    | 1800000 |
*  ldo0 | Display Panel | 25000   | 800000 | 1200000    | 1200000 | 1.2V (pkg1.1)
*  ldo1 | XUSB          | 25000   | 800000 | 1050000    | 1050000 | 1.05V (pcv)
*  ldo2 | SDMMC1        | 50000   | 800000 | 1800000    | 3300000 |
*  ldo3 |               | 50000   | 800000 | 3100000    | 3100000 |
*  ldo4 | RTC           | 12500   | 800000 |  850000    |  850000 |
*  ldo5 |               | 50000   | 800000 | 1800000    | 1800000 |
*  ldo6 |               | 50000   | 800000 | 2900000    | 2900000 |
*  ldo7 | XUSB          | 50000   | 800000 | 1050000    | 1050000 |
*  ldo8 | XUSB, DC      | 50000   | 800000 | 1050000    | 1050000 |
*/

/*
* MAX77620_AME_GPIO: control GPIO modes (bits 0 - 7 correspond to GPIO0 - GPIO7); 0 -> GPIO, 1 -> alt-mode
* MAX77620_REG_GPIOx: 0x9 sets output and enable
*/

/*! MAX77620 partitions. */
#define REGULATOR_SD0 0
#define REGULATOR_SD1 1
#define REGULATOR_SD2 2
#define REGULATOR_SD3 3
#define REGULATOR_LDO0 4
#define REGULATOR_LDO1 5
#define REGULATOR_LDO2 6
#define REGULATOR_LDO3 7
#define REGULATOR_LDO4 8
#define REGULATOR_LDO5 9
#define REGULATOR_LDO6 10
#define REGULATOR_LDO7 11
#define REGULATOR_LDO8 12
#define REGULATOR_MAX 12

int max77620_regulator_get_status(u32 id);
int max77620_regulator_config_fps(u32 id);
int max77620_regulator_set_voltage(u32 id, u32 mv);
int max77620_regulator_enable(u32 id, int enable);
void max77620_config_default();

#endif
