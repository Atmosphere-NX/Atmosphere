#ifndef EXOSPHERE_SMC_API_H
#define EXOSPHERE_SMC_API_H

#include <stdint.h>

#define SMC_HANDLER_USER 0
#define SMC_HANDLER_PRIV 1

typedef struct {
    uint64_t X[8];
} smc_args_t;

void set_priv_smc_in_progress(void);
void clear_priv_smc_in_progress(void);

uintptr_t get_smc_core012_stack_address(void);
uintptr_t get_exception_entry_core3_stack_address(void);

void call_smc_handler(unsigned int handler_id, smc_args_t *args);

#endif
