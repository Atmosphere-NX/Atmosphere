#include "utils.h"
#include "uart.h"
#include "log.h"

int main(void)
{
    uart_init(UART_C, 115200, true);

    //uart_send(UART_C, "0123\n", 3);
    serialLog("Hello from Thermosphere!\n");
    return 0;
}
