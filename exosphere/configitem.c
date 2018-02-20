#include <stdint.h>

#include "utils.h"
#include "configitem.h"

int g_battery_profile = 0;

uint32_t configitem_set(enum ConfigItem item, uint64_t value) {
    if (item != CONFIGITEM_BATTERYPROFILE) {
        return 2;
    }
    
    g_battery_profile = ((int)(value != 0)) & 1;
}

uint64_t configitem_is_recovery_boot(void) {
    uint64_t is_recovery_boot;
    if (configitem_get(CONFIGITEM_ISRECOVERYBOOT, &is_recovery_boot) != 0) {
        panic();
    }
    
    return is_recovery_boot;
}

uint32_t configitem_get(enum ConfigItem item, uint64_t *p_outvalue) {
    uint32_t result = 0;
    switch (item) {
        case CONFIGITEM_DISABLEPROGRAMVERIFICATION:
            /* TODO: This is loaded from BootConfig on dev units, always zero on retail. How should we support? */
            *p_outvalue = 0;
            break;
        case CONFIGITEM_MEMORYCONFIGURATION:
            /* TODO: Fuse driver */
            break;
        case CONFIGITEM_SECURITYENGINEIRQ:
            /* SE is interrupt #44. */
            *p_outvalue = 0x2C;
            break;
        case CONFIGITEM_UNK04:
            /* Always returns 2 on hardware. */
            *p_outvalue = 2;
            break;
        case CONFIGITEM_HARDWARETYPE:
            /* TODO: Fuse driver */
            break;
        case CONFIGITEM_ISRETAIL:
            /* TODO: Fuse driver */
            break;
        case CONFIGITEM_ISRECOVERYBOOT:
            /* TODO: This is just a constant, hardcoded into TZ on retail. How should we support? */
            *p_outvalue = 0;
            break;
        case CONFIGITEM_DEVICEID:
            /* TODO: Fuse driver */
            break;
        case CONFIGITEM_BOOTREASON:
            /* TODO: This requires reading values passed to crt0 via NX_Bootloader. TBD pending crt0 implementation. */
            break;
        case CONFIGITEM_MEMORYARRANGE:
            /* TODO: More BootConfig stuff. */
            break;
        case CONFIGITEM_ISDEBUGMODE:
            /* TODO: More BootConfig stuff. */
            break;
        case CONFIGITEM_KERNELMEMORYCONFIGURATION:
            /* TODO: More BootConfig stuff. */
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
