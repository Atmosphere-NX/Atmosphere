#ifndef FUSEE_SDRAM_H_
#define FUSEE_SDRAM_H_

void sdram_init();
const void *sdram_get_params();
void sdram_lp0_save_params(const void *params);

#endif
