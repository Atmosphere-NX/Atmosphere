#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "nxboot.h"
#include "key_derivation.h"
#include "package1.h"
#include "package2.h"
#include "loader.h"
#include "splash_screen.h"
#include "exocfg.h"
#include "display/video_fb.h"
#include "lib/ini.h"
#include "hwinit/cluster.h"

static int exosphere_ini_handler(void *user, const char *section, const char *name, const char *value) {
    exosphere_config_t *exo_cfg = (exosphere_config_t *)user;
    if (strcmp(section, "exosphere") == 0) {
        if (strcmp(name, EXOSPHERE_TARGETFW_KEY) == 0) {
            sscanf(value, "%d", &exo_cfg->target_firmware);
        } else {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

void nxboot_configure_exosphere(void) {
    exosphere_config_t exo_cfg = {0};

    exo_cfg.magic = MAGIC_EXOSPHERE_BOOTCONFIG;
    exo_cfg.target_firmware = EXOSPHERE_TARGET_FIRMWARE_MAX;

    if (ini_parse_string(get_loader_ctx()->bct0, exosphere_ini_handler, &exo_cfg) < 0) {
        printf("Error: Failed to parse BCT.ini!\n");
        generic_panic();
    }

    if (exo_cfg.target_firmware < EXOSPHERE_TARGET_FIRMWARE_MIN || exo_cfg.target_firmware > EXOSPHERE_TARGET_FIRMWARE_MAX) {
        printf("Error: Invalid Exosphere target firmware!\n");
        generic_panic();
    }

    *(MAILBOX_EXOSPHERE_CONFIGURATION) = exo_cfg;
}

/* This is the main function responsible for booting Horizon. */
void nxboot_main(void) {
    loader_ctx_t *loader_ctx = get_loader_ctx();

    /* TODO: Validate that we're capable of booting. */

    /* TODO: Initialize Boot Reason. */

    /* TODO: How should we deal with bootconfig? */

    /* Setup boot configuration for Exosphere. */
    nxboot_configure_exosphere();

    /* Derive keydata. */
    //derive_nx_keydata(MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware);

    if (loader_ctx->package2_loadfile.load_address == 0) {

        /* TODO: read package2 somewhere. */
    }

    /* Patch package2, adding thermosphere + custom KIPs. */
    package2_rebuild_and_copy((void *)loader_ctx->package2_loadfile.load_address);

    /* Boot up Exosphere. */
    MAILBOX_NX_BOOTLOADER_IS_SECMON_AWAKE = 0;
    if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware <= EXOSPHERE_TARGET_FIRMWARE_400) {
        MAILBOX_NX_BOOTLOADER_SETUP_STATE = NX_BOOTLOADER_STATE_LOADED_PACKAGE2;
    } else {
        MAILBOX_NX_BOOTLOADER_SETUP_STATE = NX_BOOTLOADER_STATE_LOADED_PACKAGE2_4X;
    }
    cluster_enable_cpu0(loader_ctx->exosphere_loadfile.load_address, 1);
    while (MAILBOX_NX_BOOTLOADER_IS_SECMON_AWAKE == 0) {
        /* Wait for Exosphere to wake up. */
    }
    if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware <= EXOSPHERE_TARGET_FIRMWARE_400) {
        MAILBOX_NX_BOOTLOADER_SETUP_STATE = NX_BOOTLOADER_STATE_FINISHED;
    } else {
        MAILBOX_NX_BOOTLOADER_SETUP_STATE = NX_BOOTLOADER_STATE_FINISHED_4X;
    }

    /* Display splash screen. */
    display_splash_screen_bmp(loader_ctx->custom_splash_path);

    /* TODO: Halt ourselves. */
}
