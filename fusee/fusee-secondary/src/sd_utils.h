#ifndef FUSEE_SD_UTILS_H
#define FUSEE_SD_UTILS_H

#include "utils.h"
#include "sdmmc.h"

void save_sd_state(void **mmc);
void resume_sd_state(void *mmc);

size_t read_sd_file(void *dst, size_t dst_size, const char *filename);

#endif
