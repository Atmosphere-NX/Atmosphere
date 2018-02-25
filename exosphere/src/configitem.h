#ifndef EXOSPHERE_CFG_ITEM_H
#define EXOSPHERE_CFG_ITEM_H

#include <stdbool.h>
#include <stdint.h>

enum ConfigItem {
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
    CONFIGITEM_BATTERYPROFILE = 13
};

uint32_t configitem_set(enum ConfigItem item, uint64_t value);
uint32_t configitem_get(enum ConfigItem item, uint64_t *p_outvalue);

bool configitem_is_recovery_boot(void);
bool configitem_is_retail(void);

#endif
