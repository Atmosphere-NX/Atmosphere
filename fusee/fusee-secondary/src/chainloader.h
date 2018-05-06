#ifndef FUSEE_CHAINLOADER_H
#define FUSEE_CHAINLOADER_H

#include <stddef.h>
#include <stdint.h>

#define PAYLOAD_ARG_DATA_MAX_SIZE 0x1000

extern uint8_t g_payload_arg_data[PAYLOAD_ARG_DATA_MAX_SIZE];

void relocate_and_chainload(uintptr_t load_address, uintptr_t src_address, size_t size, int argc);

#endif
