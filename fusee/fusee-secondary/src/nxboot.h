#ifndef FUSEE_NX_BOOT_H
#define FUSEE_NX_BOOT_H

#include "utils.h"

#define MAILBOX_NX_BOOTLOADER_BASE ((void *)(0x40002000))

#define MAILBOX_NX_BOOTLOADER_BOOT_REASON (MAILBOX_NX_BOOTLOADER_BASE + 0xE10)
#define MAILBOX_NX_BOOTLOADER_SETUP_STATE MAKE_REG32(MAILBOX_NX_BOOTLOADER_BASE + 0xEF8)
#define MAILBOX_NX_BOOTLOADER_IS_SECMON_AWAKE MAKE_REG32(MAILBOX_NX_BOOTLOADER_BASE + 0xEFC)

#define NX_BOOTLOADER_STATE_INIT 0
#define NX_BOOTLOADER_STATE_MOVED_BOOTCONFIG 1
#define NX_BOOTLOADER_STATE_LOADED_PACKAGE2 2
#define NX_BOOTLOADER_STATE_FINISHED 3
#define NX_BOOTLOADER_STATE_DRAM_INITIALIZED_4X 2
#define NX_BOOTLOADER_STATE_LOADED_PACKAGE2_4X 3
#define NX_BOOTLOADER_STATE_FINISHED_4X 4

typedef struct {
    uint32_t bootloader_version;
    uint32_t bootloader_start_block;
    uint32_t bootloader_start_page;
    uint32_t bootloader_attribute;
    uint32_t boot_reason_value;
    uint32_t boot_reason_state;
} boot_reason_t;

void nxboot_main(void);

#endif
