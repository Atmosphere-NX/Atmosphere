#include <stdint.h>
#include <stdbool.h>

#include "utils.h"
#include "exocfg.h"
#include "mmu.h"
#include "memory_map.h"

#define MAILBOX_BASE  (MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_NXBOOTLOADER_MAILBOX))

/* TODO: Should this be at a non-static location? */
#define MAILBOX_EXOSPHERE_CONFIG (*((volatile exosphere_config_t *)(MAILBOX_BASE + 0xE40ULL)))

static exosphere_config_t g_exosphere_cfg = {MAGIC_ATMOSPHERE, EXOSPHERE_TARGET_FIRMWARE_DEFAULT_FOR_DEBUG};
static bool g_has_loaded_config = false;

/* Read config out of IRAM, return target firmware version. */
unsigned int exosphere_load_config(void) {
    if (g_has_loaded_config) {
        generic_panic();
    }
    g_has_loaded_config = true;
    
    if (MAILBOX_EXOSPHERE_CONFIG.magic == MAGIC_ATMOSPHERE) {
        g_exosphere_cfg = MAILBOX_EXOSPHERE_CONFIG;
    }
    
    return g_exosphere_cfg.target_firmware;
}

unsigned int exosphere_get_target_firmware(void) {
    if (!g_has_loaded_config) {
        generic_panic();
    }
    
    return g_exosphere_cfg.target_firmware;
}