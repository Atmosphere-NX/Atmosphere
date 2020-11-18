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

namespace ams::diag {

    void AbortImpl() {
        AMS_SECMON_LOG("AbortImpl was called\n");
        AMS_LOG_FLUSH();
        reg::Write(0x4, 0xAAAAAAAA);

        /* TODO: Reboot */
        AMS_INFINITE_LOOP();
    }

    #include <exosphere/diag/diag_detailed_assertion_impl.inc>

}
