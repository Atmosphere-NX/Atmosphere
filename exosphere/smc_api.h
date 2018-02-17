#ifndef EXOSPHERE_SMC_API_H
#define EXOSPHERE_SMC_API_H

#include <stdint.h>

#define SMC_HANDLER_USER 0
#define SMC_HANDLER_PRIV 1

typedef struct {
    uint64_t X[8];
} smc_args_t;

void call_smc_handler(unsigned int handler_id, smc_args_t *args);

#endif