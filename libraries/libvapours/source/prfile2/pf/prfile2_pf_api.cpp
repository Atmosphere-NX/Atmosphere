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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif
#include "prfile2_pf_errnum.hpp"

namespace ams::prfile2::pf {


    int Initialize(u32 config, void *param) {
        /* Initialize the fatfs api. */
        if (auto err = fatfs::Initialize(config, param)) {
            return ConvertReturnValue(err);
        }

        /* Initialize the system api. */
        system::Initialize();
        return ConvertReturnValue(pf::Error_Ok);
    }

    int Attach(DriveTable **drive_tables) {
        /* Check parameters. */
        if (drive_tables == nullptr || *drive_tables == nullptr) {
            return ConvertReturnValue(SetInternalErrorAndReturn(pf::Error_InvalidParameter));
        }

        /* Attach each volume in the list. */
        for (auto *table = *drive_tables; table != nullptr; table = *(++drive_tables)) {
            if (auto err = vol::Attach(table); err != pf::Error_Ok) {
                /* Clear each unattached drive character. */
                for (table = *drive_tables; table != nullptr; table = *(++drive_tables)) {
                    table->drive_char = 0;
                }
                return ConvertReturnValue(err);
            }
        }

        return ConvertReturnValue(pf::Error_Ok);
    }


}
