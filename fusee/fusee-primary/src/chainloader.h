#ifndef FUSEE_CHAINLOADER_H
#define FUSEE_CHAINLOADER_H

#include <stddef.h>
#include <stdint.h>

#define CHAINLOADER_ARG_DATA_MAX_SIZE 0x6200
#define CHAINLOADER_MAX_ENTRIES       128

typedef struct chainloader_entry_t {
    uintptr_t load_address;
    uintptr_t src_address;
    size_t size;
    size_t num;
} chainloader_entry_t;

extern chainloader_entry_t g_chainloader_entries[CHAINLOADER_MAX_ENTRIES]; /* keep them sorted */
extern size_t g_chainloader_num_entries;
extern uintptr_t g_chainloader_entrypoint;

extern char g_chainloader_arg_data[CHAINLOADER_ARG_DATA_MAX_SIZE];

void relocate_and_chainload(int argc);

#endif
