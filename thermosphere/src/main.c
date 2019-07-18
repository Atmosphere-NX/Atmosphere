#include "utils.h"
#include "uart.h"
#include "car.h"
#include "log.h"

int main(void)
{
    // Init uart (hardcoded atm)
    uart_select(UART_C);
    clkrst_reboot(CARDEVICE_UARTC);
    uart_init(UART_C, 115200, true);

    serialLog("Hello from Thermosphere!\n");
    return 0;
}
