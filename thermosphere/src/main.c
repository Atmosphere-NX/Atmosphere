#include "utils.h"
#include "log.h"
#include "platform/uart.h"

int main(void)
{
    uartInit(115200);

    //uart_send(UART_C, "0123\n", 3);
    serialLog("Hello from Thermosphere!\r\n");
    __builtin_trap();
    return 0;
}
