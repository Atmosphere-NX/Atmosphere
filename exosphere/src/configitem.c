/*
 * Copyright (c) 2018-2019 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <stdint.h>
#include <atmosphere/version.h>

#include "bootconfig.h"
#include "configitem.h"
#include "interrupt.h"
#include "package2.h"
#include "se.h"
#include "fuse.h"
#include "utils.h"
#include "masterkey.h"
#include "exocfg.h"
#include "smc_ams.h"
#include "arm.h"

#define u8 uint8_t
#define u32 uint32_t
#include "rebootstub_bin.h"
#undef u8
#undef u32

static bool g_hiz_mode_enabled = false;
static bool g_debugmode_override_user = false, g_debugmode_override_priv = false;

uint32_t configitem_set(bool privileged, ConfigItem item, uint64_t value) {
    switch (item) {
        case CONFIGITEM_HIZMODE:
            g_hiz_mode_enabled = (value != 0);
            break;
        case CONFIGITEM_NEEDS_REBOOT:
            /* Force a reboot, if requested. */
            {
                switch (value) {
                    case REBOOT_KIND_NO_REBOOT:
                        return 0;
                    case REBOOT_KIND_TO_RCM:
                        /* Set reboot kind = rcm. */
                        MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x450ull) = 0x2;
                        break;
                    case REBOOT_KIND_TO_WB_PAYLOAD:
                        /* Set reboot kind = warmboot. */
                        MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x450ull) = 0x1;
                        /* Patch SDRAM init to perform an SVC immediately after second write */
                        MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x634ull) = 0x2E38DFFF;
                        MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x638ull) = 0x6001DC28;
                        /* Set SVC handler to jump to reboot stub in IRAM. */
                        MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x520ull) = 0x4003F000;
                        MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x53Cull) = 0x6000F208;
                        
                        /* Copy reboot stub payload. */
                        ams_map_irampage(0x4003F000);
                        for (unsigned int i = 0; i < rebootstub_bin_size; i += 4) {
                            MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_AMS_IRAM_PAGE) + i) = read32le(rebootstub_bin, i);
                        }
                        ams_unmap_irampage();
                        
                        /* Ensure stub is flushed. */
                        flush_dcache_all();
                        break;
                    default:
                        return 2;
                }
                MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x400ull) = 0x10;
                while (1) { }
            }
            break;
        case CONFIGITEM_NEEDS_SHUTDOWN:
            /* Force a shutdown, if requested. */
            {
                if (value == 0) {
                    return 0;
                }
                /* Set reboot kind = warmboot. */
                MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x450ull) = 0x1;
                /* Patch SDRAM init to perform an SVC immediately after second write */
                MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x634ull) = 0x2E38DFFF;
                MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x638ull) = 0x6001DC28;
                /* Set SVC handler to jump to reboot stub in IRAM. */
                MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x520ull) = 0x4003F000;
                MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x53Cull) = 0x6000F208;
                
                /* Copy reboot stub payload. */
                ams_map_irampage(0x4003F000);
                for (unsigned int i = 0; i < rebootstub_bin_size; i += 4) {
                    MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_AMS_IRAM_PAGE) + i) = read32le(rebootstub_bin, i);
                }
                /* Tell rebootstub to shut down. */
                MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_AMS_IRAM_PAGE) + 0x10) = 0x0;
                ams_unmap_irampage();
                
                MAKE_REG32(MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x400ull) = 0x10;
                while (1) { }
            }
            break;
        default:
            return 2;
    }
    
    return 0;
}

bool configitem_is_recovery_boot(void) {
    uint64_t is_recovery_boot;
    if (configitem_get(true, CONFIGITEM_ISRECOVERYBOOT, &is_recovery_boot) != 0) {
        generic_panic();
    }

    return is_recovery_boot != 0;
}

bool configitem_is_retail(void) {
    uint64_t is_retail;
    if (configitem_get(true, CONFIGITEM_ISRETAIL, &is_retail) != 0) {
        generic_panic();
    }

    return is_retail != 0;
}

bool configitem_is_hiz_mode_enabled(void) {
    return g_hiz_mode_enabled;
}

void configitem_set_hiz_mode_enabled(bool enabled) {
    g_hiz_mode_enabled = enabled;
}

bool configitem_is_debugmode_priv(void) {
    uint64_t debugmode = 0;
    if (configitem_get(true, CONFIGITEM_ISDEBUGMODE, &debugmode) != 0) {
        generic_panic();
    }

    return debugmode != 0;
}

uint64_t configitem_get_hardware_type(void) {
    uint64_t hardware_type;
    if (configitem_get(true, CONFIGITEM_HARDWARETYPE, &hardware_type) != 0) {
        generic_panic();
    }
    return hardware_type;
}

void configitem_set_debugmode_override(bool user, bool priv) {
    g_debugmode_override_user = user;
    g_debugmode_override_priv = priv;
}

uint32_t configitem_get(bool privileged, ConfigItem item, uint64_t *p_outvalue) {
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
            *p_outvalue = PACKAGE2_MAXVER_400_410 - 1;
            break;
        case CONFIGITEM_HARDWARETYPE:
            *p_outvalue = fuse_get_hardware_type();
            break;
        case CONFIGITEM_ISRETAIL:
            *p_outvalue = fuse_get_retail_type();
            break;
        case CONFIGITEM_ISRECOVERYBOOT:
            *p_outvalue = (int)(bootconfig_is_recovery_boot());
            break;
        case CONFIGITEM_DEVICEID:
            *p_outvalue = fuse_get_device_id();
            break;
        case CONFIGITEM_BOOTREASON:
            /* For some reason, Nintendo removed it on 4.0 */
            if (exosphere_get_target_firmware() < ATMOSPHERE_TARGET_FIRMWARE_400) {
                *p_outvalue = bootconfig_get_boot_reason();
            } else {
                result = 2;
            }
            break;
        case CONFIGITEM_MEMORYARRANGE:
            *p_outvalue = bootconfig_get_memory_arrangement();
            break;
        case CONFIGITEM_ISDEBUGMODE:
            if ((privileged && g_debugmode_override_priv) || (!privileged && g_debugmode_override_user)) {
                *p_outvalue = 1;
            } else {
                *p_outvalue = (int)(bootconfig_is_debug_mode());
            }
            break;
        case CONFIGITEM_KERNELCONFIGURATION:
            {
                uint64_t config = bootconfig_get_kernel_configuration();
                /* Always enable usermode exception handlers. */
                config |= KERNELCONFIGFLAG_ENABLE_USER_EXCEPTION_HANDLERS;
                *p_outvalue = config;
            }
            break;
        case CONFIGITEM_HIZMODE:
            *p_outvalue = (int)g_hiz_mode_enabled;
            break;
        case CONFIGITEM_ISQUESTUNIT:
            /* Added on 3.0, used to determine whether console is a kiosk unit. */
            if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_300) {
                *p_outvalue = (fuse_get_reserved_odm(4) >> 10) & 1;
            } else {
                result = 2;
            }
            break;
        case CONFIGITEM_NEWHARDWARETYPE_5X:
            /* Added in 5.x, currently hardcoded to 0. */
            if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_500) {
                *p_outvalue = 0;
            } else {
                result = 2;
            }
            break;
        case CONFIGITEM_NEWKEYGENERATION_5X:
            /* Added in 5.x. */
            if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_500) {
                *p_outvalue = fuse_get_5x_key_generation();
            } else {
                result = 2;
            }
            break;
        case CONFIGITEM_PACKAGE2HASH_5X:
            /* Added in 5.x. */
            if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_500 && bootconfig_is_recovery_boot()) {
                bootconfig_get_package2_hash_for_recovery(p_outvalue);
            } else {
                result = 2;
            }
            break;
        case CONFIGITEM_EXOSPHERE_VERSION:
            /* UNOFFICIAL: Gets information about the current exosphere version. */
            *p_outvalue = ((uint64_t)(ATMOSPHERE_RELEASE_VERSION_MAJOR & 0xFF) << 32ull) | 
                          ((uint64_t)(ATMOSPHERE_RELEASE_VERSION_MINOR & 0xFF) << 24ull) |
                          ((uint64_t)(ATMOSPHERE_RELEASE_VERSION_MICRO & 0xFF) << 16ull) |
                          ((uint64_t)(exosphere_get_target_firmware() & 0xFF) << 8ull) |
                          ((uint64_t)(mkey_get_revision() & 0xFF) << 0ull);
            break;
        case CONFIGITEM_NEEDS_REBOOT:
            /* UNOFFICIAL: The fact that we are executing means we aren't in the process of rebooting. */
            *p_outvalue = 0;
            break;
        case CONFIGITEM_NEEDS_SHUTDOWN:
            /* UNOFFICIAL: The fact that we are executing means we aren't in the process of shutting down. */
            *p_outvalue = 0;
            break;
        case CONFIGITEM_EXOSPHERE_VERHASH:
            /* UNOFFICIAL: Gets information about the current exosphere git commit hash. */
            *p_outvalue = ATMOSPHERE_RELEASE_VERSION_HASH;
            break;
        case CONFIGITEM_HAS_RCM_BUG_PATCH:
            /* UNOFFICIAL: Gets whether this unit has the RCM bug patched. */
            *p_outvalue = (int)(fuse_has_rcm_bug_patch());;
            break;
        default:
            result = 2;
            break;
    }
    return result;
}
