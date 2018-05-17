#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <malloc.h>
#include "utils.h"
#include "fs_utils.h"
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

static void nxboot_configure_exosphere(void) {
    exosphere_config_t exo_cfg = {0};

    exo_cfg.magic = MAGIC_EXOSPHERE_BOOTCONFIG;
    exo_cfg.target_firmware = 0;

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

static void nxboot_adjust_exosphere_target_firmware(const package2_header_t *package2) {
    static const uint32_t mkey_ver_to_target_fw[] = {
        EXOSPHERE_TARGET_FIRMWARE_100,
        EXOSPHERE_TARGET_FIRMWARE_200,
        EXOSPHERE_TARGET_FIRMWARE_300,
        EXOSPHERE_TARGET_FIRMWARE_300,
        EXOSPHERE_TARGET_FIRMWARE_400,
        EXOSPHERE_TARGET_FIRMWARE_500,
    };
    uint8_t package2_header_version = package2_meta_get_header_version(&package2->metadata);
    if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware == 0) {
        if (package2_header_version >= 1 && package2_header_version <= sizeof(mkey_ver_to_target_fw)/4) {
            MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware = mkey_ver_to_target_fw[package2_header_version];
        }
    }
}

/* This is the main function responsible for booting Horizon. */
static nx_keyblob_t __attribute__((aligned(16))) g_keyblobs[32];
void nxboot_main(void) {
    loader_ctx_t *loader_ctx = get_loader_ctx();
    /* void *bootconfig; */
    package2_header_t *package2;
    size_t package2_size;
    void *tsec_fw;
    size_t tsec_fw_size;
    void *warmboot_fw;
    size_t warmboot_fw_size;
    void *package1loader;
    size_t package1loader_size ;
    package1_header_t *package1;
    size_t package1_size;
    uint32_t available_revision;
    FILE *boot0, *pk2file;
    void *exosphere_memaddr;

    /* TODO: How should we deal with bootconfig? */
    package2 = memalign(4096, PACKAGE2_SIZE_MAX);
    if (package2 == NULL) {
        printf("Error: nxboot: out of memory!\n");
        generic_panic();
    }

    /* Read Package2 from a file, otherwise from its partition(s). */
    if (loader_ctx->package2_path[0] != '\0') {
        pk2file = fopen(loader_ctx->package2_path, "rb");
        if (pk2file == NULL) {
            printf("Error: Failed to open Package2 from %s: %s!\n", loader_ctx->package2_path, strerror(errno));
            generic_panic();
        }
    } else {
#ifdef I_KNOW_WHAT_IM_DOING_2
        pk2file = fopen("bcpkg21:/", "rb");
        if (pk2file == NULL || fseek(pk2file, 0x4000, SEEK_SET) != 0) {
            printf("Error: Failed to open Package2 from NAND: %s!\n", strerror(errno));
            fclose(pk2file);
            generic_panic();
        }
#else
        printf("Error: Package2 must be loaded from the SD card, unless you know what you are doing!\n");
        generic_panic();
#endif
    }

    setvbuf(pk2file, NULL, _IONBF, 0); /* Workaround. */
    if (fread(package2, sizeof(package2_header_t), 1, pk2file) < 1) {
        printf("Error: Failed to read Package2!\n");
        generic_panic();
    }

    package2_size = package2_meta_get_size(&package2->metadata);
    if (package2_size > PACKAGE2_SIZE_MAX || package2_size <= sizeof(package2_header_t)) {
        printf("Error: Package2 is too big or too small!\n");
        generic_panic();
    }
/*
    if (fread(package2->data, package2_size - sizeof(package2_header_t), 1, pk2file) < 1) {
        printf("Error: Failed to read Package2!\n");
        generic_panic();
    }
*/
    for (size_t i = 0; i < package2_size - sizeof(package2_header_t); i += 512*32) {
        /* TODO: check if we have read everything, eventually. */
        size_t r = fread(package2->data + i, 1, 512, pk2file);
        if (r == 0) {
            printf("Error: Failed to read Package2!\n");
            generic_panic();
        } else if (r < 512) {
            break;
        }
    }
    fclose(pk2file);
    printf("Read package2!\n");

    /* Setup boot configuration for Exosphère. */
    nxboot_configure_exosphere();
    nxboot_adjust_exosphere_target_firmware(package2);

    if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware == 0) {
        printf("Error: Failed to determine which target firmware too use!\n");
        generic_panic();
    }

    printf("Reading boot0...\n");
    boot0 = fopen("boot0:/", "rb");
    if (boot0 == NULL || package1_read_and_parse_boot0(&package1loader, &package1loader_size, g_keyblobs, &available_revision, boot0) == -1) {
        printf("Error: Couldn't parse boot0: %s!\n", strerror(errno));
        generic_panic();
    }
    fclose(boot0);

    /* Read the TSEC firmware from a file, otherwise from PK1L. */
    if (loader_ctx->tsecfw_path[0] != '\0') {
        tsec_fw_size = get_file_size(loader_ctx->tsecfw_path);
        if (tsec_fw_size != 0 && tsec_fw_size != 0xF00) {
            printf("Error: TSEC firmware from %s has a wrong size!\n", loader_ctx->tsecfw_path);
            generic_panic();
        } else if (tsec_fw_size == 0) {
            printf("Error: Could not read the TSEC firmware from %s!\n", loader_ctx->tsecfw_path);
            generic_panic();
        }

        tsec_fw = memalign(0x100, tsec_fw_size);
        if (tsec_fw == NULL) {
            printf("Error: nxboot_main: out of memory!\n");
            generic_panic();
        }
        if (read_from_file(tsec_fw, tsec_fw_size, loader_ctx->tsecfw_path) != tsec_fw_size) {
            printf("Error: Could not read the TSEC firmware from %s!\n", loader_ctx->tsecfw_path);
            generic_panic();
        }
    } else {
        tsec_fw_size = package1_get_tsec_fw(&tsec_fw, package1loader, package1loader_size);
        if (tsec_fw_size == 0) {
            printf("Error: Failed to read the TSEC firmware from Package1loader!\n");
            generic_panic();
        }
    }

    /* TODO: Validate that we're capable of booting. */

    /* TODO: Initialize Boot Reason. */

    /* Derive keydata. */
    if (derive_nx_keydata(MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware, g_keyblobs, available_revision, tsec_fw, tsec_fw_size) != 0) {
        printf("Error: Key derivation failed!\n");
        generic_panic();
    }

    /* Read the warmboot firmware from a file, otherwise from PK1. */
    if (loader_ctx->warmboot_path[0] != '\0') {
        warmboot_fw_size = get_file_size(loader_ctx->warmboot_path);
        if (warmboot_fw_size == 0) {
            printf("Error: Could not read the warmboot firmware from %s!\n", loader_ctx->warmboot_path);
            generic_panic();
        }

        warmboot_fw = malloc(warmboot_fw_size);
        if (warmboot_fw == NULL) {
            printf("Error: nxboot_main: out of memory!\n");
            generic_panic();
        }
        if (read_from_file(warmboot_fw, warmboot_fw_size, loader_ctx->warmboot_path) != warmboot_fw_size) {
            printf("Error: Could not read the warmboot firmware from %s!\n", loader_ctx->warmboot_path);
            generic_panic();
        }
    } else {
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
            printf("Error: Could not read the warmboot firmware from Package1!\n");
            generic_panic();
        }
    }

    printf("Rebuilding package2...\n");
    /* Patch package2, adding Thermosphère + custom KIPs. */
    package2_rebuild_and_copy(package2, MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware);
    printf(u8"Loading Exosphère...\n");

    /* Copy Exophère to a good location (or read it directly to it.) */
    if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware <= EXOSPHERE_TARGET_FIRMWARE_400) {
        exosphere_memaddr = (void *)0x40020000;
    } else {
        exosphere_memaddr = (void *)0x40018000; /* 5.x has its secmon's crt0 around this region. */
    }

    if (loader_ctx->exosphere_path[0] != '\0') {
        size_t exosphere_size = get_file_size(loader_ctx->exosphere_path);
        if (exosphere_size == 0) {
            printf(u8"Error: Could not read Exosphère from %s!\n", loader_ctx->exosphere_path);
            generic_panic();
        } else if (exosphere_size > 0x10000) {
            /* The maximum is actually a bit less than that. */
            printf(u8"Error: Exosphère from %s is too big!\n", loader_ctx->exosphere_path);
            generic_panic();
        }

        if (read_from_file(exosphere_memaddr, exosphere_size, loader_ctx->exosphere_path) != exosphere_size) {
            printf(u8"Error: Could not read Exosphère from %s!\n", loader_ctx->exosphere_path);
            generic_panic();
        }
    } else {
        extern const uint8_t exosphere_bin[];
        extern const uint32_t exosphere_bin_size;
        memcpy(exosphere_memaddr, exosphere_bin, exosphere_bin_size);
    }

    /* Boot up Exosphère. */
    MAILBOX_NX_BOOTLOADER_IS_SECMON_AWAKE = 0;
    if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware < EXOSPHERE_TARGET_FIRMWARE_400) {
        MAILBOX_NX_BOOTLOADER_SETUP_STATE = NX_BOOTLOADER_STATE_LOADED_PACKAGE2;
    } else {
        MAILBOX_NX_BOOTLOADER_SETUP_STATE = NX_BOOTLOADER_STATE_LOADED_PACKAGE2_4X;
    }
    cluster_enable_cpu0((uint64_t)(uintptr_t)exosphere_memaddr, 1);
    while (MAILBOX_NX_BOOTLOADER_IS_SECMON_AWAKE == 0) {
        /* Wait for Exosphere to wake up. */
    }
    if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware < EXOSPHERE_TARGET_FIRMWARE_400) {
        MAILBOX_NX_BOOTLOADER_SETUP_STATE = NX_BOOTLOADER_STATE_FINISHED;
    } else {
        MAILBOX_NX_BOOTLOADER_SETUP_STATE = NX_BOOTLOADER_STATE_FINISHED_4X;
    }

    /* Clean up. */
    free(package1loader);
    if (loader_ctx->tsecfw_path[0] != '\0') {
        free(tsec_fw);
    }
    if (loader_ctx->warmboot_path[0] != '\0') {
        free(warmboot_fw);
    }
    free(package2);

    /* Display splash screen. */
    display_splash_screen_bmp(loader_ctx->custom_splash_path);

    /* TODO: Halt ourselves. */
}
