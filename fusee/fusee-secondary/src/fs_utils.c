#include <stdio.h>
#include "fs_utils.h"
#include "hwinit.h"
#include "sdmmc.h"

size_t read_from_file(void *dst, size_t dst_size, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        return 0;
    } else {
        size_t sz = fread(dst, 1, dst_size, file);
        fclose(file);
        return sz;
    }
}
