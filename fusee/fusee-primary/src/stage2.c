#include "utils.h"
#include <stdint.h>
#include "sd_utils.h"
#include "stage2.h"
#include "lib/printk.h"
#include "lib/vsprintf.h"
#include "lib/ini.h"

static int stage2_ini_handler(void *user, const char *section, const char *name, const char *value) {
    stage2_config_t *config = (stage2_config_t *)user;
    uintptr_t x = 0;
    if (strcmp(section, "stage1") == 0) {
        if (strcmp(name, STAGE2_NAME_KEY) == 0) {
            strncpy(config->filename, value, sizeof(config->filename));
        } else if (strcmp(name, STAGE2_ADDRESS_KEY) == 0) {
            /* Read in load address as a hex string. */
            sscanf(value, "%x", &x);
            config->load_address = x;
            if (config->entrypoint == NULL) {
                config->entrypoint = (stage2_entrypoint_t)config->load_address;
            }
        } else if (strcmp(name, STAGE2_ENTRYPOINT_KEY) == 0) {
            /* Read in entrypoint as a hex string. */
            sscanf(value, "%x", &x);
            config->entrypoint = (stage2_entrypoint_t)x;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

stage2_entrypoint_t load_stage2(const char *bct0) {
    stage2_config_t config = {0};
    
    if (ini_parse_string(bct0, stage2_ini_handler, &config) < 0) {
        printk("Error: Failed to parse BCT.ini!\n");
        generic_panic();
    }
    
    if (config.load_address == 0 || config.filename[0] == '\x00') {
        printk("Error: Failed to determine where to load stage2!\n");
        generic_panic();
    }
    
    printk("[DEBUG] Stage 2 Config:\n");
    printk("    Filename:     %s\n", config.filename);
    printk("    Load Address: 0x%08x\n", config.load_address);
    printk("    Entrypoint:   0x%p\n", config.entrypoint);
    
    if (!read_sd_file((void *)config.load_address, 0x100000, config.filename)) {
        printk("Error: Failed to read stage2 (%s)!\n", config.filename);
        generic_panic();
    }
    
    return config.entrypoint;
}
