#include "chainloader.h"

uint8_t __attribute__((used)) g_payload_arg_data[PAYLOAD_ARG_DATA_MAX_SIZE] = {0};

#pragma GCC optimize (3)
void relocate_and_chainload_main(uintptr_t load_address, uintptr_t src_address, size_t size, int argc) {
    for(size_t i = 0; i < size; i++) {
        *(uint8_t *)(load_address + i) = *(uint8_t *)(src_address + i);
    }

    ((void (*)(int, void *))load_address)(argc, g_payload_arg_data);
}
