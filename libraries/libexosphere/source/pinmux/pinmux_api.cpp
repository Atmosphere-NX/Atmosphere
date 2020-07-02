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

namespace ams::pinmux {

    namespace {

        constinit uintptr_t g_pinmux_address = secmon::MemoryRegionPhysicalDeviceApbMisc.GetAddress();
        constinit uintptr_t g_gpio_address   = secmon::MemoryRegionPhysicalDeviceGpio.GetAddress();

    }

    void SetRegisterAddress(uintptr_t pinmux_address, uintptr_t gpio_address) {
        g_pinmux_address = pinmux_address;
        g_gpio_address   = gpio_address;
    }

    void SetupUartA() {
        /* Get the registers. */
        const uintptr_t PINMUX = g_pinmux_address;

        /* Configure Uart-A. */
        reg::Write(PINMUX + PINMUX_AUX_UART1_TX,  PINMUX_REG_BITS_ENUM(AUX_UART1_PM,       UARTA),
                                                  PINMUX_REG_BITS_ENUM(AUX_PUPD,            NONE),
                                                  PINMUX_REG_BITS_ENUM(AUX_TRISTATE, PASSTHROUGH),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,      DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));

        reg::Write(PINMUX + PINMUX_AUX_UART1_RX,  PINMUX_REG_BITS_ENUM(AUX_UART1_PM,       UARTA),
                                                  PINMUX_REG_BITS_ENUM(AUX_PUPD,         PULL_UP),
                                                  PINMUX_REG_BITS_ENUM(AUX_TRISTATE, PASSTHROUGH),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,       ENABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));

        reg::Write(PINMUX + PINMUX_AUX_UART1_RTS, PINMUX_REG_BITS_ENUM(AUX_UART1_PM,       UARTA),
                                                  PINMUX_REG_BITS_ENUM(AUX_PUPD,            NONE),
                                                  PINMUX_REG_BITS_ENUM(AUX_TRISTATE, PASSTHROUGH),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,      DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));

        reg::Write(PINMUX + PINMUX_AUX_UART1_CTS, PINMUX_REG_BITS_ENUM(AUX_UART1_PM,       UARTA),
                                                  PINMUX_REG_BITS_ENUM(AUX_PUPD,       PULL_DOWN),
                                                  PINMUX_REG_BITS_ENUM(AUX_TRISTATE, PASSTHROUGH),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,       ENABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));
    }

    void SetupUartB() {
        /* Get the registers. */
        const uintptr_t PINMUX = g_pinmux_address;

        /* Configure Uart-B. */
        reg::Write(PINMUX + PINMUX_AUX_UART2_TX,  PINMUX_REG_BITS_ENUM(AUX_UART2_PM,       UARTB),
                                                  PINMUX_REG_BITS_ENUM(AUX_PUPD,            NONE),
                                                  PINMUX_REG_BITS_ENUM(AUX_TRISTATE, PASSTHROUGH),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,      DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));

        reg::Write(PINMUX + PINMUX_AUX_UART2_RX,  PINMUX_REG_BITS_ENUM(AUX_UART2_PM,       UARTB),
                                                  PINMUX_REG_BITS_ENUM(AUX_PUPD,            NONE),
                                                  PINMUX_REG_BITS_ENUM(AUX_TRISTATE, PASSTHROUGH),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,       ENABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));

        reg::Write(PINMUX + PINMUX_AUX_UART2_RTS, PINMUX_REG_BITS_ENUM(AUX_UART2_PM,       UARTB),
                                                  PINMUX_REG_BITS_ENUM(AUX_PUPD,            NONE),
                                                  PINMUX_REG_BITS_ENUM(AUX_TRISTATE, PASSTHROUGH),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,      DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));

        reg::Write(PINMUX + PINMUX_AUX_UART2_CTS, PINMUX_REG_BITS_ENUM(AUX_UART2_PM,       UARTB),
                                                  PINMUX_REG_BITS_ENUM(AUX_PUPD,            NONE),
                                                  PINMUX_REG_BITS_ENUM(AUX_TRISTATE, PASSTHROUGH),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,       ENABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));

        /* Configure GPIO for Uart-B. */
        reg::ReadWrite(g_gpio_address + 0x108, REG_BITS_VALUE(0, 4, 0));
    }

    void SetupUartC() {
        /* Get the registers. */
        const uintptr_t PINMUX = g_pinmux_address;

        /* Configure Uart-C. */
        reg::Write(PINMUX + PINMUX_AUX_UART3_TX,  PINMUX_REG_BITS_ENUM(AUX_UART3_PM,       UARTC),
                                                  PINMUX_REG_BITS_ENUM(AUX_PUPD,            NONE),
                                                  PINMUX_REG_BITS_ENUM(AUX_TRISTATE, PASSTHROUGH),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,      DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));

        reg::Write(PINMUX + PINMUX_AUX_UART3_RX,  PINMUX_REG_BITS_ENUM(AUX_UART3_PM,       UARTC),
                                                  PINMUX_REG_BITS_ENUM(AUX_PUPD,            NONE),
                                                  PINMUX_REG_BITS_ENUM(AUX_TRISTATE,    TRISTATE),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,       ENABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));

        reg::Write(PINMUX + PINMUX_AUX_UART3_RTS, PINMUX_REG_BITS_ENUM(AUX_UART3_PM,       UARTC),
                                                  PINMUX_REG_BITS_ENUM(AUX_PUPD,       PULL_DOWN),
                                                  PINMUX_REG_BITS_ENUM(AUX_TRISTATE, PASSTHROUGH),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,      DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));

        reg::Write(PINMUX + PINMUX_AUX_UART3_CTS, PINMUX_REG_BITS_ENUM(AUX_UART3_PM,       UARTC),
                                                  PINMUX_REG_BITS_ENUM(AUX_PUPD,            NONE),
                                                  PINMUX_REG_BITS_ENUM(AUX_TRISTATE,    TRISTATE),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_INPUT,       ENABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                  PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));

        /* Configure GPIO for Uart-C. */
        reg::ReadWrite(g_gpio_address + 0x118, REG_BITS_VALUE(0, 1, 1));
        reg::Read(g_gpio_address + 0x118);
        reg::ReadWrite(g_gpio_address + 0x00C, REG_BITS_VALUE(1, 1, 0));
        reg::Read(g_gpio_address + 0x00C);
    }

    void SetupI2c1() {
        /* Get the registers. */
        const uintptr_t PINMUX = g_pinmux_address;

        /* Configure I2c1 */
        reg::Write(PINMUX + PINMUX_AUX_GEN1_I2C_SCL, PINMUX_REG_BITS_ENUM(AUX_GEN1_I2C_PM,     I2C1),
                                                     PINMUX_REG_BITS_ENUM(AUX_PUPD,            NONE),
                                                     PINMUX_REG_BITS_ENUM(AUX_TRISTATE, PASSTHROUGH),
                                                     PINMUX_REG_BITS_ENUM(AUX_E_INPUT,       ENABLE),
                                                     PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                     PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));

        reg::Write(PINMUX + PINMUX_AUX_GEN1_I2C_SDA, PINMUX_REG_BITS_ENUM(AUX_GEN1_I2C_PM,     I2C1),
                                                     PINMUX_REG_BITS_ENUM(AUX_PUPD,            NONE),
                                                     PINMUX_REG_BITS_ENUM(AUX_TRISTATE, PASSTHROUGH),
                                                     PINMUX_REG_BITS_ENUM(AUX_E_INPUT,       ENABLE),
                                                     PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                     PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));
    }

    void SetupI2c5() {
        /* Get the registers. */
        const uintptr_t PINMUX = g_pinmux_address;

        /* Configure I2c5 */
        reg::Write(PINMUX + PINMUX_AUX_PWR_I2C_SCL,  PINMUX_REG_BITS_ENUM(AUX_PWR_I2C_PM,    I2CPMU),
                                                     PINMUX_REG_BITS_ENUM(AUX_PUPD,            NONE),
                                                     PINMUX_REG_BITS_ENUM(AUX_TRISTATE, PASSTHROUGH),
                                                     PINMUX_REG_BITS_ENUM(AUX_E_INPUT,       ENABLE),
                                                     PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                     PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));

        reg::Write(PINMUX + PINMUX_AUX_PWR_I2C_SDA,  PINMUX_REG_BITS_ENUM(AUX_PWR_I2C_PM,    I2CPMU),
                                                     PINMUX_REG_BITS_ENUM(AUX_PUPD,            NONE),
                                                     PINMUX_REG_BITS_ENUM(AUX_TRISTATE, PASSTHROUGH),
                                                     PINMUX_REG_BITS_ENUM(AUX_E_INPUT,       ENABLE),
                                                     PINMUX_REG_BITS_ENUM(AUX_LOCK,         DISABLE),
                                                     PINMUX_REG_BITS_ENUM(AUX_E_OD,         DISABLE));
    }

}