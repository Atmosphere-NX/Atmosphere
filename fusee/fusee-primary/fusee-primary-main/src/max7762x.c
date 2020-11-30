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
 
#include <stdint.h>
 
#include "max7762x.h"
#include "max77620.h"
#include "i2c.h"
#include "timers.h"

#define REGULATOR_SD 0
#define REGULATOR_LDO 1

typedef struct _max77620_regulator_t
{
    uint8_t type;
    const char *name;
    uint8_t reg_sd;
    uint32_t mv_step;
    uint32_t mv_min;
    uint32_t mv_default;
    uint32_t mv_max;
    uint8_t volt_addr;
    uint8_t cfg_addr;
    uint8_t volt_mask;
    uint8_t enable_mask;
    uint8_t enable_shift;
    uint8_t status_mask;

    uint8_t fps_addr;
    uint8_t fps_src;
    uint8_t pd_period;
    uint8_t pu_period;
} max77620_regulator_t;

static const max77620_regulator_t _pmic_regulators[] = {
    {  REGULATOR_SD,  "sd0", 0x16, 12500, 600000,  625000, 1400000,      MAX77620_REG_SD0,   MAX77620_REG_SD0_CFG, 0x3F, 0x30, 4, 0x80, 0x4F, 1, 7, 1 },
    {  REGULATOR_SD,  "sd1", 0x17, 12500, 600000, 1125000, 1125000,      MAX77620_REG_SD1,   MAX77620_REG_SD1_CFG, 0x3F, 0x30, 4, 0x40, 0x50, 0, 1, 5 },
    {  REGULATOR_SD,  "sd2", 0x18, 12500, 600000, 1325000, 1350000,      MAX77620_REG_SD2,   MAX77620_REG_SD2_CFG, 0xFF, 0x30, 4, 0x20, 0x51, 1, 5, 2 },
    {  REGULATOR_SD,  "sd3", 0x19, 12500, 600000, 1800000, 1800000,      MAX77620_REG_SD3,   MAX77620_REG_SD3_CFG, 0xFF, 0x30, 4, 0x10, 0x52, 0, 3, 3 },
    { REGULATOR_LDO, "ldo0", 0x00, 25000, 800000, 1200000, 1200000, MAX77620_REG_LDO0_CFG, MAX77620_REG_LDO0_CFG2, 0x3F, 0xC0, 6, 0x00, 0x46, 3, 7, 0 },
    { REGULATOR_LDO, "ldo1", 0x00, 25000, 800000, 1050000, 1050000, MAX77620_REG_LDO1_CFG, MAX77620_REG_LDO1_CFG2, 0x3F, 0xC0, 6, 0x00, 0x47, 3, 7, 0 },
    { REGULATOR_LDO, "ldo2", 0x00, 50000, 800000, 1800000, 3300000, MAX77620_REG_LDO2_CFG, MAX77620_REG_LDO2_CFG2, 0x3F, 0xC0, 6, 0x00, 0x48, 3, 7, 0 },
    { REGULATOR_LDO, "ldo3", 0x00, 50000, 800000, 3100000, 3100000, MAX77620_REG_LDO3_CFG, MAX77620_REG_LDO3_CFG2, 0x3F, 0xC0, 6, 0x00, 0x49, 3, 7, 0 },
    { REGULATOR_LDO, "ldo4", 0x00, 12500, 800000,  850000,  850000, MAX77620_REG_LDO4_CFG, MAX77620_REG_LDO4_CFG2, 0x3F, 0xC0, 6, 0x00, 0x4A, 0, 7, 1 },
    { REGULATOR_LDO, "ldo5", 0x00, 50000, 800000, 1800000, 1800000, MAX77620_REG_LDO5_CFG, MAX77620_REG_LDO5_CFG2, 0x3F, 0xC0, 6, 0x00, 0x4B, 3, 7, 0 },
    { REGULATOR_LDO, "ldo6", 0x00, 50000, 800000, 2900000, 2900000, MAX77620_REG_LDO6_CFG, MAX77620_REG_LDO6_CFG2, 0x3F, 0xC0, 6, 0x00, 0x4C, 3, 7, 0 },
    { REGULATOR_LDO, "ldo7", 0x00, 50000, 800000, 1050000, 1050000, MAX77620_REG_LDO7_CFG, MAX77620_REG_LDO7_CFG2, 0x3F, 0xC0, 6, 0x00, 0x4D, 1, 4, 3 },
    { REGULATOR_LDO, "ldo8", 0x00, 50000, 800000, 1050000, 1050000, MAX77620_REG_LDO8_CFG, MAX77620_REG_LDO8_CFG2, 0x3F, 0xC0, 6, 0x00, 0x4E, 3, 7, 0 }
};

int max77620_regulator_get_status(uint32_t id)
{
    if (id > REGULATOR_MAX)
        return 0;

    const max77620_regulator_t *reg = &_pmic_regulators[id];
    uint8_t val = 0;

    if (reg->type == REGULATOR_SD) {
        if (i2c_query(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_STATSD, &val, 1))
            return (val & reg->status_mask) ? 0 : 1;
    }
    
    if (i2c_query(I2C_5, MAX77620_PWR_I2C_ADDR, reg->cfg_addr, &val, 1))
        return (val & 8) ? 1 : 0;
    
    return 0;
}

int max77620_regulator_config_fps(uint32_t id)
{
    if (id > REGULATOR_MAX)
        return 0;

    const max77620_regulator_t *reg = &_pmic_regulators[id];
    uint8_t val = ((reg->fps_src << 6) | (reg->pu_period << 3) | (reg->pd_period));

    if (i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, reg->fps_addr, &val, 1)) {
        return 1;
    }

    return 0;
}

int max77620_regulator_set_voltage(uint32_t id, uint32_t mv)
{
    if (id > REGULATOR_MAX)
        return 0;

    const max77620_regulator_t *reg = &_pmic_regulators[id];

    if ((mv < reg->mv_default) || (mv > reg->mv_max))
        return 0;

    uint32_t mult = (mv + reg->mv_step - 1 - reg->mv_min) / reg->mv_step;
    uint8_t val = 0;
    
    if (i2c_query(I2C_5, MAX77620_PWR_I2C_ADDR, reg->volt_addr, &val, 1))
    {
        val = ((val & ~reg->volt_mask) | (mult & reg->volt_mask));
        
        if (i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, reg->volt_addr, &val, 1))
        {
            udelay(1000);
            return 1;
        }
    }
    
    return 0;
}

int max77620_regulator_enable(uint32_t id, int enable)
{
    if (id > REGULATOR_MAX)
        return 0;

    const max77620_regulator_t *reg = &_pmic_regulators[id];

    uint32_t addr = (reg->type == REGULATOR_SD) ? reg->cfg_addr : reg->volt_addr;
    uint8_t val = 0;
    
    if (i2c_query(I2C_5, MAX77620_PWR_I2C_ADDR, addr, &val, 1))
    {
        if (enable)
            val = ((val & ~reg->enable_mask) | ((3 << reg->enable_shift) & reg->enable_mask));
        else
            val &= ~reg->enable_mask;
        
        if (i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, addr, &val, 1))
        {
            udelay(1000);
            return 1;
        }
    }

    return 0;
}

void max77620_config_default()
{
    for (uint32_t i = 1; i <= REGULATOR_MAX; i++)
    {
        uint8_t val = 0;
        if (i2c_query(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_CID4, &val, 1))
        {
            max77620_regulator_config_fps(i);
            max77620_regulator_set_voltage(i, _pmic_regulators[i].mv_default);
            
            if (_pmic_regulators[i].fps_src != MAX77620_FPS_SRC_NONE) {
                max77620_regulator_enable(i, 1);
            }
        }       
    }
    
    uint8_t val = 4;
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_SD_CFG2, &val, 1);
}

void max77620_low_battery_monitor_config()
{
    uint8_t val = (MAX77620_CNFGGLBL1_LBDAC_EN | MAX77620_CNFGGLBL1_LBHYST_N | MAX77620_CNFGGLBL1_LBDAC_N);
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_CNFGGLBL1, &val, 1);
}
