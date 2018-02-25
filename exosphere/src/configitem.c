#include <stdint.h>

#include "bootconfig.h"
#include "configitem.h"
#include "interrupt.h"
#include "package2.h"
#include "se.h"
#include "fuse.h"
#include "utils.h"

int g_battery_profile = 0;

uint32_t configitem_set(enum ConfigItem item, uint64_t value) {
    if (item != CONFIGITEM_BATTERYPROFILE) {
        return 2;
    }
    
    g_battery_profile = ((int)(value != 0)) & 1;
    return 0; /* FIXME: what should we return there */
}

bool configitem_is_recovery_boot(void) {
    uint64_t is_recovery_boot;
    if (configitem_get(CONFIGITEM_ISRECOVERYBOOT, &is_recovery_boot) != 0) {
        generic_panic();
    }

    return is_recovery_boot != 0;
}

bool configitem_is_retail(void) {
    uint64_t is_retail;
    if (configitem_get(CONFIGITEM_ISRETAIL, &is_retail) != 0) {
        generic_panic();
    }

    return is_retail != 0;
}

uint32_t configitem_get(enum ConfigItem item, uint64_t *p_outvalue) {
    uint32_t result = 0;
    switch (item) {
        case CONFIGITEM_DISABLEPROGRAMVERIFICATION:
            *p_outvalue = (int)(bootconfig_disable_program_verification());
            break;
        case CONFIGITEM_DRAMID:
            *p_outvalue = fuse_get_dram_id();
            break;
        case CONFIGITEM_SECURITYENGINEIRQ:
            /* SE is interrupt #0x2C. */
            *p_outvalue = INTERRUPT_ID_USER_SECURITY_ENGINE;
            break;
        case CONFIGITEM_VERSION:
            /* Always returns maxver - 1 on hardware. */
            *p_outvalue = PACKAGE2_MAXVER_400_CURRENT - 1;
            break;
        case CONFIGITEM_HARDWARETYPE:
            *p_outvalue = fuse_get_hardware_type();
            break;
        case CONFIGITEM_ISRETAIL:
            *p_outvalue = fuse_get_retail_type();
            break;
        case CONFIGITEM_ISRECOVERYBOOT:
            /* TODO: This requires reading values passed to crt0 via NX_Bootloader. TBD pending crt0 implementation. */
            *p_outvalue = 0;
            break;
        case CONFIGITEM_DEVICEID:
            *p_outvalue = fuse_get_device_id();
            break;
        case CONFIGITEM_BOOTREASON:
            /* TODO: This requires reading values passed to crt0 via NX_Bootloader. TBD pending crt0 implementation. */
            break;
        case CONFIGITEM_MEMORYARRANGE:
            *p_outvalue = bootconfig_get_memory_arrangement();
            break;
        case CONFIGITEM_ISDEBUGMODE:
            *p_outvalue = (int)(bootconfig_is_debug_mode());
            break;
        case CONFIGITEM_KERNELMEMORYCONFIGURATION:
            *p_outvalue = bootconfig_get_kernel_memory_configuration();
            break;
        case CONFIGITEM_BATTERYPROFILE:
            *p_outvalue = g_battery_profile;
            break;
        default:
            result = 2;
            break;
    }
    return result;
}
