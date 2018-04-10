#ifndef FUSEE_LOADER_H
#define FUSEE_LOADER_H

#include "utils.h"

typedef void (*entrypoint_t)(int argc, void **argv); 

typedef struct {
    char path[0x300];
    const char *key;
    uintptr_t load_address;
} load_file_t;

typedef struct {
    const char *bct0;
    entrypoint_t chainload_entrypoint;
    load_file_t package2_loadfile;
    load_file_t exosphere_loadfile;
    load_file_t tsecfw_loadfile;
    load_file_t warmboot_loadfile;
    char custom_splash_path[0x300];
} loader_ctx_t;

#define LOADER_ENTRYPOINT_KEY "entrypoint"
#define LOADER_LOADLIST_KEY "loadlist"
#define LOADER_CUSTOMSPLASH_KEY "custom_splash"

#define LOADER_PACKAGE2_KEY "package2"
#define LOADER_EXOSPHERE_KEY "exosphere"
#define LOADER_TSECFW_KEY "tsecfw"
#define LOADER_WARMBOOT_KEY "warmboot"

/* TODO: Should we allow files bigger than 16 MB? */
#define LOADER_FILESIZE_MAX 0x01000000

loader_ctx_t *get_loader_ctx(void);

void load_payload(const char *bct0);

#endif