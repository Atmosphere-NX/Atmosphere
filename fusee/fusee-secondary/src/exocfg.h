#ifndef FUSEE_EXOSPHERE_CONFIG_H
#define FUSEE_EXOSPHERE_CONFIG_H

/* This serves to set configuration for *exosphere itself*, separate from the SecMon Exosphere mimics. */

/* "XBC0" */
#define MAGIC_EXOSPHERE_BOOTCONFIG (0x30434258)

#define EXOSPHERE_TARGET_FIRMWARE_100 1
#define EXOSPHERE_TARGET_FIRMWARE_200 2
#define EXOSPHERE_TARGET_FIRMWARE_300 3
#define EXOSPHERE_TARGET_FIRMWARE_400 4
#define EXOSPHERE_TARGET_FIRMWARE_500 5

#define EXOSPHERE_TARGET_FIRMWARE_MIN EXOSPHERE_TARGET_FIRMWARE_100
#define EXOSPHERE_TARGET_FIRMWARE_MAX EXOSPHERE_TARGET_FIRMWARE_500

typedef struct {
    unsigned int magic;
    unsigned int target_firmware;
} exosphere_config_t;

#define MAILBOX_EXOSPHERE_CONFIGURATION ((volatile exosphere_config_t *)(0x40002E40))

#define EXOSPHERE_TARGETFW_KEY "target_firmware"

#endif