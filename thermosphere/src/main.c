#include "utils.h"
#include "core_ctx.h"
#include "debug_log.h"
#include "platform/uart.h"
#include "traps.h"

int main(void)
{
    enableTraps();

    if (currentCoreCtx->coreId == 0) {
        uartInit(115200);
        DEBUG("Hello from Thermosphere!\n");
        __builtin_trap();
    }
    else {
        DEBUG("Core %u booted\n", currentCoreCtx->coreId);
    }

    return 0;
}
