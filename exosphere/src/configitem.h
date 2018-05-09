#ifndef EXOSPHERE_CFG_ITEM_H
#define EXOSPHERE_CFG_ITEM_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CONFIGITEM_DISABLEPROGRAMVERIFICATION = 1,
    CONFIGITEM_DRAMID = 2,
    CONFIGITEM_SECURITYENGINEIRQ = 3,
    CONFIGITEM_VERSION = 4,
    CONFIGITEM_HARDWARETYPE = 5,
    CONFIGITEM_ISRETAIL = 6,
    CONFIGITEM_ISRECOVERYBOOT = 7,
    CONFIGITEM_DEVICEID = 8,
    CONFIGITEM_BOOTREASON = 9,
    CONFIGITEM_MEMORYARRANGE = 10,
    CONFIGITEM_ISDEBUGMODE = 11,
    CONFIGITEM_KERNELMEMORYCONFIGURATION = 12,
    CONFIGITEM_BATTERYPROFILE = 13,
    CONFIGITEM_ODM4BIT10_4X = 14,
    CONFIGITEM_NEWHARDWARETYPE_5X = 15,
    CONFIGITEM_NEWKEYGENERATION_5X = 16,
    CONFIGITEM_PACKAGE2HASH_5X = 17,
    
    /* These are unofficial, for usage by Exosphere. */
    CONFIGITEM_EXOSPHERE_VERSION = 65000
} ConfigItem;

uint32_t configitem_set(ConfigItem item, uint64_t value);
uint32_t configitem_get(ConfigItem item, uint64_t *p_outvalue);

bool configitem_is_recovery_boot(void);
bool configitem_is_retail(void);
bool configitem_should_profile_battery(void);

uint64_t configitem_get_hardware_type(void);

#endif
