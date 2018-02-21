#include <stdint.h>

#include "utils.h"

#include "package2.h"
#include "configitem.h"
#include "se.h"
#include "masterkey.h"
#include "cache.h"
#include "randomcache.h"
#include "timers.h"

void setup_mmio_virtual_addresses(void) {
    /* TODO: Set Timers address to 0x1F008B000. */
    /* TODO: Set Security Engine address to 0x1F008F000. */
    /* TODO: Set CAR address to 0x1F0087000. */
    /* TODO: Set PMC address to 0x1F0089400. */
    /* TODO: Set Fuse address to 0x1F0096800. */
    /* TODO: Set Interrupt addresses to 0x1F0080000, 0x1F0082000. */
    /* TODO: Set Flow Controller address to 0x1F009D000. */
    /* TODO: Set UART-A address to 0x1F0085000. */
    /* TODO: Set I2C-0 address to 0x1F00A5000. */
    /* TODO: Set I2C-4 address to 0x1F00A1000. */
    /* TODO: Set MISC address to 0x70000000. */
    /* TODO: Set GPIO-1 address to 0x6000D000. */
}

/* Hardware init, sets up the RNG and SESSION keyslots, derives new DEVICE key. */
void setup_se(void) {
    uint8_t work_buffer[0x10];
    
    /* Sanity check the Security Engine. */
    se_verify_flags_cleared();
    se_clear_interrupts();
    
    /* Perform some sanity initialization. */
    security_engine_t *p_security_engine = get_security_engine_address();
    p_security_engine->_0x4 = 0;
    p_security_engine->AES_KEY_READ_DISABLE_REG = 0;
    p_security_engine->RSA_KEY_READ_DISABLE_REG = 0;
    p_security_engine->_0x0 &= 0xFFFFFFFB;
    
    /* Currently unknown what each flag does. */
    for (unsigned int i = 0; i < KEYSLOT_AES_MAX; i++) {
        set_aes_keyslot_flags(i, 0x15);
    }
    
    for (unsigned int i = 4; i < KEYSLOT_AES_MAX; i++) {
        set_aes_keyslot_flags(i, 0x40);
    }
    
    for (unsigned int i = 0; i < KEYSLOT_RSA_MAX; i++) {
        set_rsa_keyslot_flags(i, 0x41);
    }
    
    /* Detect Master Key revision. */
    mkey_detect_revision();
    
    /* Setup new device key, if necessary. */
    if (mkey_get_revision() >= MASTERKEY_REVISION_400_CURRENT) {
        const uint8_t 4x_new_devicekey_source[0x10] = {0x8B, 0x4E, 0x1C, 0x22, 0x42, 0x07, 0xC8, 0x73, 0x56, 0x94, 0x08, 0x8B, 0xCC, 0x47, 0x0F, 0x5D};
        se_aes_ecb_decrypt_block(KEYSLOT_SWITCH_4XNEWDEVICEKEYGENKEY, work_buffer, 0x10, 4x_new_devicekey_source, 0x10);
        decrypt_data_into_keyslot(KEYSLOT_SWITCH_DEVICEKEY, KEYSLOT_SWITCH_4XNEWCONSOLEKEYGENKEY, work_buffer, 0x10);
        clear_aes_keyslot(KEYSLOT_SWITCH_4XNEWCONSOLEKEYGENKEY);
        set_aes_keyslot_flags(KEYSLOT_SWITCH_DEVICEKEY, 0xFF);
    }
    
    se_initialize_rng(KEYSLOT_SWITCH_DEVICEKEY);
    
    /* Generate random data, transform with device key to get RNG key. */
    se_generate_random(KEYSLOT_SWITCH_DEVICEKEY, work_buffer, 0x10);
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_RNGKEY, KEYSLOT_SWITCH_DEVICEKEY, work_buffer, 0x10);
    set_aes_keyslot_flags(KEYSLOT_SWITCH_RNGKEY, 0xFF);
    
    /* Repeat for Session key. */
    se_generate_random(KEYSLOT_SWITCH_DEVICEKEY, work_buffer, 0x10);
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_SESSIONKEY, KEYSLOT_SWITCH_DEVICEKEY, work_buffer, 0x10);
    set_aes_keyslot_flags(KEYSLOT_SWITCH_SESSIONKEY, 0xFF);
    
    /* TODO: Create Test Vector, to validate keyslot data is unchanged post warmboot. */
}

void setup_boot_config(void) {
    /* Load boot config only if dev unit. */
    if (configitem_is_retail()) {
        bootconfig_clear();
    } else {
        flush_dcache_range((uint8_t *)NX_BOOTLOADER_BOOTCONFIG_POINTER, (uint8_t *)NX_BOOTLOADER_BOOTCONFIG_POINTER + sizeof(bootconfig_t));
        bootconfig_load_and_verify((bootconfig_t *)NX_BOOTLOADER_BOOTCONFIG_POINTER);
    }
}

/* This function is called during coldboot crt0, and validates a package2. */
/* This package2 is read into memory by a concurrent BPMP bootloader. */
void load_package2(void) {
    /* Setup MMIO virtual pointers. */
    setup_mmio_virtual_addresses();
    
    /* Setup the Security Engine. */
    setup_se();
    
    /* TODO: bootup_misc_mmio(). */
    /* This func will also be called on warmboot. */
    /* And will verify stored SE Test Vector, clear keyslots, */
    /* Generate an SRK, set the warmboot firmware location, */
    /* Configure the GPU uCode carveout, configure the Kernel default carveouts, */
    /* Initialize the PMC secure scratch registers, initialize MISC registers, */
    /* And assign "se_operation_completed" to Interrupt 0x5A. */
    
    /* TODO: Read and save BOOTREASON stored by NX_BOOTLOADER at 0x1F009FE00 */
    
    /* Initialize cache'd random bytes for kernel. */
    randomcache_init();
    
    /* TODO: memclear the initial copy of Exosphere running in IRAM (relocated to TZRAM by earlier code). */
    
    /* Let NX Bootloader know that we're running. */
    MAILBOX_NX_BOOTLOADER_IS_SECMON_AWAKE = 1;
    
    /* Synchronize with NX BOOTLOADER. */
    if (MAILBOX_NX_BOOTLOADER_SETUP_STATE == NX_BOOTLOADER_STATE_INIT) {
        while (MAILBOX_NX_BOOTLOADER_SETUP_STATE < NX_BOOTLOADER_STATE_MOVED_BOOTCONFIG) {
            wait(1);
        }
    }

    /* Load Boot Config into global. */
    setup_boot_config();
    
    /* Synchronize with NX BOOTLOADER. */
    if (MAILBOX_NX_BOOTLOADER_SETUP_STATE == NX_BOOTLOADER_STATE_MOVED_BOOTCONFIG) {
        while (MAILBOX_NX_BOOTLOADER_SETUP_STATE < NX_BOOTLOADER_STATE_LOADED_PACKAGE2) {
            wait(1);
        }
    }
    
    /* TODO: Parse/load Package2. */
    
}