#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "sdmmc.h"
#include "raw_mmc_dev.h"
#include "utils.h"
#include "nxboot.h"
#include "key_derivation.h"
#include "gpt.h"
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

static struct mmc nand_mmc; /* TODO: Remove, move it elsewhere and actually initalize the controller!! */

static int init_rawnand_and_boot0_devices(void) {
    if (rawmmcdev_mount_unencrypted_device("rawnand", &nand_mmc, SDMMC_PARTITION_USER, 0, 32ull<<30) == -1) {
        return -1;
    }
    if (rawmmcdev_mount_unencrypted_device("boot0", &nand_mmc, SDMMC_PARTITION_BOOT0, 0, 0x184000) == -1) {
        return -1;
    }

    return 0;
}

static bool g_bcpkg2_initialized = false;
static int bcpkg2_finder_callback(const efi_entry_t *entry, size_t entry_offset, FILE *disk) {
    /* TODO: what about backup partitions? */
    static const uint16_t part_name[] = u"BCPKG2-1-Normal-Main";
    if (memcmp(entry->name, part_name, sizeof(part_name)) == 0) {
        uint64_t part_offset = 512 * entry->first_lba;
        uint64_t part_size = 512 * (entry->last_lba - entry->first_lba);

        int rc = rawmmcdev_mount_unencrypted_device("bcpkg2", &nand_mmc, SDMMC_PARTITION_USER, part_offset, part_size);

        g_bcpkg2_initialized = rc == 0;
        return rc;
    }

    return 0;
}

/* Find BCPKG2-1-Normal-Main using the GPT, then register it. */
static int init_bcpkg2_device(void) {
    FILE *rawnand = fopen("rawnand:/", "rb");
    int rc;
    if (rawnand == NULL) {
        return -1;
    }

    rc = gpt_iterate_through_entries(rawnand, bcpkg2_finder_callback);
    fclose(rawnand);

    if (rc == 0 && !g_bcpkg2_initialized) {
        errno = ENOENT;
        return -1;
    }

    return rc;
}

/* This is the main function responsible for booting Horizon. */
void nxboot_main(void) {
    loader_ctx_t *loader_ctx = get_loader_ctx();

    /* TODO: this is not always necessary */
    if (init_rawnand_and_boot0_devices()) {
        printf("Error: Failed to mount rawnand and/or boot0: %s!\n", strerror(errno));
        generic_panic();
    }
    /* TODO: Validate that we're capable of booting. */

    /* TODO: Initialize Boot Reason. */

    /* TODO: How should we deal with bootconfig? */

    /* Setup boot configuration for Exosphere. */
    nxboot_configure_exosphere();

    /* Derive keydata. */
    derive_nx_keydata(MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware);

    if (loader_ctx->package2_loadfile.load_address == 0) {
        if (init_bcpkg2_device() == -1) {
            printf("Error: Failed to mount BCPKG2-1-Normal-Main: %s!\n", strerror(errno));
            generic_panic();
        }

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

    rawmmcdev_unmount_all();

    /* TODO: Halt ourselves. */
}
