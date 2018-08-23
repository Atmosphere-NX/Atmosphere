#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <malloc.h>
#include "utils.h"
#include "fs_utils.h"
#include "nxboot.h"
#include "nxfs.h"
#include "di.h"
#include "mc.h"
#include "se.h"
#include "pmc.h"
#include "cluster.h"
#include "flow.h"
#include "timers.h"
#include "key_derivation.h"
#include "package1.h"
#include "package2.h"
#include "loader.h"
#include "splash_screen.h"
#include "exocfg.h"
#include "display/video_fb.h"
#include "lib/ini.h"

#define u8 uint8_t
#define u32 uint32_t
#include "exosphere_bin.h"
#undef u8
#undef u32

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

static uint32_t nxboot_get_target_firmware(const void *package1loader) {
    const package1loader_header_t *package1loader_header = (const package1loader_header_t *)package1loader;
    switch (package1loader_header->version) {
        case 0x01:          /* 1.0.0 */
            return EXOSPHERE_TARGET_FIRMWARE_100;
        case 0x02:          /* 2.0.0 - 2.3.0 */
            return EXOSPHERE_TARGET_FIRMWARE_200;
        case 0x04:          /* 3.0.0 and 3.0.1 - 3.0.2 */
            return EXOSPHERE_TARGET_FIRMWARE_300;
        case 0x07:          /* 4.0.0 - 4.1.0 */
            return EXOSPHERE_TARGET_FIRMWARE_400;
        case 0x0B:          /* 5.0.0 - 5.1.0 */
            return EXOSPHERE_TARGET_FIRMWARE_500;
        default:
            return 0;
    }
}

static void nxboot_configure_exosphere(uint32_t target_firmware) {
    exosphere_config_t exo_cfg = {0};

    exo_cfg.magic = MAGIC_EXOSPHERE_BOOTCONFIG;
    exo_cfg.target_firmware = target_firmware;

    if (ini_parse_string(get_loader_ctx()->bct0, exosphere_ini_handler, &exo_cfg) < 0) {
        fatal_error("[NXBOOT]: Failed to parse BCT.ini!\n");
    }

    if ((exo_cfg.target_firmware < EXOSPHERE_TARGET_FIRMWARE_MIN) || (exo_cfg.target_firmware > EXOSPHERE_TARGET_FIRMWARE_MAX)) {
        fatal_error("[NXBOOT]: Invalid Exosphere target firmware!\n");
    }

    *(MAILBOX_EXOSPHERE_CONFIGURATION) = exo_cfg;
}

/* This is the main function responsible for booting Horizon. */
static nx_keyblob_t __attribute__((aligned(16))) g_keyblobs[32];
void nxboot_main(void) {
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    volatile tegra_se_t *se = se_get_regs();
    loader_ctx_t *loader_ctx = get_loader_ctx();
    /* void *bootconfig; */
    package2_header_t *package2;
    size_t package2_size;
    void *tsec_fw;
    size_t tsec_fw_size;
    void *warmboot_fw;
    size_t warmboot_fw_size;
    void *warmboot_memaddr;
    void *package1loader;
    size_t package1loader_size;
    package1_header_t *package1;
    size_t package1_size;
    uint32_t available_revision;
    FILE *boot0, *pk2file;
    void *exosphere_memaddr;

    /* Allocate memory for reading Package2. */
    package2 = memalign(4096, PACKAGE2_SIZE_MAX);
    if (package2 == NULL) {
        fatal_error("[NXBOOT]: Out of memory!\n");
    }

    /* Read Package2 from a file, otherwise from its partition(s). */
    printf("[NXBOOT]: Reading package2...\n");
    if (loader_ctx->package2_path[0] != '\0') {
        pk2file = fopen(loader_ctx->package2_path, "rb");
        if (pk2file == NULL) {
            fatal_error("[NXBOOT]: Failed to open Package2 from %s: %s!\n", loader_ctx->package2_path, strerror(errno));
        }
    } else {
        pk2file = fopen("bcpkg21:/", "rb");
        if (pk2file == NULL || fseek(pk2file, 0x4000, SEEK_SET) != 0) {
            printf("[NXBOOT]: Failed to open Package2 from eMMC: %s!\n", strerror(errno));
            fclose(pk2file);
            generic_panic();
        }
    }

    setvbuf(pk2file, NULL, _IONBF, 0); /* Workaround. */
    if (fread(package2, sizeof(package2_header_t), 1, pk2file) < 1) {
        fatal_error("[NXBOOT]: Failed to read Package2!\n");
    }
    package2_size = package2_meta_get_size(&package2->metadata);
    if (package2_size > PACKAGE2_SIZE_MAX || package2_size <= sizeof(package2_header_t)) {
        fatal_error("[NXBOOT]: Package2 is too big or too small!\n");
    }
    if (fread(package2->data, package2_size - sizeof(package2_header_t), 1, pk2file) < 1) {
        fatal_error("[NXBOOT]: Failed to read Package2!\n");
    }
    fclose(pk2file);
    
    /* Read and parse boot0. */
    printf("[NXBOOT]: Reading boot0...\n");
    boot0 = fopen("boot0:/", "rb");
    if (boot0 == NULL || package1_read_and_parse_boot0(&package1loader, &package1loader_size, g_keyblobs, &available_revision, boot0) == -1) {
        fatal_error("[NXBOOT]: Couldn't parse boot0: %s!\n", strerror(errno));
    }
    fclose(boot0);

    /* Read the TSEC firmware from a file, otherwise from PK1L. */
    if (loader_ctx->tsecfw_path[0] != '\0') {
        tsec_fw_size = get_file_size(loader_ctx->tsecfw_path);
        if (tsec_fw_size != 0 && tsec_fw_size != 0xF00) {
            fatal_error("[NXBOOT]: TSEC firmware from %s has a wrong size!\n", loader_ctx->tsecfw_path);
        } else if (tsec_fw_size == 0) {
            fatal_error("[NXBOOT]: Could not read the TSEC firmware from %s!\n", loader_ctx->tsecfw_path);
        }

        tsec_fw = memalign(0x100, tsec_fw_size);
        if (tsec_fw == NULL) {
            fatal_error("[NXBOOT]: Out of memory!\n");
        }
        if (read_from_file(tsec_fw, tsec_fw_size, loader_ctx->tsecfw_path) != tsec_fw_size) {
            fatal_error("[NXBOOT]: Could not read the TSEC firmware from %s!\n", loader_ctx->tsecfw_path);
        }
    } else {
        tsec_fw_size = package1_get_tsec_fw(&tsec_fw, package1loader, package1loader_size);
        if (tsec_fw_size == 0) {
            fatal_error("[NXBOOT]: Failed to read the TSEC firmware from Package1loader!\n");
        }
    }

    /* TODO: Validate that we're capable of booting. */

    /* TODO: Initialize Boot Reason. */
    
    /* Find the system's target firmware. */
    uint32_t target_firmware = nxboot_get_target_firmware(package1loader);
    if (!target_firmware)
        fatal_error("[NXBOOT]: Failed to detect target firmware!\n");
    else
        printf("[NXBOOT]: Detected target firmware %ld!\n", target_firmware);

    /* Setup boot configuration for Exosphère. */
    nxboot_configure_exosphere(target_firmware);
    
    /* Derive keydata. */
    if (derive_nx_keydata(MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware, g_keyblobs, available_revision, tsec_fw, tsec_fw_size) != 0) {
        fatal_error("[NXBOOT]: Key derivation failed!\n");
    }

    /* Read the warmboot firmware from a file, otherwise from PK1. */
    if (loader_ctx->warmboot_path[0] != '\0') {
        warmboot_fw_size = get_file_size(loader_ctx->warmboot_path);
        if (warmboot_fw_size == 0) {
            fatal_error("[NXBOOT]: Could not read the warmboot firmware from %s!\n", loader_ctx->warmboot_path);
        }

        warmboot_fw = malloc(warmboot_fw_size);
        if (warmboot_fw == NULL) {
            fatal_error("[NXBOOT]: Out of memory!\n");
        }
        if (read_from_file(warmboot_fw, warmboot_fw_size, loader_ctx->warmboot_path) != warmboot_fw_size) {
            fatal_error("[NXBOOT]: Could not read the warmboot firmware from %s!\n", loader_ctx->warmboot_path);
        }
    } else {
        uint8_t ctr[16];
        package1_size = package1_get_encrypted_package1(&package1, ctr, package1loader, package1loader_size);
        if (package1_decrypt(package1, package1_size, ctr)) {
            warmboot_fw = package1_get_warmboot_fw(package1);
            warmboot_fw_size = package1->warmboot_size;
        } else {
            warmboot_fw = NULL;
            warmboot_fw_size = 0;
        }

        if (warmboot_fw_size == 0) {
            fatal_error("[NXBOOT]: Could not read the warmboot firmware from Package1!\n");
        }
    }
    
    /* Select the right address for the warmboot firmware. */
    if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware < EXOSPHERE_TARGET_FIRMWARE_400) {
        warmboot_memaddr = (void *)0x8000D000;
    } else {
        warmboot_memaddr = (void *)0x4003B000;
    }
    
    printf("[NXBOOT]: Copying warmboot firmware...\n");
    
    /* Copy the warmboot firmware and set the address in PMC if necessary. */
    if (warmboot_fw && (warmboot_fw_size > 0)) {
        memcpy(warmboot_memaddr, warmboot_fw, warmboot_fw_size);
        if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware < EXOSPHERE_TARGET_FIRMWARE_400)
            pmc->scratch1 = (uint32_t)warmboot_memaddr;
    }
    
    printf("[NXBOOT]: Rebuilding package2...\n");
    
    /* Patch package2, adding Thermosphère + custom KIPs. */
    package2_rebuild_and_copy(package2, MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware);

    printf(u8"Reading Exosphère...\n");
    
    /* Select the right address for Exosphère. */
    if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware <= EXOSPHERE_TARGET_FIRMWARE_400) {
        exosphere_memaddr = (void *)0x40020000;
    } else {
        exosphere_memaddr = (void *)0x40018000; /* 5.x has its secmon's crt0 around this region. */
    }

    /* Copy Exosphère to a good location or read it directly to it. */
    if (loader_ctx->exosphere_path[0] != '\0') {
        size_t exosphere_size = get_file_size(loader_ctx->exosphere_path);
        if (exosphere_size == 0) {
            fatal_error(u8"Could not read Exosphère from %s!\n", loader_ctx->exosphere_path);
        } else if (exosphere_size > 0x10000) {
            /* The maximum is actually a bit less than that. */
            fatal_error(u8"Exosphère from %s is too big!\n", loader_ctx->exosphere_path);
        }

        if (read_from_file(exosphere_memaddr, exosphere_size, loader_ctx->exosphere_path) != exosphere_size) {
            fatal_error(u8"Could not read Exosphère from %s!\n", loader_ctx->exosphere_path);
        }
    } else {
        memcpy(exosphere_memaddr, exosphere_bin, exosphere_bin_size);
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
    
    /* Unmount everything. */
    nxfs_unmount_all();
    
    /* Clear used keyslots. */
    clear_aes_keyslot(KEYSLOT_SWITCH_PACKAGE2KEY);
    clear_aes_keyslot(KEYSLOT_SWITCH_RNGKEY);
    
    /* Lock keyslots. */
    set_aes_keyslot_flags(KEYSLOT_SWITCH_MASTERKEY, 0xFF);
    if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware < EXOSPHERE_TARGET_FIRMWARE_400) {
        set_aes_keyslot_flags(KEYSLOT_SWITCH_DEVICEKEY, 0xFF);
    } else {
        set_aes_keyslot_flags(KEYSLOT_SWITCH_4XOLDDEVICEKEY, 0xFF);
    }

    /* TODO: Handle bootconfig. */
    memset((void *)0x4003D000, 0, 0x3000);
    
    /* Finalize the GPU UCODE carveout. */
    mc_config_carveout_finalize();
    
    /* Lock AES keyslots. */
    for (uint32_t i = 0; i < 16; i++)
        set_aes_keyslot_flags(i, 0x15);

    /* Lock RSA keyslots. */
    for (uint32_t i = 0; i < 2; i++)
        set_rsa_keyslot_flags(i, 1);

    /* Lock the Security Engine. */
    se->_0x4 = 0;
    se->AES_KEY_READ_DISABLE_REG = 0;
    se->RSA_KEY_READ_DISABLE_REG = 0;
    se->_0x0 &= 0xFFFFFFFB;
    
    /* Boot up Exosphère. */
    MAILBOX_NX_BOOTLOADER_IS_SECMON_AWAKE = 0;
    if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware < EXOSPHERE_TARGET_FIRMWARE_400) {
        MAILBOX_NX_BOOTLOADER_SETUP_STATE = NX_BOOTLOADER_STATE_LOADED_PACKAGE2;
    } else {
        MAILBOX_NX_BOOTLOADER_SETUP_STATE = NX_BOOTLOADER_STATE_LOADED_PACKAGE2_4X;
    }
    
    printf("[NXBOOT]: Powering on the CCPLEX...\n");
    
    /* Display splash screen. */
    display_splash_screen_bmp(loader_ctx->custom_splash_path);
    
    /* Turn off the backlight. */
    display_backlight(false);
    
    /* Terminate the display. */
    display_end();
    
    /* Boot CPU0. */
    cluster_boot_cpu0((uint64_t)(uintptr_t)exosphere_memaddr);
    
    /* Wait for Exosphère to wake up. */
    while (MAILBOX_NX_BOOTLOADER_IS_SECMON_AWAKE == 0) {
        udelay(1);
    }
    
    /* Signal Exosphère. */
    if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware < EXOSPHERE_TARGET_FIRMWARE_400) {
        MAILBOX_NX_BOOTLOADER_SETUP_STATE = NX_BOOTLOADER_STATE_FINISHED;
    } else {
        MAILBOX_NX_BOOTLOADER_SETUP_STATE = NX_BOOTLOADER_STATE_FINISHED_4X;
    }

    /* Halt ourselves in waitevent state. */
    while (1) {
        FLOW_CTLR_HALT_COP_EVENTS_0 = 0x50000000;
    }
}
