#include "utils.h"
#include "core_ctx.h"
#include "log.h"
#include "platform/uart.h"

int main(void)
{
    if (currentCoreCtx->coreId == 0) {
        uartInit(115200);
        serialLog("Hello from Thermosphere!\n");
        __builtin_trap();
    }

    else {
        serialLog("Core %u booted\n", currentCoreCtx->coreId);
    }

    return 0;
}
