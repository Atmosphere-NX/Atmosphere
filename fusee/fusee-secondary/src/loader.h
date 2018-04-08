#ifndef FUSEE_LOADER_H
#define FUSEE_LOADER_H

typedef void (*entrypoint_t)(int argc, void **argv); 

typedef struct {
    char path[0x300];
    const char *key;
    uintptr_t load_address;
} load_file_t;

typedef struct {
    entrypoint_t entrypoint;
} loader_ctx_t;

#define LOADER_ENTRYPOINT_KEY "entrypoint"
#define LOADER_LOADLIST_KEY "loadlist"

/* TODO: Should we allow files bigger than 16 MB? */
#define LOADER_FILESIZE_MAX 0x01000000

entrypoint_t load_payload(const char *bct0);

#endif