#include "utils.h"
#include "uart.h"
#include "log.h"

int main(void)
{
    uart_config(UART_C);
    uart_reset(UART_C);
    uart_init(UART_C, 115200, true);

    //uart_send(UART_C, "0123\n", 3);
    serialLog("Hello from Thermosphere!\r\n");
    return 0;
}
