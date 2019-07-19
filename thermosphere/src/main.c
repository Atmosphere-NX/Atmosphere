#include "utils.h"
#include "uart.h"
#include "car.h"
#include "log.h"
#include "gpio.h"

int main(void)
{
    // Init uart (hardcoded atm)
    gpio_configure_mode(TEGRA_GPIO(G, 0), GPIO_MODE_GPIO);      // Leave UART-B as GPIO
    gpio_configure_mode(TEGRA_GPIO(D, 1), GPIO_MODE_SFIO);      // Change UART-C to SPIO
    uart_select(UART_C);                                        // Configure pinmux for UART-C
    clkrst_reboot(CARDEVICE_UARTC);                             // Enable UART-C clock
    uart_init(UART_C, 115200, true);

    //uart_send(UART_C, "0123\n", 3);
    serialLog("Hello from Thermosphere!\n");
    return 0;
}
