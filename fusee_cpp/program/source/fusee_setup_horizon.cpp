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
#include <exosphere.hpp>
#include "fusee_key_derivation.hpp"
#include "fusee_secondary_archive.hpp"
#include "fusee_setup_horizon.hpp"
#include "fusee_fatal.hpp"

namespace ams::nxboot {

    namespace {

        constexpr inline const uintptr_t CLKRST  = secmon::MemoryRegionPhysicalDeviceClkRst.GetAddress();
        constexpr inline const uintptr_t MC      = secmon::MemoryRegionPhysicalDeviceMemoryController.GetAddress();

        void DisableArc() {
            /* Enable ARC_CLK_OVR_ON. */
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD, CLK_RST_REG_BITS_ENUM(LVL2_CLK_GATE_OVRD_ARC_CLK_OVR_ON, OFF));

            /* Enable the ARC. */
            reg::ReadWrite(MC + MC_IRAM_REG_CTRL, MC_REG_BITS_ENUM(IRAM_REG_CTRL_IRAM_CFG_WRITE_ACCESS, DISABLED));

            /* Set IRAM BOM/TOP to open up access to all mmio. */
            reg::Write(MC + MC_IRAM_BOM, 0xFFFFF000);
            reg::Write(MC + MC_IRAM_TOM, 0x00000000);

            /* Read to ensure our configuration takes. */
            reg::Read(MC + MC_IRAM_REG_CTRL);
        }

        void DeriveAllKeys(const fuse::SocType soc_type) {
            /* If on erista, run the TSEC keygen firmware. */
            if (soc_type == fuse::SocType_Erista) {
                clkrst::SetBpmpClockRate(clkrst::BpmpClockRate_408MHz);

                if (!tsec::RunTsecFirmware(GetSecondaryArchive().tsec_keygen, sizeof(GetSecondaryArchive().tsec_keygen))) {
                    ShowFatalError("Failed to run tsec_keygen firmware!\n");
                }

                clkrst::SetBpmpClockRate(clkrst::BpmpClockRate_576MHz);
            }

            /* Derive master/device keys. */
            if (soc_type == fuse::SocType_Erista) {
                DeriveKeysErista();
            } else /* if (soc_type == fuse::SocType_Mariko) */ {
                DeriveKeysMariko();
            }
        }

    }

    void SetupAndStartHorizon() {
        /* Get soc/hardware type. */
        const auto soc_type = fuse::GetSocType();
        const auto hw_type  = fuse::GetHardwareType();

        /* Derive all keys. */
        DeriveAllKeys(soc_type);

        /* Disable the ARC redirect. */
        /* NOTE: Devices can no longer access IRAM from this point onwards. */
        DisableArc();

        AMS_UNUSED(hw_type);
        ShowFatalError("SetupAndStartHorizon not fully implemented\n");
    }

}