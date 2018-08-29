#include <stdint.h>

#include "btn.h"
#include "i2c.h"
#include "gpio.h"
#include "timers.h"

uint32_t btn_read()
{
    uint32_t res = 0;
    
    if (!gpio_read(GPIO_BUTTON_VOL_DOWN))
        res |= BTN_VOL_DOWN;
    
    if (!gpio_read(GPIO_BUTTON_VOL_UP))
        res |= BTN_VOL_UP;
    
    uint32_t val = 0;
    if (i2c_query(I2C_5, MAX77620_PWR_I2C_ADDR, 0x15, &val, 1))
    {
        if (val & 0x4)
            res |= BTN_POWER;
    }
    
    return res;
}

uint32_t btn_wait()
{
    uint32_t res = 0, btn = btn_read();
    int pwr = 0;

    if (btn & BTN_POWER)
    {
        pwr = 1;
        btn &= ~BTN_POWER;
    }

    do
    {
        res = btn_read();

        if (!(res & BTN_POWER) && pwr)
            pwr = 0;
        else if (pwr)
            res &= ~BTN_POWER;
    } while (btn == res);

    return res;
}

uint32_t btn_wait_timeout(uint32_t time_ms, uint32_t mask)
{
    uint32_t timeout = get_time_ms() + time_ms;
    uint32_t res = btn_read() & mask;

    do
    {
        if (!(res & mask))
            res = btn_read() & mask;
    } while (get_time_ms() < timeout);

    return res;
}