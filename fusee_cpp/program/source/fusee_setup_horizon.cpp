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
#include "fusee_secondary_archive.hpp"
#include "fusee_setup_horizon.hpp"
#include "fusee_fatal.hpp"

namespace ams::nxboot {

    namespace {

        void DeriveAllKeys(const fuse::SocType soc_type) {
            /* If on erista, run the TSEC keygen firmware. */
            if (soc_type == fuse::SocType_Erista) {
                clkrst::SetBpmpClockRate(clkrst::BpmpClockRate_408MHz);

                if (!tsec::RunTsecFirmware(GetSecondaryArchive().tsec_keygen, sizeof(GetSecondaryArchive().tsec_keygen))) {
                    ShowFatalError("Failed to run tsec_keygen firmware!\n");
                }

                clkrst::SetBpmpClockRate(clkrst::BpmpClockRate_576MHz);
            }

            ShowFatalError("DeriveAllKeys not fully implemented\n");
        }

    }

    void SetupAndStartHorizon() {
        /* Get soc/hardware type. */
        const auto soc_type = fuse::GetSocType();
        const auto hw_type  = fuse::GetHardwareType();

        /* Derive all keys. */
        DeriveAllKeys(soc_type);

        AMS_UNUSED(hw_type);
        ShowFatalError("SetupAndStartHorizon not fully implemented\n");
    }

}