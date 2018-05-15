#ifndef FUSEE_STRATOSPHERE_H
#define FUSEE_STRATOSPHERE_H

#include "utils.h"
#include "kip.h"

#define STRATOSPHERE_INI1_EMBEDDED 0x0
#define STRATOSPHERE_INI1_PACKAGE2 0x1
#define STRATOSPHERE_INI1_MAX      0x2

ini1_header_t *stratosphere_get_ini1(uint32_t target_firmware);
void stratosphere_free_ini1(void);

ini1_header_t *stratosphere_merge_inis(ini1_header_t **inis, unsigned int num_inis);

#endif
