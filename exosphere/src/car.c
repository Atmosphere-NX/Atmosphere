#include <stdint.h>

#include "utils.h"
#include "car.h"
#include "timers.h"

static inline uint32_t get_special_clk_reg(CarDevice dev) {
    switch (dev) {
        case CARDEVICE_UARTA: return 0x178;
        case CARDEVICE_UARTB: return 0x17C;
        case CARDEVICE_I2C1: return 0x124;
        case CARDEVICE_I2C5: return 0x128;
        case CARDEVICE_BPMP: return 0;
        default: generic_panic();
    }
} 

static inline uint32_t get_special_clk_val(CarDevice dev) {
    switch (dev) {
        case CARDEVICE_UARTA: return 0;
        case CARDEVICE_UARTB: return 0;
        case CARDEVICE_I2C1: return (6 << 29);
        case CARDEVICE_I2C5: return (6 << 29);
        case CARDEVICE_BPMP: return 0;
        default: generic_panic();
    }
}

static uint32_t g_clk_reg_offsets[NUM_CAR_BANKS] = {0x320, 0x328, 0x330, 0x440, 0x448, 0x284, 0x29C};
static uint32_t g_rst_reg_offsets[NUM_CAR_BANKS] = {0x300, 0x308, 0x310, 0x430, 0x438, 0x290, 0x2A8}; 

void clk_enable(CarDevice dev) {
    uint32_t special_reg;
    if ((special_reg = get_special_clk_reg(dev))) {
        MAKE_CAR_REG(special_reg) = get_special_clk_val(dev);
    }
    MAKE_CAR_REG(g_clk_reg_offsets[dev >> 5]) |= BIT(dev & 0x1F);
}

void clk_disable(CarDevice dev) {
    MAKE_CAR_REG(g_clk_reg_offsets[dev >> 5] + 0x004) |= BIT(dev & 0x1F);
}

void rst_enable(CarDevice dev) {
    MAKE_CAR_REG(g_rst_reg_offsets[dev >> 5]) |= BIT(dev & 0x1F);
}

void rst_disable(CarDevice dev) {
    MAKE_CAR_REG(g_rst_reg_offsets[dev >> 5] + 0x004) |= BIT(dev & 0x1F);
}


void clkrst_enable(CarDevice dev) {
    clk_enable(dev);
    rst_disable(dev);
}

void clkrst_disable(CarDevice dev) {
    rst_enable(dev);
    clk_disable(dev);
}

void clkrst_reboot(CarDevice dev) {
    clkrst_disable(dev);
    wait(100);
    clkrst_enable(dev);
}
