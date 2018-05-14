#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <malloc.h>
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
static nx_keyblob_t __attribute__((aligned(16))) g_keyblobs[32];
void nxboot_main(void) {
    loader_ctx_t *loader_ctx = get_loader_ctx();
    package2_header_t *package2_src = (package2_header_t *)loader_ctx->package2_loadfile.load_address;
    package2_header_t *package2 = NULL;
    void *tsec_fw = (void *)loader_ctx->tsecfw_loadfile.load_address;
    size_t tsec_fw_size = loader_ctx->tsecfw_loadfile.load_size;
    void *warmboot_fw = (void *)loader_ctx->warmboot_loadfile.load_address;
    size_t warmboot_fw_size = loader_ctx->warmboot_loadfile.load_size;
    void *package1loader = NULL;
    size_t package1loader_size = 0;
    package1_header_t *package1 = NULL;
    size_t package1_size = 0;
    uint32_t revision = EXOSPHERE_TARGET_FIRMWARE_MAX;
    FILE *boot0 = fopen("boot0:/", "rb");

    if (boot0 == NULL || package1_read_and_parse_boot0(&package1loader, &package1loader_size, g_keyblobs, &revision, boot0) == -1) {
        printf("Error: Couldn't parse boot0: %s!\n", strerror(errno));
        generic_panic();
    }
    fclose(boot0);

    if (tsec_fw == NULL) {
        tsec_fw_size = package1_get_tsec_fw(&tsec_fw, package1loader, package1loader_size);
        if (tsec_fw_size == 0) {
            printf("Error: Failed to read the TSEC firmware from Package1loader!\n");
            generic_panic();
        }
    }

    /* TODO: Validate that we're capable of booting. */

    /* TODO: Initialize Boot Reason. */

    /* Setup boot configuration for Exosphere. */
    nxboot_configure_exosphere();

    /* Derive keydata. */
    if (derive_nx_keydata(MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware, g_keyblobs, revision, tsec_fw, tsec_fw_size) != 0) {
        printf("Error: Key derivation failed!\n");
        generic_panic();
    }

    if (warmboot_fw == NULL) {
        uint8_t ctr[16];
        package1_size = package1_get_encrypted_package1(&package1, ctr, package1loader, package1loader_size);
        if(package1_decrypt(package1, package1_size, ctr)) {
            warmboot_fw = package1_get_warmboot_fw(package1);
            warmboot_fw_size = package1->warmboot_size;
        } else {
            warmboot_fw = NULL;
            warmboot_fw_size = 0;
        }

        if (warmboot_fw_size == 0) {
            printf("Error: Failed to read the warmboot firmware from Package1!\n");
            generic_panic();
        }
    }

    free(package1loader);

    package2 = memalign(16, PACKAGE2_SIZE_MAX);
    if (package2 == NULL) {
        printf("Error: nxboot: out of memory!\n");
        generic_panic();
    }

    if (package2_src == NULL) {
        /* TODO: How should we deal with bootconfig? */
        FILE *bcpkg21 = fopen("bcpkg21:/", "rb");
        if (bcpkg21 == NULL)  {
            printf("Error: Failed to read Package2 from NAND!\n");
            generic_panic();
        }
        if (fseek(bcpkg21, 0x4000, SEEK_SET) != 0 || fread(package2, 1, PACKAGE2_SIZE_MAX, bcpkg21) < sizeof(package2_header_t)) {
            printf("Error: Failed to read Package2 from NAND!\n");
            fclose(bcpkg21);
            generic_panic();
        }
        fclose(bcpkg21);
    } else {
        memcpy(package2, package2_src, loader_ctx->package2_loadfile.load_size);
    }

    /* Patch package2, adding thermosphere + custom KIPs. */
    package2_rebuild_and_copy(package2);

    free(package2);

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
