#include "utils.h"
#include <stdint.h>
#include "display/video_fb.h"
#include "sd_utils.h"
#include "stage2.h"
#include "chainloader.h"
#include "lib/printk.h"
#include "lib/vsprintf.h"
#include "lib/ini.h"
#include "lib/fatfs/ff.h"

char g_stage2_path[0x100] = {0};

const char *stage2_get_program_path(void) {
    return g_stage2_path;
}

static int stage2_ini_handler(void *user, const char *section, const char *name, const char *value) {
    stage2_config_t *config = (stage2_config_t *)user;
    uintptr_t x = 0;
    if (strcmp(section, "stage1") == 0) {
        if (strcmp(name, STAGE2_NAME_KEY) == 0) {
            strncpy(config->path, value, sizeof(config->path));
        } else if (strcmp(name, STAGE2_ADDRESS_KEY) == 0) {
            /* Read in load address as a hex string. */
            sscanf(value, "%x", &x);
            config->load_address = x;
            if (config->entrypoint == 0) {
                config->entrypoint = config->load_address;
            }
        } else if (strcmp(name, STAGE2_ENTRYPOINT_KEY) == 0) {
            /* Read in entrypoint as a hex string. */
            sscanf(value, "%x", &x);
            config->entrypoint = x;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

void load_stage2(const char *bct0) {
    stage2_config_t config = {0};
    FILINFO info;
    size_t size;
    uintptr_t tmp_addr;

    if (ini_parse_string(bct0, stage2_ini_handler, &config) < 0) {
        printk("Error: Failed to parse BCT.ini!\n");
        generic_panic();
    }

    if (config.load_address == 0 || config.path[0] == '\x00') {
        printk("Error: Failed to determine where to load stage2!\n");
        generic_panic();
    }

    if (strlen(config.path) + 1 + sizeof(stage2_args_t) > CHAINLOADER_ARG_DATA_MAX_SIZE) {
        printk("Error: Stage2's path name is too big!\n");
    }

    if (!check_32bit_address_loadable(config.entrypoint)) {
        printk("Error: Stage2's entrypoint is invalid!\n");
        generic_panic();
    }

    if (!check_32bit_address_loadable(config.load_address)) {
        printk("Error: Stage2's load address is invalid!\n");
        generic_panic();
    }

    printk("[DEBUG] Stage 2 Config:\n");
    printk("    File Path:    %s\n", config.path);
    printk("    Load Address: 0x%08x\n", config.load_address);
    printk("    Entrypoint:   0x%p\n", config.entrypoint);

    if (f_stat(config.path, &info) != FR_OK) {
        printk("Error: Failed to stat stage2 (%s)!\n", config.path);
        generic_panic();
    }

    size = (size_t)info.fsize;

    /* the LFB is located at 0xC0000000 atm */
    if (size > 0xC0000000u - 0x80000000u) {
        printk("Error: Stage2 is way too big!\n");
        generic_panic();
    }

    if (!check_32bit_address_range_loadable(config.load_address, size)) {
        printk("Error: Stage2 has an invalid load address & size combination (0x%08x 0x%08x)!\n", config.load_address, size);
        generic_panic();
    }

    if (config.entrypoint < config.load_address || config.entrypoint >= config.load_address + size) {
        printk("Error: Stage2's entrypoint is outside Stage2!\n");
        generic_panic();
    }

    if (check_32bit_address_range_in_program(config.load_address, size)) {
        tmp_addr = 0x80000000u;
    } else {
        tmp_addr = config.load_address;
    }

    if (read_sd_file((void *)tmp_addr, size, config.path) != size) {
        printk("Error: Failed to read stage2 (%s)!\n", config.path);
        generic_panic();
    }

    g_chainloader_num_entries             = 1;
    g_chainloader_entries[0].load_address = config.load_address;
    g_chainloader_entries[0].src_address  = tmp_addr;
    g_chainloader_entries[0].size         = size;
    g_chainloader_entries[0].num          = 0;
    g_chainloader_entrypoint              = config.entrypoint;

    strncpy(g_stage2_path, config.path, sizeof(g_stage2_path));
}
