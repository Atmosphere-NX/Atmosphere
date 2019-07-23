#include "utils.h"
#include "log.h"
#include "platform/uart.h"

int main(void)
{
    uartInit(115200);

    serialLog("fifo flush fifo flush\n");
    serialLog("Hello from Thermosphere!\n");
    __builtin_trap();
    return 0;
}
