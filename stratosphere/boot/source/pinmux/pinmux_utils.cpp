/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "pinmux_utils.hpp"

namespace ams::pinmux {

    namespace {

        /* Pull in Pinmux map definitions. */
#include "pinmux_map.inc"

        constexpr u32 ApbMiscPhysicalBase = 0x70000000;

        /* Globals. */
        bool g_initialized_pinmux_vaddr = false;
        uintptr_t g_pinmux_vaddr = 0;

        /* Helpers. */
        inline const Definition *GetDefinition(u32 pinmux_name) {
            AMS_ABORT_UNLESS(pinmux_name < PadNameMax);
            return &Map[pinmux_name];
        }

        inline const DrivePadDefinition *GetDrivePadDefinition(u32 drivepad_name) {
            AMS_ABORT_UNLESS(drivepad_name < DrivePadNameMax);
            return &DrivePadMap[drivepad_name];
        }

        uintptr_t GetBaseAddress() {
            if (!g_initialized_pinmux_vaddr) {
                g_pinmux_vaddr = dd::GetIoMapping(ApbMiscPhysicalBase, 0x4000);
                g_initialized_pinmux_vaddr = true;
            }
            return g_pinmux_vaddr;
        }



    }

    u32 UpdatePark(u32 pinmux_name) {
        const uintptr_t pinmux_base_vaddr = GetBaseAddress();
        const Definition *pinmux_def = GetDefinition(pinmux_name);

        /* Fetch this PINMUX's register offset */
        u32 pinmux_reg_offset = pinmux_def->reg_offset;

        /* Fetch this PINMUX's mask value */
        u32 pinmux_mask_val = pinmux_def->mask_val;

        /* Get current register ptr. */
        uintptr_t pinmux_reg = pinmux_base_vaddr + pinmux_reg_offset;

        /* Read from the PINMUX register */
        u32 pinmux_val = reg::Read(pinmux_reg);

        /* This PINMUX supports park change */
        if (pinmux_mask_val & 0x20) {
            /* Clear park status if set */
            if (pinmux_val & 0x20) {
                pinmux_val &= ~(0x20);
            }
        }

        /* Write to the appropriate PINMUX register */
        reg::Write(pinmux_reg, pinmux_val);

        /* Do a dummy read from the PINMUX register */
        pinmux_val = reg::Read(pinmux_reg);

        return pinmux_val;
    }

    u32 UpdatePad(u32 pinmux_name, u32 pinmux_config_val, u32 pinmux_config_mask_val) {
        const uintptr_t pinmux_base_vaddr = GetBaseAddress();
        const Definition *pinmux_def = GetDefinition(pinmux_name);

        /* Fetch this PINMUX's register offset */
        u32 pinmux_reg_offset = pinmux_def->reg_offset;

        /* Fetch this PINMUX's mask value */
        u32 pinmux_mask_val = pinmux_def->mask_val;

        /* Get current register ptr. */
        uintptr_t pinmux_reg = pinmux_base_vaddr + pinmux_reg_offset;

        /* Read from the PINMUX register */
        u32 pinmux_val = reg::Read(pinmux_reg);

        /* This PINMUX register is locked */
        AMS_ABORT_UNLESS((pinmux_val & 0x80) == 0);

        u32 pm_val = (pinmux_config_val & 0x07);

        /* Adjust PM */
        if (pinmux_config_mask_val & 0x07) {
            /* Apply additional changes first */
            if (pm_val == 0x05) {
                /* This pin supports PUPD change */
                if (pinmux_mask_val & 0x0C) {
                    /* Change PUPD */
                    if ((pinmux_val & 0x0C) != 0x04) {
                        pinmux_val &= 0xFFFFFFF3;
                        pinmux_val |= 0x04;
                    }
                }

                /* This pin supports Tristate change */
                if (pinmux_mask_val & 0x10) {
                    /* Change Tristate */
                    if (!(pinmux_val & 0x10)) {
                        pinmux_val |= 0x10;
                    }
                }

                /* This pin supports EInput change */
                if (pinmux_mask_val & 0x40) {
                    /* Change EInput */
                    if (pinmux_val & 0x40) {
                        pinmux_val &= 0xFFFFFFBF;
                    }
                }
            } else if (pm_val >= 0x06) {
                /* Default to safe value */
                pm_val = 0x04;
            }

            /* Translate PM value if necessary */
            if (pm_val == 0x04 || pm_val == 0x05) {
                pm_val = pinmux_def->pm_val;
            }

            /* This pin supports PM change */
            if (pinmux_mask_val & 0x03) {
                /* Change PM */
                if ((pinmux_val & 0x03) != (pm_val & 0x03)) {
                    pinmux_val &= 0xFFFFFFFC;
                    pinmux_val |= (pm_val & 0x03);
                }
            }
        }

        u32 pupd_config_val = (pinmux_config_val & 0x18);

        /* Adjust PUPD */
        if (pinmux_config_mask_val & 0x18) {
            if (pupd_config_val < 0x11) {
                /* This pin supports PUPD change */
                if (pinmux_mask_val & 0x0C) {
                    /* Change PUPD */
                    if (((pinmux_val >> 0x02) & 0x03) != (pupd_config_val >> 0x03)) {
                        pinmux_val &= 0xFFFFFFF3;
                        pinmux_val |= (pupd_config_val >> 0x01);
                    }
                }
            }
        }

        u32 eod_config_val = (pinmux_config_val & 0x60);

        /* Adjust EOd field */
        if (pinmux_config_mask_val & 0x60) {
            if (eod_config_val == 0x20) {
                /* This pin supports Tristate change */
                if (pinmux_mask_val & 0x10) {
                    /* Change Tristate */
                    if (!(pinmux_val & 0x10)) {
                        pinmux_val |= 0x10;
                    }
                }

                /* This pin supports EInput change */
                if (pinmux_mask_val & 0x40) {
                    /* Change EInput */
                    if (!(pinmux_val & 0x40)) {
                        pinmux_val |= 0x40;
                    }
                }

                /* This pin supports EOd change */
                if (pinmux_mask_val & 0x800) {
                    /* Change EOd */
                    if (pinmux_val & 0x800) {
                        pinmux_val &= 0xFFFFF7FF;
                    }
                }
            } else if (eod_config_val == 0x40) {
                /* This pin supports Tristate change */
                if (pinmux_mask_val & 0x10) {
                    /* Change Tristate */
                    if (pinmux_val & 0x10) {
                        pinmux_val &= 0xFFFFFFEF;
                    }
                }

                /* This pin supports EInput change */
                if (pinmux_mask_val & 0x40) {
                    /* Change EInput */
                    if (!(pinmux_val & 0x40)) {
                        pinmux_val |= 0x40;
                    }
                }

                /* This pin supports EOd change */
                if (pinmux_mask_val & 0x800) {
                    /* Change EOd */
                    if (pinmux_val & 0x800) {
                        pinmux_val &= 0xFFFFF7FF;
                    }
                }
            } else if (eod_config_val == 0x60) {
                /* This pin supports Tristate change */
                if (pinmux_mask_val & 0x10) {
                    /* Change Tristate */
                    if (pinmux_val & 0x10) {
                        pinmux_val &= 0xFFFFFFEF;
                    }
                }

                /* This pin supports EInput change */
                if (pinmux_mask_val & 0x40) {
                    /* Change EInput */
                    if (!(pinmux_val & 0x40)) {
                        pinmux_val |= 0x40;
                    }
                }

                /* This pin supports EOd change */
                if (pinmux_mask_val & 0x800) {
                    /* Change EOd */
                    if (!(pinmux_val & 0x800)) {
                        pinmux_val |= 0x800;
                    }
                }
            } else {
                /* This pin supports Tristate change */
                if (pinmux_mask_val & 0x10) {
                    /* Change Tristate */
                    if (pinmux_val & 0x10) {
                        pinmux_val &= 0xFFFFFFEF;
                    }
                }

                /* This pin supports EInput change */
                if (pinmux_mask_val & 0x40) {
                    /* Change EInput */
                    if (pinmux_val & 0x40) {
                        pinmux_val &= 0xFFFFFFBF;
                    }
                }

                /* This pin supports EOd change */
                if (pinmux_mask_val & 0x800) {
                    /* Change EOd */
                    if (pinmux_val & 0x800) {
                        pinmux_val &= 0xFFFFF7FF;
                    }
                }
            }
        }

        u32 lock_config_val = (pinmux_config_val & 0x80);

        /* Adjust Lock */
        if (pinmux_config_mask_val & 0x80) {
            /* This pin supports Lock change */
            if (pinmux_mask_val & 0x80) {
                /* Change Lock */
                if ((pinmux_val ^ pinmux_config_val) & 0x80) {
                    pinmux_val &= 0xFFFFFF7F;
                    pinmux_val |= lock_config_val;
                }
            }
        }

        u32 ioreset_config_val = (((pinmux_config_val >> 0x08) & 0x1) << 0x10);

        /* Adjust IoReset */
        if (pinmux_config_mask_val & 0x100) {
            /* This pin supports IoReset change */
            if (pinmux_mask_val & 0x10000) {
                /* Change IoReset */
                if (((pinmux_val >> 0x10) ^ (pinmux_config_val >> 0x08)) & 0x01) {
                    pinmux_val &= 0xFFFEFFFF;
                    pinmux_val |= ioreset_config_val;
                }
            }
        }

        u32 park_config_val = (((pinmux_config_val >> 0x0A) & 0x1) << 0x5);

        /* Adjust Park */
        if (pinmux_config_mask_val & 0x400) {
            /* This pin supports Park change */
            if (pinmux_mask_val & 0x20) {
                /* Change Park */
                if (((pinmux_val >> 0x05) ^ (pinmux_config_val >> 0x0A)) & 0x01) {
                    pinmux_val &= 0xFFFFFFDF;
                    pinmux_val |= park_config_val;
                }
            }
        }

        u32 elpdr_config_val = (((pinmux_config_val >> 0x0B) & 0x1) << 0x08);

        /* Adjust ELpdr */
        if (pinmux_config_mask_val & 0x800) {
            /* This pin supports ELpdr change */
            if (pinmux_mask_val & 0x100) {
                /* Change ELpdr */
                if (((pinmux_val >> 0x08) ^ (pinmux_config_val >> 0x0B)) & 0x01) {
                    pinmux_val &= 0xFFFFFEFF;
                    pinmux_val |= elpdr_config_val;
                }
            }
        }

        u32 ehsm_config_val = (((pinmux_config_val >> 0x0C) & 0x1) << 0x09);

        /* Adjust EHsm */
        if (pinmux_config_mask_val & 0x1000) {
            /* This pin supports EHsm change */
            if (pinmux_mask_val & 0x200) {
                /* Change EHsm */
                if (((pinmux_val >> 0x09) ^ (pinmux_config_val >> 0x0C)) & 0x01) {
                    pinmux_val &= 0xFFFFFDFF;
                    pinmux_val |= ehsm_config_val;
                }
            }
        }

        u32 eiohv_config_val = (((pinmux_config_val >> 0x09) & 0x1) << 0x0A);

        /* Adjust EIoHv */
        if (pinmux_config_mask_val & 0x200) {
            /* This pin supports EIoHv change */
            if (pinmux_mask_val & 0x400) {
                /* Change EIoHv */
                if (((pinmux_val >> 0x0A) ^ (pinmux_config_val >> 0x09)) & 0x01) {
                    pinmux_val &= 0xFFFFFBFF;
                    pinmux_val |= eiohv_config_val;
                }
            }
        }

        u32 eschmt_config_val = (((pinmux_config_val >> 0x0D) & 0x1) << 0x0C);

        /* Adjust ESchmt */
        if (pinmux_config_mask_val & 0x2000) {
            /* This pin supports ESchmt change */
            if (pinmux_mask_val & 0x1000) {
                /* Change ESchmt */
                if (((pinmux_val >> 0x0C) ^ (pinmux_config_val >> 0x0D)) & 0x01) {
                    pinmux_val &= 0xFFFFEFFF;
                    pinmux_val |= eschmt_config_val;
                }
            }
        }

        u32 preemp_config_val = (((pinmux_config_val >> 0x10) & 0x1) << 0xF);

        /* Adjust Preemp */
        if (pinmux_config_mask_val & 0x10000) {
            /* This pin supports Preemp change */
            if (pinmux_mask_val & 0x8000) {
                /* Change Preemp */
                if (((pinmux_val >> 0x0F) ^ (pinmux_config_val >> 0x10)) & 0x01) {
                    pinmux_val &= 0xFFFF7FFF;
                    pinmux_val |= preemp_config_val;
                }
            }
        }

        u32 drvtype_config_val = (((pinmux_config_val >> 0x0E) & 0x3) << 0xD);

        /* Adjust DrvType */
        if (pinmux_config_mask_val & 0xC000) {
            /* This pin supports DrvType change */
            if (pinmux_mask_val & 0x6000) {
                /* Change DrvType */
                if (((pinmux_val >> 0x0D) ^ (pinmux_config_val >> 0x0E)) & 0x03) {
                    pinmux_val &= 0xFFFF9FFF;
                    pinmux_val |= drvtype_config_val;
                }
            }
        }

        /* Write to the appropriate PINMUX register */
        reg::Write(pinmux_reg, pinmux_val);

        /* Do a dummy read from the PINMUX register */
        pinmux_val = reg::Read(pinmux_reg);

        return pinmux_val;
    }

    u32 UpdateDrivePad(u32 pinmux_drivepad_name, u32 pinmux_drivepad_config_val, u32 pinmux_drivepad_config_mask_val) {
        const uintptr_t pinmux_base_vaddr = GetBaseAddress();
        const DrivePadDefinition *pinmux_drivepad_def = GetDrivePadDefinition(pinmux_drivepad_name);

        /* Fetch this PINMUX drive group's register offset */
        u32 pinmux_drivepad_reg_offset = pinmux_drivepad_def->reg_offset;

        /* Fetch this PINMUX drive group's mask value */
        u32 pinmux_drivepad_mask_val = pinmux_drivepad_def->mask_val;

        /* Get current register ptr. */
        uintptr_t pinmux_drivepad_reg = pinmux_base_vaddr + pinmux_drivepad_reg_offset;

        /* Read from the PINMUX drive group register */
        u32 pinmux_drivepad_val = reg::Read(pinmux_drivepad_reg);

        /* Adjust DriveDownStrength */
        if (pinmux_drivepad_config_mask_val & 0x1F000) {
            u32 mask_val = 0x7F000;

            /* Adjust mask value */
            if ((pinmux_drivepad_mask_val & 0x7F000) != 0x7F000)
                mask_val = 0x1F000;

            /* This drive group supports DriveDownStrength change */
            if (pinmux_drivepad_mask_val & mask_val) {
                /* Change DriveDownStrength */
                if (((pinmux_drivepad_config_val & 0x7F000) & mask_val) != (pinmux_drivepad_val & mask_val)) {
                    pinmux_drivepad_val &= ~(mask_val);
                    pinmux_drivepad_val |= ((pinmux_drivepad_config_val & 0x7F000) & mask_val);
                }
            }
        }

        /* Adjust DriveUpStrength */
        if (pinmux_drivepad_config_mask_val & 0x1F00000) {
            u32 mask_val = 0x7F00000;

            /* Adjust mask value */
            if ((pinmux_drivepad_mask_val & 0x7F00000) != 0x7F00000)
                mask_val = 0x1F00000;

            /* This drive group supports DriveUpStrength change */
            if (pinmux_drivepad_mask_val & mask_val) {
                /* Change DriveUpStrength */
                if (((pinmux_drivepad_config_val & 0x7F00000) & mask_val) != (pinmux_drivepad_val & mask_val)) {
                    pinmux_drivepad_val &= ~(mask_val);
                    pinmux_drivepad_val |= ((pinmux_drivepad_config_val & 0x7F00000) & mask_val);
                }
            }
        }

        /* Adjust DriveDownSlew */
        if (pinmux_drivepad_config_mask_val & 0x30000000) {
            /* This drive group supports DriveDownSlew change */
            if (pinmux_drivepad_mask_val & 0x30000000) {
                /* Change DriveDownSlew */
                if ((pinmux_drivepad_val ^ pinmux_drivepad_config_val) & 0x30000000) {
                    pinmux_drivepad_val &= 0xCFFFFFFF;
                    pinmux_drivepad_val |= (pinmux_drivepad_config_val & 0x30000000);
                }
            }
        }

        /* Adjust DriveUpSlew */
        if (pinmux_drivepad_config_mask_val & 0xC0000000) {
            /* This drive group supports DriveUpSlew change */
            if (pinmux_drivepad_mask_val & 0xC0000000) {
                /* Change DriveUpSlew */
                if ((pinmux_drivepad_val ^ pinmux_drivepad_config_val) & 0xC0000000) {
                    pinmux_drivepad_val &= 0x3FFFFFFF;
                    pinmux_drivepad_val |= (pinmux_drivepad_config_val & 0xC0000000);
                }
            }
        }

        /* Write to the appropriate PINMUX drive group register */
        reg::Write(pinmux_drivepad_reg, pinmux_drivepad_val);

        /* Do a dummy read from the PINMUX drive group register */
        pinmux_drivepad_val = reg::Read(pinmux_drivepad_reg);

        return pinmux_drivepad_val;
    }

    u32 DummyReadDrivePad(u32 pinmux_drivepad_name) {
        const uintptr_t pinmux_base_vaddr = GetBaseAddress();
        const DrivePadDefinition *pinmux_drivepad_def = GetDrivePadDefinition(pinmux_drivepad_name);

        /* Fetch this PINMUX drive group's register offset */
        u32 pinmux_drivepad_reg_offset = pinmux_drivepad_def->reg_offset;

        /* Get current register ptr. */
        uintptr_t pinmux_drivepad_reg = pinmux_base_vaddr + pinmux_drivepad_reg_offset;

        return reg::Read(pinmux_drivepad_reg);
    }

    void UpdateAllParks() {
        /* Update parks. */
        for (size_t i = 0; i < PadNameMax; i++) {
            UpdatePark(static_cast<u32>(i));
        }
    }

    void DummyReadAllDrivePads() {
        /* Dummy read all drive pads. */
        for (size_t i = 0; i < DrivePadNameMax; i++) {
            DummyReadDrivePad(static_cast<u32>(i));
        }
    }

}
