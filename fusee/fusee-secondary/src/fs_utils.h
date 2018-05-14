#ifndef FUSEE_FS_UTILS_H
#define FUSEE_FS_UTILS_H

#include "utils.h"
#include "sdmmc.h"

size_t get_file_size(const char *filename);
size_t read_from_file(void *dst, size_t dst_size, const char *filename);
size_t dump_to_file(const void *src, size_t src_size, const char *filename);

#endif
