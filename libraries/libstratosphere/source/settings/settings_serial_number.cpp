/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere.hpp>
#include "impl/settings_serial_number_impl.hpp"

namespace ams::settings::factory {

    Result GetSerialNumber(SerialNumber *out) {
        R_TRY_CATCH(settings::impl::GetSerialNumber(out)) {
            /* It's not a fatal error if the calib filesystem is corrupted. */
            R_CATCH_RETHROW(settings::ResultCalibrationDataFileSystemCorrupted)
            R_CATCH_RETHROW(settings::ResultCalibrationDataCrcError)
        } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

        R_SUCCEED();
    }

}

namespace ams::settings::system {

    void GetSerialNumber(SerialNumber *out) {
        R_ABORT_UNLESS(settings::impl::GetSerialNumber(out));
    }

}
